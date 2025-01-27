#include <sqlite3.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <ctime> 
#include <iomanip>
#include <type_traits>

#define DB_RELATIVE_PATH "../../db/"
#define DB_FILE_FORMAT ".db"
#define DB_KEYS "keys"

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
    void execute(int check_error = SQLITE_DONE);

    // Выполнить запрос с получения элемента столбца
    std::string execute_text(const int column_index);
    int execute_int(const int column_index);
};


// Класс со вспомогательными функциями, которые не изменяют данные в базе, но нужны для их обработки
class KeysDataBaseHelper {
public:
    // Получение текущего времени в виде строки
    static std::string get_system_time();
    // Дополняем новым значением строку с объединёнными в ней значениями параметров
    static void members_info_addition(std::string& keys_db, std::string& ids_db, const std::vector<std::string>& keys, const std::vector<std::string>& ids);

    // Преобразовать в вектор строку с объединёнными в ней значениями параметров нескольких пользователей
    static std::vector<std::string> keys_string_to_vector(const std::string& keys_str, const int key_len);
    static std::vector<std::string> ids_string_to_vector(const std::string& ids_str);
};


// Строки запросов создания таблиц
const std::string create_aes_table_request = R"(
    CREATE TABLE IF NOT EXISTS aes (
        node_id INTEGER PRIMARY KEY AUTOINCREMENT,
        chat_id TEXT DEFAULT "",
        session_key  TEXT DEFAULT "",
        date TEXT DEFAULT "",
        messages INTEGER DEFAULT 0,
        status INTEGER DEFAULT 0,
        initiator INTEGER DEFAULT 0
    );
)";
const std::string create_rsa_table_request = R"(
    CREATE TABLE IF NOT EXISTS rsa (
        node_id INTEGER PRIMARY KEY AUTOINCREMENT,
        chat_id TEXT DEFAULT "",
        key_len INTEGER DEFAULT 2048,
        total_members INTEGER DEFAULT 0,
        sent_members INTEGER DEFAULT 0,
        private_key  TEXT DEFAULT "",
        members_public_keys TEXT DEFAULT "",
        members_ids TEXT DEFAULT "",
        date TEXT DEFAULT "",
        messages INTEGER DEFAULT 0,
        status INTEGER DEFAULT 0,
        initiator INTEGER DEFAULT 0
    );
)";

// Структуры для задания параметров (значения столбцов) в таблице rsa и aes
struct AesParamsFiller {
    int node_id = 0;               // id записи в таблице
    std::string chat_id = "";      // id чата
    std::string session_key = "";  // общий ключ шифровки и дешифровки сообщений в чате
    std::string date = "";         // дата создания session_key
    int messages = 0;              // количество сообщений, которые были написаны этим ключом
    int status = 0;                // статус ключа: на формировании(0) / устарел(-1) / используется(1)
    int initiator = 0;             // флаг, являетесь ли вы инициатором создания ключа: не являетесь(0) / ялвяетесь (1)
};
struct RsaParamsFiller {
    int node_id = 0;                                    // id записи в таблице
    std::string chat_id = "";                           // id чата
    int key_len = 0;                                    // длинна rsa ключа
    int total_members = 0;                              // общее количество пользователей чата
    int sent_members = 0;                               // количество пользователей чата, которые поделились своим публичным ключом
    std::string private_key = "";                       // ваш приватный ключ для дешифровки приходящих сообщений
    std::vector<std::string> members_public_keys = {};  // cклеенные публичные ключи пользователей чата для отправки им сообщений
    std::vector<std::string> members_ids = {};          // склеенные id пользователей чата
    std::string date = "";                              // дата создания ваших ключей
    int messages = 0;                                   // количество сообщений, поддерживающихся шифровкой rsa
    int status = 0;                                     // статус ключа: на формировании(0) / устарел(-1) / используется(1)
    int initiator = 0;                                  // флаг, являетесь ли вы инициатором создания ключа: не являетесь(0) / ялвяетесь (1)
};

// Определения имён таблиц и порядка колонок
enum class AesColumnsDefs : int {
    NODE_ID,
    CHAT_ID,
    SESSION_KEY,
    DATE,
    MESSAGES,
    STATUS,
    INITIATOR
};
enum class RsaColumnsDefs : int {
    NODE_ID,
    CHAT_ID,
    KEY_LEN,
    TOTAL_MEMBERS,
    SENT_MEMBERS,
    PRIVATE_KEY,
    MEMBERS_PUBLIC_KEYS,
    MEMBERS_IDS,
    DATE,
    MESSAGES,
    STATUS,
    INITIATOR
};
enum class DbTablesDefs : int {
    RSA,
    AES
};

// Перевод числового значения параметров в их строковое определение
const std::unordered_map<DbTablesDefs, std::string> DbTablesUndefs = {
    {DbTablesDefs::RSA, "rsa"},
    {DbTablesDefs::AES, "aes"}
};

// Ограничение возможных типов для обобщения некоторых функций, связанных с таблицей rsa
template <typename T>
constexpr bool is_keys_column = std::is_same_v<T, AesColumnsDefs> || std::is_same_v<T, RsaColumnsDefs>;


// Класс для работы с базой данных ключей пользователей
class KeysDataBase : public DataBase {
private:
    // Создание таблиц
    void create_keys_tables();

    // Перевод статуса ключей из стостояний в состояние
    void disable_other_keys(const std::string& chat_id, const DbTablesDefs table);
    void enable_key(const std::string& chat_id, const DbTablesDefs table);

public:
    // Шаблонные методы получения отдельных параметров в таблице
    template <typename T, typename = std::enable_if_t<is_keys_column<T>>>
    std::string get_active_param_text(const std::string& chat_id, const DbTablesDefs table, const T column, const int active_status) {
        const std::string sql_request = "SELECT * FROM " + DbTablesUndefs.at(table) +" WHERE chat_id = ? AND status = ?;";
        Statement stmt(db, sql_request);
        stmt.bind_text(1, chat_id);
        stmt.bind_int(2, active_status);
        return stmt.execute_text(static_cast<int>(column));
    }
    template <typename T, typename = std::enable_if_t<is_keys_column<T>>>
    int get_active_param_int(const std::string& chat_id, const DbTablesDefs table, const T column, const int active_status) {
        const std::string sql_request = "SELECT * FROM " + DbTablesUndefs.at(table) +" WHERE chat_id = ? AND status = ?;";
        Statement stmt(db, sql_request);
        stmt.bind_text(1, chat_id);
        stmt.bind_int(2, active_status);
        return stmt.execute_int(static_cast<int>(column));
    }

    // Создание своих ключей rsa и aes
    void add_aes_key(const AesParamsFiller& data);
    void add_rsa_key(const RsaParamsFiller& data);

    // Добавление ключей пользователей чата
    void add_rsa_member_key(const RsaParamsFiller& data);

    // Обновление информации о количестве сообщений
    void increase_messages_counter(const std::string& chat_id);

    KeysDataBase();
};  
