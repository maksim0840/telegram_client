#include "chat_commands_manager.h"
#include <iostream>

std::string ChatCommandsManager::start_rsa(const std::string& chat_id_str, const int rsa_key_len, const bool dh_fastmode) {
    // Получаем информацию о ключах
    auto [public_key, private_key] = rsa_manager.create_key(rsa_key_len);

    std::optional<int> key_n = db.get_last_key_n(chat_id_str, KeysTablesDefs::RSA);
    std::optional<int> my_id_pos = db.get_active_param_int(chat_id_str, KeysTablesDefs::CHATS, ChatsColumnsDefs::MY_ID_POS);
    key_n = (key_n) ? (*key_n + 1) : 1;
    if (!my_id_pos) { throw std::runtime_error("Ошибка получения параметра"); } 

    // Добавляем ключ в базу
    RsaParamsFiller rsa_params;
    rsa_params.chat_id = chat_id_str;
    rsa_params.key_n = *key_n;
    rsa_params.key_len = rsa_key_len;
    rsa_params.private_key = private_key;
    rsa_params.sent_in_chat = 1;
    db.add_rsa_key(rsa_params, public_key);

    // Выставляем параметры для вывода
    Message message;
    message.rsa_init = true;
    message.rsa_form = true;
    message.dh_fastmode = dh_fastmode;
    message.rsa_key_n = *key_n;
    message.last_peer_n = *my_id_pos;
    message.rsa_key_len = rsa_key_len;
    message.text = public_key;

    return message.get_text_with_options();
}


std::vector<std::string> ChatCommandsManager::continue_rsa(const std::string& chat_id_str, const Message& input_message) {
    std::vector<std::string> output;

    std::optional<int> last_key_n = db.get_last_key_n(chat_id_str, KeysTablesDefs::RSA);
    last_key_n = (last_key_n) ? (*last_key_n) : -1;

    // Если в базе данных отсутсвует новейший полученный номер ключа rsa
    if (input_message.rsa_key_n > *last_key_n) { 
        auto [public_key, private_key] = rsa_manager.create_key(input_message.rsa_key_len);

        // Добавляем свой ключ rsa в базу под этим номером
        RsaParamsFiller rsa_params;
        rsa_params.chat_id = chat_id_str;
        rsa_params.key_n = input_message.rsa_key_n;
        rsa_params.key_len = input_message.rsa_key_len;
        rsa_params.private_key = private_key;
        rsa_params.sent_in_chat = 0;
        db.add_rsa_key(rsa_params, public_key);
    }
    // Если полученная информация о ключе является устаревшей
    else if (input_message.rsa_key_n != *last_key_n) {
        return output;
    }

    std::optional<std::string> members_public_keys = db.get_active_param_text(chat_id_str, input_message.rsa_key_n, KeysTablesDefs::RSA, RsaColumnsDefs::MEMBERS_PUBLIC_KEYS, 0);
    if (!members_public_keys) { return output; }
    std::vector<std::string> members_public_keys_vec = KeysDataBaseHelper::string_to_vector(*members_public_keys);
    
    // Добавляем в базу полученный ключ, если информация о нём отсутсвует в базе
    if (members_public_keys_vec[input_message.last_peer_n] == "") {
        db.add_rsa_member_key(chat_id_str, input_message.text, input_message.last_peer_n);
    }

    std::optional<int> my_id_pos = db.get_active_param_int(chat_id_str, KeysTablesDefs::CHATS, ChatsColumnsDefs::MY_ID_POS);
    std::optional<int> sent_flag = db.get_active_param_int(chat_id_str, KeysTablesDefs::RSA, RsaColumnsDefs::SENT_IN_CHAT, 0);
    std::optional<int> sent_members = db.get_active_param_int(chat_id_str, input_message.rsa_key_n, KeysTablesDefs::RSA, RsaColumnsDefs::SENT_MEMBERS, 0);
    if (!my_id_pos) { throw std::runtime_error("Ошибка получения параметра"); }
    if (!sent_flag) { return output; }
    if (!sent_members) { return output; }

    // Проверяем, дошла ли до нас очередь и не отправляли ли мы свой ключ ранее
    if (((input_message.last_peer_n + 1) % members_public_keys_vec.size() == *my_id_pos) && (*sent_flag == 0)) {

        // Выставляем флаг отправки нашего клоюча
        db.set_sent_flag(chat_id_str, input_message.rsa_key_n, KeysTablesDefs::RSA);

        // Отправляем сообщение с инофрмацией о нашем rsa ключе
        Message output_message;
        output_message.rsa_form = true;
        output_message.rsa_key_n = input_message.rsa_key_n;
        output_message.last_peer_n = *my_id_pos;
        output_message.rsa_key_len = input_message.rsa_key_len;
        output_message.text = members_public_keys_vec[*my_id_pos];

        // Завершаем обмен rsa, если все учатники поделились ключами и выставляем соотвествующие флаги
        if (*sent_members == members_public_keys_vec.size()) {
            output_message.rsa_init = true;
            output_message.rsa_use = true;
            end_rsa(chat_id_str, input_message.rsa_key_n, input_message.dh_fastmode);
        }

        output.push_back(output_message.get_text_with_options());
    }

    return output;
}


std::vector<std::string> ChatCommandsManager::end_rsa(const std::string& chat_id_str, const int rsa_key_n, const bool dh_fastmode) {
    std::vector<std::string> output;

    std::optional<int> status = db.get_active_param_int(chat_id_str, rsa_key_n, KeysTablesDefs::RSA, RsaColumnsDefs::STATUS, 0);
    if (!status) { return output; } 

    // Активируем ключ
    db.enable_key(chat_id_str, KeysTablesDefs::RSA);

    std::optional<int> my_id_pos = db.get_active_param_int(chat_id_str, KeysTablesDefs::CHATS, ChatsColumnsDefs::MY_ID_POS);
    if (!my_id_pos) { throw std::runtime_error("Ошибка получения параметра"); } 

    // Запускаем получение rsa ключа
    if (*my_id_pos == 0) {
        output = {start_aes(chat_id_str, rsa_key_n, dh_fastmode)};
    }

    return output;
}


std::string ChatCommandsManager::start_aes(const std::string& chat_id_str, const int rsa_key_n, const bool dh_fastmode) {

    std::optional<int> members_count = db.get_active_param_int(chat_id_str, KeysTablesDefs::CHATS, ChatsColumnsDefs::MEMBERS_COUNT);
    if (!members_count) { throw std::runtime_error("Ошибка получения параметра"); } 

    if (members_count == 1) {
        // Создать ключ для одного пользователя
        std::string aes_key = create_key_solo();

        // Добавить информацию в базу данных

        Message message;

    
        return завершающее состояние;
    }
   
}


/*


output_message = Message();
output_message.rsa_use = true;
output_message.aes_init = true;
output_message.aes_form = true;
output_message.rsa_key_n = input_message.rsa_key_n;
output_message.aes_key_n = 0; //!~!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
output_message.last_peer_n = *my_id_pos;
output_message.rsa_key_len = input_message.rsa_key_len;
output_message.text = "start aes!!!!!!!!!!!!!!!!!!!!!!!!!!"; //!~!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
output.push_back(QString::fromStdString(output_message.get_text_with_options()));
*/