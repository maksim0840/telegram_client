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

std::optional<int> KeysDataBase::get_last_key_n(const std::string& chat_id, const std::string& my_id, const KeysTablesDefs table) {
    const std::string sql_request = "SELECT MAX(key_n) FROM " + KeysTablesUndefs.at(table) + " WHERE chat_id = ? AND my_id = ?;";
    Statement stmt(db, sql_request);
    stmt.bind_text(1, chat_id);
    stmt.bind_text(2, my_id);
    return stmt.execute_int(0);
}

void KeysDataBase::disable_other_keys(const std::string& chat_id, const std::string& my_id, const KeysTablesDefs table) {
    const std::string sql_request = "UPDATE " + KeysTablesUndefs.at(table) + " SET status = -1 WHERE (status = 1 OR status = 0) AND chat_id = ? AND my_id = ?;";
    Statement stmt(db, sql_request);
    stmt.bind_text(1, chat_id);
    stmt.bind_text(2, my_id);
    stmt.execute();
}

void KeysDataBase::disable_other_keys(const std::string& chat_id, const int my_id_pos, const KeysTablesDefs table) {
    const std::string sql_request = "UPDATE " + KeysTablesUndefs.at(table) + " SET status = -1 WHERE (status = 1 OR status = 0) AND chat_id = ? AND my_id_pos = ?;";
    Statement stmt(db, sql_request);
    stmt.bind_text(1, chat_id);
    stmt.bind_int(2, my_id_pos);
    stmt.execute();
}

void KeysDataBase::enable_key(const std::string& chat_id, const std::string& my_id, const int key_n, const KeysTablesDefs table) {
    const std::string sql_request = "UPDATE " + KeysTablesUndefs.at(table) + " SET status = 1 WHERE status = 0 AND chat_id = ? AND my_id = ? AND key_n = ?;";
    Statement stmt(db, sql_request);
    stmt.bind_text(1, chat_id);
    stmt.bind_text(2, my_id);
    stmt.bind_int(3, key_n);
    stmt.execute();
}

void KeysDataBase::add_aes_key(const AesParamsFiller& data) {
    const std::string sql_request = "INSERT INTO aes (chat_id, my_id, key_n, session_key, p, g, public_key, private_key, date, messages, status, sent_in_chat, sent_members) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    std::string date = KeysDataBaseHelper::get_system_time();

    std::optional<int> members_count = get_active_param_int(data.chat_id, KeysTablesDefs::CHATS, ChatsColumnsDefs::MEMBERS_COUNT);
    if (!members_count) { throw std::runtime_error("Ошибка получения параметра"); }

    std::vector<std::string> sent_in_chat_vec(*members_count, "");
    std::string sent_in_chat = KeysDataBaseHelper::vector_to_string(sent_in_chat_vec);

    disable_other_keys(data.chat_id, data.my_id, KeysTablesDefs::AES); // сбросить флаги "используется" у всех aes ключей этого чата

    Statement stmt(db, sql_request);

    stmt.bind_text(1, data.chat_id);
    stmt.bind_text(2, data.my_id);
    stmt.bind_int(3, data.key_n);
    stmt.bind_text(4, data.session_key);
    stmt.bind_text(5, data.p);
    stmt.bind_text(6, data.g);
    stmt.bind_text(7, data.public_key);
    stmt.bind_text(8, data.private_key);
    stmt.bind_text(9, date);
    stmt.bind_int(10, 0); // messages = 0   
    stmt.bind_int(11, data.status);
    stmt.bind_text(12, sent_in_chat);
    stmt.bind_int(13, 1); // sent_members = 1

    stmt.execute();
}

void KeysDataBase::add_rsa_key(const RsaParamsFiller& data, const std::string& my_public_key) {
    const std::string sql_request = "INSERT INTO rsa (chat_id, my_id, key_n, key_len, sent_members, private_key, members_public_keys, date, status, sent_in_chat) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    std::string date = KeysDataBaseHelper::get_system_time();

    std::optional<int> members_count = get_active_param_int(data.chat_id, KeysTablesDefs::CHATS, ChatsColumnsDefs::MEMBERS_COUNT);
    if (!members_count) { throw std::runtime_error("Ошибка получения параметра"); }
    std::optional<int> my_id_pos = get_active_param_int(data.chat_id, KeysTablesDefs::CHATS, ChatsColumnsDefs::MY_ID_POS);
    if (!members_count) { throw std::runtime_error("Ошибка получения параметра"); }

    std::vector<std::string> members_public_keys_vec(*members_count, "");
    members_public_keys_vec[*my_id_pos] = my_public_key;
    std::string members_public_keys = KeysDataBaseHelper::vector_to_string(members_public_keys_vec);

    disable_other_keys(data.chat_id, data.my_id, KeysTablesDefs::RSA); // сбросить флаги "используется" у всех rsa ключей этого чата
    
    Statement stmt(db, sql_request);

    stmt.bind_text(1, data.chat_id);
    stmt.bind_text(2, data.my_id);
    stmt.bind_int(3, data.key_n);
    stmt.bind_int(4, data.key_len);
    stmt.bind_int(5, 1); // sent_members = 1
    stmt.bind_text(6, data.private_key);
    stmt.bind_text(7, members_public_keys);
    stmt.bind_text(8, date);
    stmt.bind_int(9, 0); // status = 0
    stmt.bind_int(10, data.sent_in_chat); // status = 0
    stmt.execute();
}

