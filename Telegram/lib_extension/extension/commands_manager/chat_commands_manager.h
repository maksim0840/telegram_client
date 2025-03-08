#include "../local_storage/keys_db.h"
#include "../keys_manager/keys_manager.h"

#pragma once

class ChatCommandsManager {
private:
    KeysDataBase db_keys;
    RsaKeyManager rsa_manager;
    AesKeyManager aes_manager;
    std::string chat_id_;

public:
    ChatCommandsManager(const std::string& chat_id);

    // Начать процедуру установки rsa шифрования между всеми пользователями + отправить свой публичный ключ
    std::string start_rsa(const int key_len = 2048);

    // in progress...
    std::string start_aes();

    // Завершить процедуру составления aes ключа (ключа шифровки сообщений в чате) и запомнить его в db
    std::string finish_aes();

    // Отключить режим шифрования (чтобы сообщения отправлялись напрямую без шифровки)
    std::string stop_encryption();
};