#include "keys_db.h"

/* class KeysDataBaseHelper */

std::string KeysDataBaseHelper::get_system_time() {
    // Получаем текущее время системы
    std::time_t now = std::time(nullptr);
    std::tm localTime;
    localtime_r(&now, &localTime);

    // Форматируем дату и время в строку "DD.MM.YYYY HH:MM:SS"
    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%d.%m.%Y %H:%M:%S", &localTime);
    return std::string(buffer);
}

void KeysDataBaseHelper::members_info_addition(std::string& keys_db, std::string& ids_db, const std::vector<std::string>& keys, const std::vector<std::string>& ids) {
    // Дополним стоку keys_db новыми ключами
    for (const auto& key : keys) {
        keys_db += key;
    }
    // Дополним строку ids_db новыми ключами и разделителем ';'
    for (const auto& id : ids) {
        ids_db += id + ';';
    }
}

std::vector<std::string> KeysDataBaseHelper::string_to_vector(const std::string& ids_str) {
    std::vector<std::string> ids_vec = {};
    std::string id;

    for (const char sym : ids_str) {
        if (sym == ';') {
            ids_vec.push_back(id);
            id = "";
            continue;
        }
        id.push_back(sym);
    }
    return ids_vec;
}

std::string KeysDataBaseHelper::vector_to_string(const std::vector<std::string>& ids_vec) {
    std::string ids_str;
    for (const auto& v : ids_vec) {
        ids_str += v + ";";
    }
    return ids_str;
}

/* class KeysDataBase */

void KeysDataBase::create_keys_tables() {
    Statement stmt(db, create_aes_table_request);
    stmt.execute();
    stmt = Statement(db, create_rsa_table_request);
    stmt.execute();
    stmt = Statement(db, create_chats_table_request);
    stmt.execute();
}

/*
std::optional<std::vector<std::string>> KeysDataBase::get_row(const std::string& chat_id, const KeysTablesDefs table) {
    const std::string sql_request = "SELECT * FROM " + KeysTablesUndefs.at(table) + " WHERE chat_id = ?;";
    Statement stmt(db, sql_request);
    stmt.bind_text(1, chat_id);
    return stmt.execute_row(static_cast<int>(KeysColumnsCount.at(table)));
}
*/

void KeysDataBase::disable_other_keys(const std::string& chat_id, const KeysTablesDefs table) {
    const std::string sql_request = "UPDATE " + KeysTablesUndefs.at(table) + " SET status = -1 WHERE (status = 1 OR status = 0) AND chat_id = ?;";
    Statement stmt(db, sql_request);
    stmt.bind_text(1, chat_id);
    stmt.execute();
}

void KeysDataBase::enable_key(const std::string& chat_id, const KeysTablesDefs table) {
    const std::string sql_request = "UPDATE " + KeysTablesUndefs.at(table) + " SET status = 1 WHERE status = 0 AND chat_id = ?;";
    Statement stmt(db, sql_request);
    stmt.bind_text(1, chat_id);
    stmt.execute();
}

void KeysDataBase::set_sent_flag(const std::string& chat_id, const int key_n, const KeysTablesDefs table) {
    const std::string sql_request = "UPDATE " + KeysTablesUndefs.at(table) + " SET sent_in_chat = 1 WHERE chat_id = ? AND key_n = ?;";
    Statement stmt(db, sql_request);
    stmt.bind_text(1, chat_id);
    stmt.bind_int(2, key_n);
    stmt.execute();
}

void KeysDataBase::add_aes_key(const AesParamsFiller& data) {
    const std::string sql_request = "INSERT INTO aes (chat_id, key_n, session_key, p, g, public_key, private_key, date, messages, status, sent_in_chat, sent_members) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    std::string date = KeysDataBaseHelper::get_system_time();

    std::optional<int> members_count = get_active_param_int(data.chat_id, KeysTablesDefs::CHATS, ChatsColumnsDefs::MEMBERS_COUNT);
    if (!members_count) { throw std::runtime_error("Ошибка получения параметра"); }

    std::vector<std::string> sent_in_chat_vec(*members_count, "");
    std::string sent_in_chat = KeysDataBaseHelper::vector_to_string(sent_in_chat_vec);

    disable_other_keys(data.chat_id, KeysTablesDefs::AES); // сбросить флаги "используется" у всех aes ключей этого чата

    Statement stmt(db, sql_request);

    stmt.bind_text(1, data.chat_id);
    stmt.bind_int(2, data.key_n);
    stmt.bind_text(3, data.session_key);
    stmt.bind_text(4, data.p);
    stmt.bind_text(5, data.g);
    stmt.bind_text(6, data.public_key);
    stmt.bind_text(7, data.private_key);
    stmt.bind_text(8, date);
    stmt.bind_int(9, 0); // messages = 0   
    stmt.bind_int(10, data.status);
    stmt.bind_text(11, sent_in_chat);
    stmt.bind_int(12, 1); // sent_members = 1

    stmt.execute();
}

