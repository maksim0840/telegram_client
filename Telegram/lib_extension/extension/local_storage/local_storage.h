#include <sqlite3.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <ctime> 
#include <iomanip>
#include <type_traits>
#include <optional>
#include <mutex>

// DB_PATH определяется в файле запуска docker-контейнера в файле run.sh
#ifndef DB_PATH
#define DB_PATH "default/path/to/db"
#endif

#define DB_FILE_FORMAT ".db"

#pragma once

// Класс-обёртка базы данных
class DataBase {
protected:
    std::string name_;
    sqlite3* db; // указатель на базу данных

public:
    std::string get_name();
    std::string get_filepath();

    void open_db_file(const std::string& filepath);
    void close_db_file();

    DataBase(const std::string& name);
    ~DataBase();
};


// Класс-обёртка запроса к базе данных
class Statement {
private:
    sqlite3_stmt* stmt;
    sqlite3* db_;
    bool executed_flag; // флаг для проверки факта вызова методов execute с текущим экземляром класса
    static std::mutex db_mutex;

public:
    Statement(sqlite3* db, const std::string& sql_request);
    ~Statement();
    // Оператор присваивания через перемещение - для работы выражения stmt = Statement(db, sql_request);
    Statement& operator=(Statement&& other) noexcept;

    // очищение полей
    void finalize();
    // Получение приватного поля stmt
    sqlite3_stmt* get();

    // Подставить в знак '?' значение value
    void bind_text(const int index, const std::string& value);
    void bind_int(const int index, const int value);

    // Выполнить запрос без получения значений
    bool execute(int check_error = SQLITE_DONE);

    // Выполнить запрос с получения элемента столбца
    std::optional<std::string> execute_text(const int column_index);
    std::optional<int> execute_int(const int column_index);

};