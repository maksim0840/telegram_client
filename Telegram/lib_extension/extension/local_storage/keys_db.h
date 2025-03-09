#include "local_storage.h"
#include <optional>
#include <algorithm>

#define DB_KEYS "keys"

#pragma once

// Класс со вспомогательными функциями, которые не изменяют данные в базе, но нужны для их обработки
class KeysDataBaseHelper {
public:
    // Получение текущего времени в виде строки
    static std::string get_system_time();
    // Дополняем новым значением строку с объединёнными в ней значениями параметров
    static void members_info_addition(std::string& keys_db, std::string& ids_db, const std::vector<std::string>& keys, const std::vector<std::string>& ids);

    // Преобразовать в вектор строку с объединёнными в ней значениями параметров нескольких пользователей
    static std::vector<std::string> keys_string_to_vector(const std::string& keys_str, const int key_len);
    static std::vector<std::string> string_to_vector(const std::string& ids_str);

    // Преобразовать вектор в строку с разделителями ';'
    static std::string vector_to_string(const std::vector<std::string>& ids_vec);
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
        key_n INTEGER,
        key_len INTEGER DEFAULT 2048,
        sent_members INTEGER DEFAULT 0,
        private_key  TEXT DEFAULT "",
        members_public_keys TEXT DEFAULT "",
        date TEXT DEFAULT "",
        messages INTEGER DEFAULT 0,
        status INTEGER DEFAULT 0,
        sent_in_chat INTEGER DEFAULT 0
    );
)";
const std::string create_chats_table_request = R"(
    CREATE TABLE IF NOT EXISTS chats (
        node_id INTEGER PRIMARY KEY AUTOINCREMENT,
        chat_id TEXT DEFAULT "",
        my_id_pos INTEGER DEFAULT 0,
        members_count INTEGER DEFAULT 0,
        members_ids TEXT DEFAULT "",
        date TEXT DEFAULT "",
        status INTEGER DEFAULT 0
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
    int key_n = 0;                                      // номер ключа
    int key_len = 0;                                    // длинна rsa ключа
    int sent_members = 0;                               // количество пользователей, которые поделились своим rsa ключом
    std::string private_key = "";                       // ваш приватный ключ для дешифровки приходящих сообщений
    std::vector<std::string> members_public_keys = {};  // cклеенные публичные ключи пользователей чата для отправки им сообщений
    std::string date = "";                              // дата создания ваших ключей
    int messages = 0;                                   // количество сообщений, поддерживающихся шифровкой rsa
    int status = 0;                                     // статус ключа: на формировании(0) / устарел(-1) / используется(1)
    int sent_in_chat = 0;                               // флаг, который устанавливается только в случае отправки сообщения в чат
};
struct ChatsParamsFiller {
    int node_id = 0;                            // id записи в таблице
    std::string chat_id = "";                   // id чата
    int my_id_pos = 0;                          // позиция id вашего аккаунта, в списке members_ids
    int members_count = 0;                      // общее количество пользователей чата
    std::vector<std::string> members_ids = {};  // склеенные id пользователей чата
    std::string date = "";                      // дата создания ваших ключей
    int status = 0;                             // статус информации о пользователях чата: устарел(-1) / используется(1)
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
    KEY_N,
    KEY_LEN,
    SENT_MEMBERS,
    PRIVATE_KEY,
    MEMBERS_PUBLIC_KEYS,
    DATE,
    MESSAGES,
    STATUS,
    SENT_IN_CHAT
};
enum class ChatsColumnsDefs : int {
    NODE_ID,
    CHAT_ID,
    MY_ID_POS,
    MEMBERS_COUNT,
    MEMBERS_IDS,
    DATE
};
enum class KeysTablesDefs : int {
    AES,
    RSA,
    CHATS
};

// Перевод числового значения параметров в их строковое определение
const std::unordered_map<KeysTablesDefs, std::string> KeysTablesUndefs = {
    {KeysTablesDefs::AES, "aes"},
    {KeysTablesDefs::RSA, "rsa"},
    {KeysTablesDefs::CHATS, "chats"}
};

/*
// Количество столбцов в таблице
const std::unordered_map<KeysTablesDefs, int> KeysColumnsCount = {
    {KeysTablesDefs::AES, 7},
    {KeysTablesDefs::RSA, 11},
    {KeysTablesDefs::CHATS, 6}
};
*/

