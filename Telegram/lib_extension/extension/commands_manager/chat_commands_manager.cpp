#include "chat_commands_manager.h"
#include <iostream>

ChatCommandsManager::ChatCommandsManager(const std::string& chat_id) {
    chat_id_ = chat_id;
}

std::string ChatCommandsManager::start_rsa(const int key_len) {
    auto [public_key, private_key] = rsa_manager.create_key(key_len);
    
    RsaParamsFiller rsa_params = {
        .chat_id = chat_id_,
        .key_len = 0, // изменить в настройках базы данных
        .total_members = 0, // изменить в настройках данного класса
        .private_key = private_key,
        .initiator = 1
    };
    db_keys.add_rsa_key(rsa_params);

    return "[extension]/add_rsa " + public_key;
}

std::string ChatCommandsManager::start_aes() {
    /* IN PROGRESS... - создание ключа по алгоритму DH */
    return "NULL";
}

std::string ChatCommandsManager::finish_aes() {
    auto aes_key = aes_manager.create_key_solo();

    /* ВРЕМЕННО: для всех чатов создаётся личный ключ (только для себя) */
    AesParamsFiller aes_params = {
        .chat_id = chat_id_,
        .session_key = aes_key,
        .initiator = 1
    };
    db_keys.add_aes_key(aes_params);

    return "[extension]/chat_key_has_been_changed";
}

std::string ChatCommandsManager::stop_encryption() {
    db_keys.disable_other_keys(chat_id_, DbTablesDefs::RSA);
    db_keys.disable_other_keys(chat_id_, DbTablesDefs::AES);
    return "[extension]/chat_key_has_been_removed.";
}