void KeysDataBase::add_rsa_key(const RsaParamsFiller& data, const std::string& my_public_key) {
    const std::string sql_request = "INSERT INTO rsa (chat_id, key_n, key_len, sent_members, private_key, members_public_keys, date, messages, status, sent_in_chat) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    std::string date = KeysDataBaseHelper::get_system_time();

    std::optional<int> members_count = get_active_param_int(data.chat_id, KeysTablesDefs::CHATS, ChatsColumnsDefs::MEMBERS_COUNT);
    if (!members_count) { throw std::runtime_error("Ошибка получения параметра"); }
    std::optional<int> my_id_pos = get_active_param_int(data.chat_id, KeysTablesDefs::CHATS, ChatsColumnsDefs::MY_ID_POS);
    if (!members_count) { throw std::runtime_error("Ошибка получения параметра"); }

    std::vector<std::string> members_public_keys_vec(*members_count, "");
    members_public_keys_vec[*my_id_pos] = my_public_key;
    std::string members_public_keys = KeysDataBaseHelper::vector_to_string(members_public_keys_vec);

    disable_other_keys(data.chat_id, KeysTablesDefs::RSA); // сбросить флаги "используется" у всех rsa ключей этого чата
    
    Statement stmt(db, sql_request);

    stmt.bind_text(1, data.chat_id);
    stmt.bind_int(2, data.key_n);
    stmt.bind_int(3, data.key_len);
    stmt.bind_int(4, 1); // sent_members = 1
    stmt.bind_text(5, data.private_key);
    stmt.bind_text(6, members_public_keys);
    stmt.bind_text(7, date);
    stmt.bind_int(8, 0); // messages = 0   
    stmt.bind_int(9, 0); // status = 0
    stmt.bind_int(10, data.sent_in_chat); // status = 0
    stmt.execute();
}

void KeysDataBase::add_chat_params(const ChatsParamsFiller& data) {
    const std::string sql_request = "INSERT INTO chats (chat_id, my_id_pos, members_count, members_ids, date, status) VALUES (?, ?, ?, ?, ?, ?);";
    std::string date = KeysDataBaseHelper::get_system_time();
    std::string members_ids = KeysDataBaseHelper::vector_to_string(data.members_ids);

    disable_other_keys(data.chat_id, KeysTablesDefs::CHATS); // сбросить флаги "используется" у предыдущих записей

    Statement stmt(db, sql_request);

    stmt.bind_text(1, data.chat_id);
    stmt.bind_int(2, data.my_id_pos);
    stmt.bind_int(3, data.members_ids.size()); // members_count
    stmt.bind_text(4, members_ids);
    stmt.bind_text(5, date);
    stmt.bind_int(6, 1); // status = 1
    stmt.execute();
}

void KeysDataBase::add_rsa_member_key(const std::string& chat_id, const int key_n, const std::string& key, const int pos) {
    // Извлекаем существующие списоки параметров
    std::optional<std::string> members_public_keys = get_active_param_text(chat_id, KeysTablesDefs::RSA, RsaColumnsDefs::MEMBERS_PUBLIC_KEYS, 0);
    if (!members_public_keys) { throw std::runtime_error("Ошибка получения параметра"); }

    // Дополняем извлеченные значения новым
    std::vector<std::string> members_public_keys_vec = KeysDataBaseHelper::string_to_vector(*members_public_keys);
    members_public_keys_vec[pos] = key;
    members_public_keys = KeysDataBaseHelper::vector_to_string(members_public_keys_vec);

    // Записываем изменённые значения
    const std::string sql_request = "UPDATE rsa SET sent_members = sent_members + 1, members_public_keys = ? WHERE chat_id = ? AND key_n = ? AND status = 0;";
    Statement stmt(db, sql_request);
    stmt.bind_text(1, *members_public_keys);
    stmt.bind_text(2, chat_id);
    stmt.bind_int(3, key_n);
    stmt.execute();
}

void KeysDataBase::add_aes_sent_in_chat(const std::string& chat_id, const int key_n, const int pos) {
    // Извлекаем существующие списоки параметров
    std::optional<std::string> sent_in_chat = get_active_param_text(chat_id, KeysTablesDefs::AES, RsaColumnsDefs::SENT_IN_CHAT, 0);
    if (!sent_in_chat) { throw std::runtime_error("Ошибка получения параметра"); }

    // Дополняем извлеченные значения новым
    std::vector<std::string> sent_in_chat_vec = KeysDataBaseHelper::string_to_vector(*sent_in_chat);
    sent_in_chat_vec[pos] = "1";
    sent_in_chat = KeysDataBaseHelper::vector_to_string(sent_in_chat_vec);

    // Записываем изменённые значения
    const std::string sql_request = "UPDATE aes SET sent_members = sent_members + 1, sent_in_chat = ? WHERE chat_id = ? AND key_n = ? AND status = 0;";
    Statement stmt(db, sql_request);
    stmt.bind_text(1, *sent_in_chat);
    stmt.bind_text(2, chat_id);
    stmt.bind_int(3, key_n);
    stmt.execute();
}

void KeysDataBase::increase_messages_counter(const std::string& chat_id) {
    // Увеличиваем количество сообщений в таблице aes
    const std::string sql_request_aes = "UPDATE aes SET messages = messages + 1 WHERE chat_id = ? AND status = 1;";
    Statement stmt(db, sql_request_aes);
    stmt.bind_text(1, chat_id);
    stmt.execute();

    // Увеличиваем количество сообщений в таблице rsa
    const std::string sql_request_rsa = "UPDATE rsa SET messages = messages + 1 WHERE chat_id = ? AND status = 1;";
    stmt = Statement(db, sql_request_rsa);
    stmt.bind_text(1, chat_id);
    stmt.execute();
}

std::optional<int> KeysDataBase::get_last_key_n(const std::string& chat_id, const KeysTablesDefs table) {
    const std::string sql_request = "SELECT MAX(key_n) FROM " + KeysTablesUndefs.at(table) + " WHERE chat_id = ?;";
    Statement stmt(db, sql_request);
    stmt.bind_text(1, chat_id);
    return stmt.execute_int(0);
}

KeysDataBase::KeysDataBase() : DataBase(DB_KEYS) {
    create_keys_tables();
}