// Ограничение возможных типов для обобщения некоторых функций, связанных с таблицей rsa
template <typename T>
constexpr bool is_keys_column = std::is_same_v<T, AesColumnsDefs> || std::is_same_v<T, RsaColumnsDefs> || std::is_same_v<T, ChatsColumnsDefs>;


// Класс для работы с базой данных ключей пользователей
class KeysDataBase : public DataBase {
private:
    // Создание таблиц
    void create_keys_tables();

public:
    // Шаблонные методы получения отдельных параметров в таблице
    template <typename T, typename = std::enable_if_t<is_keys_column<T>>>
    std::optional<std::string> get_active_param_text(const std::string& chat_id, const KeysTablesDefs table, const T column, const int active_status = 1) {
        const std::string sql_request = "SELECT * FROM " + KeysTablesUndefs.at(table) + " WHERE chat_id = ? AND status = ?;";
        Statement stmt(db, sql_request);
        stmt.bind_text(1, chat_id);
        stmt.bind_int(2, active_status);
        return stmt.execute_text(static_cast<int>(column));
    }
    template <typename T, typename = std::enable_if_t<is_keys_column<T>>>
    std::optional<int> get_active_param_int(const std::string& chat_id, const KeysTablesDefs table, const T column, const int active_status = 1) {
        const std::string sql_request = "SELECT * FROM " + KeysTablesUndefs.at(table) + " WHERE chat_id = ? AND status = ?;";
        Statement stmt(db, sql_request);
        stmt.bind_text(1, chat_id);
        stmt.bind_int(2, active_status);
        return stmt.execute_int(static_cast<int>(column));
    }

    // Шаблонные методы получения отдельных параметров в таблице (+ с указанием key_n)
    template <typename T, typename = std::enable_if_t<is_keys_column<T>>>
    std::optional<std::string> get_active_param_text(const std::string& chat_id, const int key_n, const KeysTablesDefs table, const T column, const int active_status = 1) {
        const std::string sql_request = "SELECT * FROM " + KeysTablesUndefs.at(table) + " WHERE chat_id = ? AND key_n = ? AND status = ?;";
        Statement stmt(db, sql_request);
        stmt.bind_text(1, chat_id);
        stmt.bind_int(2, key_n);
        stmt.bind_int(3, active_status);
        return stmt.execute_text(static_cast<int>(column));
    }
    template <typename T, typename = std::enable_if_t<is_keys_column<T>>>
    std::optional<int> get_active_param_int(const std::string& chat_id, const int key_n, const KeysTablesDefs table, const T column, const int active_status = 1) {
        const std::string sql_request = "SELECT * FROM " + KeysTablesUndefs.at(table) + " WHERE chat_id = ? AND key_n = ? AND status = ?;";
        Statement stmt(db, sql_request);
        stmt.bind_text(1, chat_id);
        stmt.bind_int(2, key_n);
        stmt.bind_int(3, active_status);
        return stmt.execute_int(static_cast<int>(column));
    }

    /*
    // Получить все параметры в строке
    std::optional<std::vector<std::string>> get_row(const std::string& chat_id, const KeysTablesDefs table);
    */

    // Перевод статуса ключей из стостояний в состояние
    void disable_other_keys(const std::string& chat_id, const KeysTablesDefs table);
    void enable_key(const std::string& chat_id, const KeysTablesDefs table);

    // Выставление флага отправки
    void set_sent_flag(const std::string& chat_id, const int key_n, const KeysTablesDefs table);

    // Создание своих ключей rsa и aes
    void add_aes_key(const AesParamsFiller& data);
    void add_rsa_key(const RsaParamsFiller& data, const std::string& my_public_key);

    // Добавление ключей пользователей чата (возвращает true - если все собеседники обменялись ключами, false - иначе)
    void add_rsa_member_key(const std::string& chat_id, const std::string& key, const int pos);

    // Добавляем информацию о чате
    void add_chat_params(const ChatsParamsFiller& data);

    // Обновление информации о количестве сообщений
    void increase_messages_counter(const std::string& chat_id);

    // Получить самый большой по значению key_n из таблицы
    std::optional<int> get_last_key_n(const std::string& chat_id, const KeysTablesDefs table);

    KeysDataBase();
};  
