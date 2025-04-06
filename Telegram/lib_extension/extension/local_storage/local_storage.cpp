#include "local_storage.h"


/* class DataBase */

std::string DataBase::get_name() {
    return name_;
}

std::string DataBase::get_filepath() {
    return std::string(DB_PATH) + "/" + name_ + DB_FILE_FORMAT;
}

void DataBase::open_db_file(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(db_mutex); // лочим мьютекс чтобы не было гонки

    if (db) {
        throw std::runtime_error("База данных уже открыта");
    }
    int res = sqlite3_open(filepath.c_str(), &db);
    if (res != SQLITE_OK) {
        throw std::runtime_error("Не удаётся открыть файл");
    }
}

void DataBase::close_db_file() {
    std::lock_guard<std::mutex> lock(db_mutex); // лочим мьютекс чтобы не было гонки

    if (!db) {
        throw std::runtime_error("Попытка закрыть непроинициализированную базу данных");
    }
    int res = sqlite3_close(db);
    if (res != SQLITE_OK) {
        throw std::runtime_error("Не удаётся закрыть базу данных");
    }
    db = nullptr;
}

DataBase::DataBase(const std::string& name) {
    db = nullptr;
    name_ = name;
    open_db_file(get_filepath());
}
DataBase::~DataBase() {
    close_db_file();
}

/* class Statement */

Statement::Statement(sqlite3* db, const std::string& sql_request) {
    std::lock_guard<std::mutex> lock(db_mutex); // лочим мьютекс чтобы не было гонки

    stmt = nullptr;
    db_ = db;
    executed_flag = false;

    int res = sqlite3_prepare_v2(db, sql_request.c_str(), -1, &stmt, nullptr);
    if (res != SQLITE_OK) {
        throw std::runtime_error("Ошибка подготовки запроса: " + std::string(sqlite3_errmsg(db_)));
    }
}

Statement::~Statement() {
    finalize();
}

Statement& Statement::operator=(Statement&& other) noexcept {
    if (this != &other) {
        finalize(); // освобождаем предыдущий stmt, если он существует
        stmt = other.stmt;
        db_ = other.db_;
        other.stmt = nullptr;
        other.db_ = nullptr;
        other.executed_flag = false;
    }
    return *this;
}

void Statement::finalize() {
    std::lock_guard<std::mutex> lock(db_mutex); // лочим мьютекс чтобы не было гонки

    if (stmt) {
        sqlite3_finalize(stmt);
    }
    stmt = nullptr;
    db_ = nullptr;
    executed_flag = false;
}

sqlite3_stmt* Statement::get() {
    return stmt;
}

void Statement::bind_text(const int index, const std::string& value) {
    std::lock_guard<std::mutex> lock(db_mutex); // лочим мьютекс чтобы не было гонки

    int res = sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_STATIC);
    if (res != SQLITE_OK) {
        throw std::runtime_error("Ошибка привязки текста: " + std::string(sqlite3_errmsg(sqlite3_db_handle(stmt))));
    }
}

void Statement::bind_int(const int index, const int value) {
    std::lock_guard<std::mutex> lock(db_mutex); // лочим мьютекс чтобы не было гонки
    
    int res = sqlite3_bind_int(stmt, index, value);
    if (res != SQLITE_OK) {
        throw std::runtime_error("Ошибка привязки целого числа: " + std::string(sqlite3_errmsg(sqlite3_db_handle(stmt))));
    }
}

bool Statement::execute(int check_error) {
    std::lock_guard<std::mutex> lock(db_mutex); // лочим мьютекс чтобы не было гонки

    int res = sqlite3_step(stmt);
    if (res != check_error) {
        return false;
    }
    executed_flag = true;
    return true;
}

std::optional<std::string> Statement::execute_text(const int column_index) {
    if (!executed_flag && !execute(SQLITE_ROW)) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(db_mutex); // лочим мьютекс чтобы не было гонки
    
    const unsigned char* text = sqlite3_column_text(stmt, column_index);
    if (text == nullptr) {
        return "";
    }
    return std::string(reinterpret_cast<const char*>(text));
}

std::optional<int> Statement::execute_int(const int column_index) {
    if (!executed_flag && !execute(SQLITE_ROW)) {
        return std::nullopt;
    }

    std::lock_guard<std::mutex> lock(db_mutex); // лочим мьютекс чтобы не было гонки

    int res = sqlite3_column_int(stmt, column_index);
    return res;
}