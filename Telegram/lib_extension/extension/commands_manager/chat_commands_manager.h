#include "../local_storage/keys_db.h"
#include "../keys_manager/keys_manager.h"
#include "message_options.h"

#pragma once

class ChatCommandsManager {
private:
    KeysDataBase db;
    RsaKeyManager rsa_manager;
    AesKeyManager aes_manager;

public:
    std::string start_rsa(const std::string& chat_id_str, const std::string& my_id_str, const int rsa_key_len = 2048, const bool dh_fastmode = true);
    std::vector<std::string> continue_rsa(const std::string& chat_id_str, const std::string& my_id_str, const Message& input_message);
    std::vector<std::string> end_rsa(const std::string& chat_id_str, const std::string& my_id_str, const int rsa_key_n, const bool dh_fastmode);

    std::vector<std::string> start_aes(const std::string& chat_id_str, const std::string& my_id_str, const int rsa_key_n, const bool dh_fastmode);
    std::vector<std::string> continue_aes(const std::string& chat_id_str, const std::string& my_id_str, const Message& input_message);
    std::vector<std::string> end_aes(const std::string& chat_id_str, const std::string& my_id_str, const int aes_key_n, const bool dh_fastmode);

};