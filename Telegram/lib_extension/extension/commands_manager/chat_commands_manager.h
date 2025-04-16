#include "../local_storage/keys_db.h"
#include "../keys_manager/keys_manager.h"
#include "../chat_members/chat_members.h"
#include "message_options.h"
#include <QtGlobal>
#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <thread>

#define MAX_PEERS_IN_CHAT 100 // для удобства (можно спокойно увеличивать данное значение если требуется)
#define SENDING_DELAY 500 // задержка при отправке в милисекундах (лучше не менять т.к. часто банит)

#pragma once

namespace ext {

enum class KeyCreationStages {
    INIT_RSA_ENCRYPTION,
    RSA_SEND_PUBLIC_KEY,
    INIT_AES_ENCRYPTION,
    AES_FORM_SESSION_KEY,
    END_KEY_FORMING
};

class ChatCommandsManager {
private:
    //KeysDataBase db; //- не можеть быть статическим, поэтому не объявлять
    RsaKeyManager rsa_manager;
    AesKeyManager aes_manager;

    std::function<void(const std::string&)> lambda_send_message; // лямбда функция отправки сообщения (передаётся в chat_key_creation.cpp)
    std::function<void(quint64&, quint64&, std::vector<quint64>&)> lambda_get_chat_ids; // лямбда функция получения id собеседников (передаётся в chat_key_creation.cpp)

    std::string my_id_str;
    std::string chat_id_str;
    std::vector<std::string> chat_members_str;
    int my_id_pos; // индекс элемента из chat_members_str, который равен my_id_str
    int members_len; // размер chat_members_str

    // результат выполнения функций
    std::vector<std::string> members_rsa_public_key;
    std::string my_rsa_private_key;
    DHParamsStr my_dh_params;
    std::string aes_key;

public:
    // setter-ы
    void set_lambda_functions(
        const std::function<void(const std::string&)>& send_func,
        const std::function<void(quint64&, quint64&, std::vector<quint64>&)>& get_id_func);
    void set_chat_members_info();
    void reset_functions_results();

    // Функции создания ключей
    KeyCreationStages init_rsa_encryption(Message& rcv_msg);
    KeyCreationStages rsa_send_public_key(Message& rcv_msg);
    KeyCreationStages init_aes_encryption(Message& rcv_msg);
    KeyCreationStages aes_form_session_key(Message& rcv_msg, std::string snd_id_str);
    void end_key_forming(Message& rcv_msg);

    // Функции добавления в базу данных
    void db_add_aes(const int aes_key_n);
    void db_add_rsa(const int rsa_key_n, const int rsa_key_len);
    
    void end_encryption(bool notify_others = false);
};

} // namespace ext