void KeysDataBase::add_chat_params(const ChatsParamsFiller& data) {
    const std::string sql_request = "INSERT INTO chats (chat_id, my_id_pos, members_count, members_ids, date, status) VALUES (?, ?, ?, ?, ?, ?);";
    std::string date = KeysDataBaseHelper::get_system_time();
    std::string members_ids = KeysDataBaseHelper::vector_to_string(data.members_ids);

    disable_other_keys(data.chat_id, data.my_id_pos, KeysTablesDefs::CHATS); // сбросить флаги "используется" у предыдущих записей

    Statement stmt(db, sql_request);

    stmt.bind_text(1, data.chat_id);
    stmt.bind_int(2, data.my_id_pos);
    stmt.bind_int(3, data.members_ids.size()); // members_count
    stmt.bind_text(4, members_ids);
    stmt.bind_text(5, date);
    stmt.bind_int(6, 1); // status = 1
    stmt.execute();
}

void KeysDataBase::set_rsa_sent_flag(const std::string& chat_id, const std::string& my_id, const int key_n) {
    const std::string sql_request = "UPDATE rsa SET sent_in_chat = 1 WHERE chat_id = ? AND my_id = ? AND key_n = ?;";
    Statement stmt(db, sql_request);
    stmt.bind_text(1, chat_id);
    stmt.bind_text(2, my_id);
    stmt.bind_int(3, key_n);
    stmt.execute();
}

void KeysDataBase::add_rsa_member_key(const std::string& chat_id, const std::string& my_id, const int key_n, const std::string& key, const int pos) {
    // Извлекаем существующие списоки параметров
    std::optional<std::string> members_public_keys = get_active_param_text(chat_id, KeysTablesDefs::RSA, RsaColumnsDefs::MEMBERS_PUBLIC_KEYS, 0);
    if (!members_public_keys) { throw std::runtime_error("Ошибка получения параметра"); }

    // Дополняем извлеченные значения новым
    std::vector<std::string> members_public_keys_vec = KeysDataBaseHelper::string_to_vector(*members_public_keys);
    if (pos >= members_public_keys_vec.size()) { throw std::runtime_error("Выход за границы массива"); }
    members_public_keys_vec[pos] = key;
    members_public_keys = KeysDataBaseHelper::vector_to_string(members_public_keys_vec);

    // Записываем изменённые значения
    const std::string sql_request = "UPDATE rsa SET sent_members = sent_members + 1, members_public_keys = ? WHERE chat_id = ? AND my_id = ? AND key_n = ? AND status = 0;";
    Statement stmt(db, sql_request);
    stmt.bind_text(1, *members_public_keys);
    stmt.bind_text(2, chat_id);
    stmt.bind_text(3, my_id);
    stmt.bind_int(4, key_n);
    stmt.execute();
}

void KeysDataBase::add_aes_sent_in_chat(const std::string& chat_id, const std::string& my_id, const int key_n, const int pos) {
    // Извлекаем существующие списоки параметров
    std::optional<std::string> sent_in_chat = get_active_param_text(chat_id, KeysTablesDefs::AES, AesColumnsDefs::SENT_IN_CHAT, 0);
    if (!sent_in_chat) { throw std::runtime_error("Ошибка получения параметра"); }

    // Дополняем извлеченные значения новым
    std::vector<std::string> sent_in_chat_vec = KeysDataBaseHelper::string_to_vector(*sent_in_chat);
    if (pos >= sent_in_chat_vec.size()) { throw std::runtime_error("Выход за границы массива"); }
    sent_in_chat_vec[pos] = "1";
    sent_in_chat = KeysDataBaseHelper::vector_to_string(sent_in_chat_vec);
 

    // Записываем изменённые значения
    const std::string sql_request = "UPDATE aes SET sent_members = sent_members + 1, sent_in_chat = ? WHERE chat_id = ? AND my_id = ? AND key_n = ? AND status = 0;";
    Statement stmt(db, sql_request);
    stmt.bind_text(1, *sent_in_chat);
    stmt.bind_text(2, chat_id);
    stmt.bind_text(3, my_id);
    stmt.bind_int(4, key_n);
    stmt.execute();
}

void KeysDataBase::add_aes_session_key(const std::string& chat_id, const std::string& my_id, const int key_n, const std::string& session_key) {
    const std::string sql_request = "UPDATE aes SET session_key = ? WHERE chat_id = ? AND my_id = ? AND key_n = ? AND status = 0;";
    Statement stmt(db, sql_request);
    stmt.bind_text(1, session_key);
    stmt.bind_text(2, chat_id);
    stmt.bind_text(3, my_id);
    stmt.bind_int(4, key_n);
    stmt.execute();
}

void KeysDataBase::increase_aes_messages_counter(const std::string& chat_id, const std::string& my_id, const int key_n) {
    // Увеличиваем количество сообщений в таблице aes
    const std::string sql_request_aes = "UPDATE aes SET messages = messages + 1 WHERE chat_id = ? AND my_id = ? AND key_n = ? AND status = 1;";
    Statement stmt(db, sql_request_aes);
    stmt.bind_text(1, chat_id);
    stmt.bind_text(2, my_id);
    stmt.bind_int(3, key_n);
    stmt.execute();
}

KeysDataBase::KeysDataBase() : DataBase(DB_KEYS) {
    create_keys_tables();
}

