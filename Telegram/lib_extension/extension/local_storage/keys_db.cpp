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

std::vector<std::string> KeysDataBaseHelper::keys_string_to_vector(const std::string& keys_str, const int key_len) {
    std::vector<std::string> keys_vec = {};
    std::string key;
    int len = 0;
    
    for (const char sym : keys_str) {
        key.push_back(sym);
        ++len;

        if (len == key_len) {
            keys_vec.push_back(key);
            key = "";
            len = 0;
        }
    }
    return keys_vec;
}

std::vector<std::string> KeysDataBaseHelper::ids_string_to_vector(const std::string& ids_str) {
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

std::string KeysDataBaseHelper::ids_vector_to_string(const std::vector<std::string>& ids_vec) {
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

void KeysDataBase::add_aes_key(const AesParamsFiller& data) {
    const std::string sql_request = "INSERT INTO aes (chat_id, session_key, date, messages, status, initiator) VALUES (?, ?, ?, ?, ?, ?);";
    std::string date = KeysDataBaseHelper::get_system_time();

    disable_other_keys(data.chat_id, KeysTablesDefs::AES); // сбросить флаги "используется" у всех aes ключей этого чата

    Statement stmt(db, sql_request);

    stmt.bind_text(1, data.chat_id);
    stmt.bind_text(2, data.session_key);
    stmt.bind_text(3, date);
    stmt.bind_int(4, 0); // messages = 0   
    stmt.bind_int(5, 1); // status = 1
    stmt.bind_int(6, data.initiator);

    stmt.execute();
}

void KeysDataBase::add_rsa_key(const RsaParamsFiller& data) {
    const std::string sql_request = "INSERT INTO rsa (chat_id, key_len, total_members, sent_members, private_key, date, messages, status, initiator) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);";
    std::string date = KeysDataBaseHelper::get_system_time();

    disable_other_keys(data.chat_id, KeysTablesDefs::RSA); // сбросить флаги "используется" у всех rsa ключей этого чата
    
    Statement stmt(db, sql_request);

    stmt.bind_text(1, data.chat_id);
    stmt.bind_int(2, data.key_len);
    stmt.bind_int(3, data.total_members);
    stmt.bind_int(4, 0); // sent_members = 0
    stmt.bind_text(5, data.private_key);
    stmt.bind_text(6, date);
    stmt.bind_int(7, 0); // messages = 0   
    stmt.bind_int(8, 0); // status = 0
    stmt.bind_int(9, data.initiator);

    stmt.execute();
}

void KeysDataBase::add_chat_params(const ChatsParamsFiller& data) {
    const std::string sql_request = "INSERT INTO chats (chat_id, my_id, members_count, members_ids, date, status) VALUES (?, ?, ?, ?, ?, ?);";
    std::string date = KeysDataBaseHelper::get_system_time();
    std::string members_ids = KeysDataBaseHelper::ids_vector_to_string(data.members_ids);

    disable_other_keys(data.chat_id, KeysTablesDefs::CHATS); // сбросить флаги "используется" у предыдущих записей

    Statement stmt(db, sql_request);

    stmt.bind_text(1, data.chat_id);
    stmt.bind_text(2, data.my_id);
    stmt.bind_int(3, data.members_ids.size()); // members_count
    stmt.bind_text(4, members_ids);
    stmt.bind_text(5, date);
    stmt.bind_int(6, 1); // status = 1

    stmt.execute();
}

void KeysDataBase::add_rsa_member_key(const RsaParamsFiller& data) {
    // Извлекаем существующие списоки парамтеров
    std::optional<std::string> members_public_keys = get_active_param_text(data.chat_id, KeysTablesDefs::RSA, RsaColumnsDefs::MEMBERS_PUBLIC_KEYS, 0);
    if (!members_public_keys) { throw std::runtime_error("Ошибка получения параметра"); }
    std::optional<std::string>  members_ids = get_active_param_text(data.chat_id, KeysTablesDefs::RSA, RsaColumnsDefs::MEMBERS_IDS, 0);
    if (!members_ids) { throw std::runtime_error("Ошибка получения параметра"); }

    // Дополняем извлеченные значения новыми
    KeysDataBaseHelper::members_info_addition(*members_public_keys, *members_ids, data.members_public_keys, data.members_ids);

    // Записываем изменённые значения
    const std::string sql_request = "UPDATE rsa SET sent_members = sent_members + 1, members_public_keys = ?, members_ids = ? WHERE chat_id = ? AND status = 0;";
    Statement stmt(db, sql_request);
    stmt.bind_text(1, *members_public_keys);
    stmt.bind_text(2, *members_ids);
    stmt.bind_text(3, data.chat_id);
    stmt.execute();

    // Проверяем количество пользователей, которые отправили свои ключи
    std::optional<int> total_members = get_active_param_int(data.chat_id, KeysTablesDefs::RSA, RsaColumnsDefs::TOTAL_MEMBERS, 0);
    if (!total_members) { throw std::runtime_error("Ошибка получения параметра"); }
    std::optional<int> sent_members = get_active_param_int(data.chat_id, KeysTablesDefs::RSA, RsaColumnsDefs::SENT_MEMBERS, 0);
    if (!sent_members) { throw std::runtime_error("Ошибка получения параметра"); }

    if (*total_members == *sent_members) {
        enable_key(data.chat_id, KeysTablesDefs::RSA);
    }
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

KeysDataBase::KeysDataBase() : DataBase(DB_KEYS) {
    create_keys_tables();
}

