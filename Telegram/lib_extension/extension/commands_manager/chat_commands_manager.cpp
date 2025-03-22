#include "chat_commands_manager.h"
#include <iostream>

std::string ChatCommandsManager::start_rsa(const std::string& chat_id_str, const std::string& my_id_str, const int rsa_key_len, const bool dh_fastmode) {
    // Деактивируем предыдущий ключ aes шифрования
    db.disable_other_keys(chat_id_str, my_id_str, KeysTablesDefs::AES);
    std::cout << 1 << '\n';
    // Получаем информацию о ключах
    auto [public_key, private_key] = rsa_manager.create_key(rsa_key_len);
    std::optional<int> key_n = db.get_last_key_n(chat_id_str, my_id_str, KeysTablesDefs::RSA);
    std::optional<int> my_id_pos = db.get_active_param_int(chat_id_str, KeysTablesDefs::CHATS, ChatsColumnsDefs::MY_ID_POS);
    key_n = (key_n) ? (*key_n + 1) : 1;
    if (!my_id_pos) { throw std::runtime_error("Ошибка получения параметра"); } 
    std::cout << 2 << '\n';
    // Добавляем ключ в базу
    RsaParamsFiller rsa_params;
    rsa_params.chat_id = chat_id_str;
    rsa_params.my_id = my_id_str;
    rsa_params.key_n = *key_n;
    rsa_params.key_len = rsa_key_len;
    rsa_params.private_key = private_key;
    rsa_params.sent_in_chat = 1;
    db.add_rsa_key(rsa_params, public_key);
    std::cout << 3 << '\n';
    // Выставляем параметры для вывода
    Message message;
    message.rsa_init = true;
    message.rsa_form = true;
    message.dh_fastmode = dh_fastmode;
    message.rsa_key_n = *key_n;
    message.last_peer_n = *my_id_pos;
    message.rsa_key_len = rsa_key_len;
    message.text = public_key;
    std::cout << 4 << '\n';
    
    std::optional<int> members_count = db.get_active_param_int(chat_id_str, KeysTablesDefs::CHATS, ChatsColumnsDefs::MEMBERS_COUNT);
    if (!members_count) { throw std::runtime_error("Ошибка получения параметра"); } 
    if (*members_count == 1) {
        return start_aes(chat_id_str, my_id_str, message.rsa_key_n, message.dh_fastmode)[0];
    }

    return message.get_text_with_options();
}


std::vector<std::string> ChatCommandsManager::continue_rsa(const std::string& chat_id_str, const std::string& my_id_str, const Message& input_message) {
    std::vector<std::string> output;
    std::cout << 5 << '\n';
    std::optional<int> last_key_n = db.get_last_key_n(chat_id_str, my_id_str, KeysTablesDefs::RSA);
    last_key_n = (last_key_n) ? (*last_key_n) : -1;

    // Если в базе данных отсутсвует новейший полученный номер ключа rsa
    if (input_message.rsa_key_n > *last_key_n) { 
        std::cout << 6 << '\n';
        // Деактивируем предыдущий ключ aes шифрования
        db.disable_other_keys(chat_id_str, my_id_str, KeysTablesDefs::AES);

        auto [public_key, private_key] = rsa_manager.create_key(input_message.rsa_key_len);

        // Добавляем свой ключ rsa в базу под этим же номером
        RsaParamsFiller rsa_params;
        rsa_params.chat_id = chat_id_str;
        rsa_params.my_id = my_id_str;
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
    std::cout << 7 << '\n';
    std::optional<std::string> members_public_keys = db.get_active_param_text(chat_id_str, my_id_str, input_message.rsa_key_n, KeysTablesDefs::RSA, RsaColumnsDefs::MEMBERS_PUBLIC_KEYS, 0);
    if (!members_public_keys) { return output; }
    std::vector<std::string> members_public_keys_vec = KeysDataBaseHelper::string_to_vector(*members_public_keys);

    // Добавляем в базу полученный ключ, если информация о нём отсутсвует в базе
    if (members_public_keys_vec[input_message.last_peer_n] == "") {
        db.add_rsa_member_key(chat_id_str, my_id_str, input_message.rsa_key_n, input_message.text, input_message.last_peer_n);
    }
    std::cout << 8 << '\n';
    std::optional<int> my_id_pos = db.get_active_param_int(chat_id_str, KeysTablesDefs::CHATS, ChatsColumnsDefs::MY_ID_POS);
    std::optional<int> sent_flag = db.get_active_param_int(chat_id_str, KeysTablesDefs::RSA, RsaColumnsDefs::SENT_IN_CHAT, 0);
    std::optional<int> sent_members = db.get_active_param_int(chat_id_str, my_id_str, input_message.rsa_key_n, KeysTablesDefs::RSA, RsaColumnsDefs::SENT_MEMBERS, 0);
    if (!my_id_pos) { throw std::runtime_error("Ошибка получения параметра"); }
    if (!sent_flag) { return output; }
    if (!sent_members) { return output; }

    // Проверяем, дошла ли до нас очередь и не отправляли ли мы свой ключ ранее
    if (((input_message.last_peer_n + 1) % members_public_keys_vec.size() == *my_id_pos) && (*sent_flag == 0)) {
        std::cout << 9 << '\n';
        // Выставляем флаг отправки нашего клоюча
        db.set_rsa_sent_flag(chat_id_str, my_id_str, input_message.rsa_key_n);

        // Отправляем сообщение с инофрмацией о нашем rsa ключе
        Message output_message;
        output_message.rsa_form = true;
        output_message.rsa_key_n = input_message.rsa_key_n;
        output_message.last_peer_n = *my_id_pos;
        output_message.rsa_key_len = input_message.rsa_key_len;
        output_message.text = members_public_keys_vec[*my_id_pos];

        // Завершаем обмен rsa, если все учатники поделились ключами и выставляем соотвествующие флаги
        if (*sent_members == members_public_keys_vec.size()) {
            std::cout << 10 << '\n';
            output_message.rsa_init = true;
            output_message.rsa_use = true;
            output.push_back(output_message.get_text_with_options());
            std::vector<std::string> end_rsa_output = end_rsa(chat_id_str, my_id_str, input_message.rsa_key_n, input_message.dh_fastmode);
            output.insert(output.end(), end_rsa_output.begin(), end_rsa_output.end());
            return output;
        }
        std::cout << 11 << '\n';
        output.push_back(output_message.get_text_with_options());
    }
    std::cout << 12 << '\n';
    return output;
}


std::vector<std::string> ChatCommandsManager::end_rsa(const std::string& chat_id_str, const std::string& my_id_str, const int rsa_key_n, const bool dh_fastmode) {
    std::vector<std::string> output;
    std::cout << 13 << '\n';
    // Проверяем, был ли активирован ключ до этого
    std::optional<int> status = db.get_active_param_int(chat_id_str, my_id_str, rsa_key_n, KeysTablesDefs::RSA, RsaColumnsDefs::STATUS, 0);
    if (!status) { return output; } 

    // Активируем ключ
    db.enable_key(chat_id_str, my_id_str, rsa_key_n, KeysTablesDefs::RSA);
    std::cout << 14 << '\n';
    std::optional<int> my_id_pos = db.get_active_param_int(chat_id_str, KeysTablesDefs::CHATS, ChatsColumnsDefs::MY_ID_POS);
    if (!my_id_pos) { throw std::runtime_error("Ошибка получения параметра"); } 

    // Запускаем получение rsa ключа
    if (*my_id_pos == 0) {
        std::cout << 15 << '\n';
        output = start_aes(chat_id_str, my_id_str, rsa_key_n, dh_fastmode);
    }
    std::cout << 16 << '\n';
    return output;
}


std::vector<std::string> ChatCommandsManager::start_aes(const std::string& chat_id_str, const std::string& my_id_str, const int rsa_key_n, const bool dh_fastmode) {
    std::vector<std::string> output;
    std::cout << 17 << '\n';
    std::optional<int> my_id_pos = db.get_active_param_int(chat_id_str, KeysTablesDefs::CHATS, ChatsColumnsDefs::MY_ID_POS);
    std::optional<int> members_count = db.get_active_param_int(chat_id_str, KeysTablesDefs::CHATS, ChatsColumnsDefs::MEMBERS_COUNT);
    std::optional<int> aes_key_n = db.get_last_key_n(chat_id_str, my_id_str, KeysTablesDefs::AES);
    if (!my_id_pos) { throw std::runtime_error("Ошибка получения параметра"); }
    if (!members_count) { throw std::runtime_error("Ошибка получения параметра"); } 
    aes_key_n = (aes_key_n) ? (*aes_key_n + 1) : 1;

    if (members_count == 1) {
        std::cout << 18 << '\n';
        // Создать ключ для одного пользователя
        std::string aes_key = aes_manager.create_key_solo();

        // Добавить ключ в базу данных
        AesParamsFiller aes_params;
        aes_params.chat_id = chat_id_str;
        aes_params.my_id = my_id_str;
        aes_params.key_n = *aes_key_n;
        aes_params.session_key = aes_key;
        db.add_aes_key(aes_params);

        // Запоминаем, что ключ был создан
        db.add_aes_sent_in_chat(chat_id_str, my_id_str, *aes_key_n, *my_id_pos);

        return end_aes(chat_id_str, my_id_str, *aes_key_n, dh_fastmode);
    }
    std::cout << 19 << '\n';
    // Создаём параметры шифрования для генерации ключа между несколькими собеседниками
    DHParamsStr dh_params = aes_manager.get_dh_params(dh_fastmode);

    // Добавить информацию о параметрах в базу данных
    AesParamsFiller aes_params;
    aes_params.chat_id = chat_id_str;
    aes_params.my_id = my_id_str;
    aes_params.key_n = *aes_key_n;
    aes_params.p = dh_params.p;
    aes_params.g = dh_params.g;
    aes_params.public_key = dh_params.public_key;
    aes_params.private_key = dh_params.private_key;
    db.add_aes_key(aes_params);
    std::cout << 20 << '\n';
    std::optional<int> rsa_key_len = db.get_active_param_int(chat_id_str, my_id_str, rsa_key_n, KeysTablesDefs::RSA, RsaColumnsDefs::KEY_LEN);
    if (!rsa_key_len) { return output; }

    std::vector<std::string> shared_params = {dh_params.p, dh_params.g};

    // Каждому собеседнику, кроме себя отправить параметры p и g
    for (int i = 0; i < *members_count; ++i) {
        if (i == *my_id_pos) {
            continue;
        }
        std::cout << 21 << '\n';

        // Отправить сообщение
        Message output_message;
        output_message.rsa_use = true;
        output_message.aes_init = true;
        output_message.aes_form = true;
        output_message.dh_fastmode = dh_fastmode;
        output_message.rsa_key_n = rsa_key_n;
        output_message.aes_key_n = *aes_key_n;
        output_message.last_peer_n = i;
        output_message.rsa_key_len = *rsa_key_len;
        output_message.text = KeysDataBaseHelper::vector_to_string(shared_params);

        output.push_back(output_message.get_text_with_options());
    }
    std::cout << 22 << '\n';

    // Начинаем формировать ключ
    Message output_message;
    output_message.rsa_use = true;
    output_message.aes_form = true;
    output_message.dh_fastmode = dh_fastmode;
    output_message.rsa_key_n = rsa_key_n;
    output_message.aes_key_n = *aes_key_n;
    output_message.last_peer_n = ((*my_id_pos - 1) % *members_count + *members_count) % *members_count; // <=> (*my_id_pos - 1) mod *members_count
    output_message.rsa_key_len = *rsa_key_len;
    output_message.text = dh_params.public_key;
    output.push_back(output_message.get_text_with_options());
    std::cout << 22.5 << ' ' << output_message.last_peer_n << '\n';
    // Запоминаем, что ключ для конкретного пользователя был отправлен
    db.add_aes_sent_in_chat(chat_id_str, my_id_str, *aes_key_n, output_message.last_peer_n);
    std::cout << 23 << '\n';

    return output;
}


std::vector<std::string> ChatCommandsManager::continue_aes(const std::string& chat_id_str, const std::string& my_id_str, const Message& input_message) {
    std::vector<std::string> output;
    std::cout << 24 << '\n';

    std::optional<int> my_id_pos = db.get_active_param_int(chat_id_str, KeysTablesDefs::CHATS, ChatsColumnsDefs::MY_ID_POS);
    std::optional<int> members_count = db.get_active_param_int(chat_id_str, KeysTablesDefs::CHATS, ChatsColumnsDefs::MEMBERS_COUNT);
    std::optional<std::string> sent_in_chat = db.get_active_param_text(chat_id_str, my_id_str, input_message.aes_key_n, KeysTablesDefs::AES, AesColumnsDefs::SENT_IN_CHAT, 0);
    if (!my_id_pos) { throw std::runtime_error("Ошибка получения параметра"); }
    if (!members_count) { throw std::runtime_error("Ошибка получения параметра"); }

    std::cout << 25 << '\n';

    // Признак необходимости принять параметры p и g
    if (!sent_in_chat && input_message.aes_init && input_message.aes_form) {
        // Получаем {public_key, private_key}
        std::vector<std::string> shared_params = KeysDataBaseHelper::string_to_vector(input_message.text); 

        // Генерируем свои параметры на основе полученных
        DHParamsStr dh_params = aes_manager.get_dh_params_secondly(shared_params[0], shared_params[1]);
        std::cout << 26 << '\n';

        // Добавить информацию о своих ключах в базу данных
        AesParamsFiller aes_params;
        aes_params.chat_id = chat_id_str;
        aes_params.my_id = my_id_str;
        aes_params.key_n = input_message.aes_key_n;
        aes_params.p = dh_params.p;
        aes_params.g = dh_params.g;
        aes_params.public_key = dh_params.public_key;
        aes_params.private_key = dh_params.private_key;
        db.add_aes_key(aes_params);

        sent_in_chat = input_message.text;
    }
    else if (!sent_in_chat) { throw std::runtime_error("Ошибка получения параметра"); }
    std::cout << 27 << '\n';

    // Если сообщение уже было отправленно, то игнорируем вызов
    std::vector<std::string> sent_in_chat_vec = KeysDataBaseHelper::string_to_vector(*sent_in_chat);
    if (sent_in_chat_vec[input_message.last_peer_n] != "") {
        return output;
    }

    // Получаем параметры dh
    std::optional<std::string> p = db.get_active_param_text(chat_id_str, my_id_str, input_message.aes_key_n, KeysTablesDefs::AES, AesColumnsDefs::P, 0);
    std::optional<std::string> g = db.get_active_param_text(chat_id_str, my_id_str, input_message.aes_key_n, KeysTablesDefs::AES, AesColumnsDefs::G, 0);
    std::optional<std::string> public_key = db.get_active_param_text(chat_id_str, my_id_str, input_message.aes_key_n, KeysTablesDefs::AES, AesColumnsDefs::PUBLIC_KEY, 0);
    std::optional<std::string> private_key = db.get_active_param_text(chat_id_str, my_id_str, input_message.aes_key_n, KeysTablesDefs::AES, AesColumnsDefs::PRIVATE_KEY, 0);
    if (!p) { throw std::runtime_error("Ошибка получения параметра"); }
    if (!g) { throw std::runtime_error("Ошибка получения параметра"); }
    if (!public_key) { throw std::runtime_error("Ошибка получения параметра"); }
    if (!private_key) { throw std::runtime_error("Ошибка получения параметра"); }
    DHParamsStr dh_params;
    dh_params.p = *p;
    dh_params.g = *g;
    dh_params.public_key = *public_key;
    dh_params.private_key = *private_key;
    std::cout << 28 << '\n';

    // Получаем итоговый результат вычисления shared_key
    if (input_message.last_peer_n == *my_id_pos) {
        
        // Возводим передаваемый ключ в свою степень
        std::string session_key = aes_manager.сreate_key_multi(dh_params, input_message.text, true);

        // Добавляем ключ в базу
        db.add_aes_session_key(chat_id_str, my_id_str, input_message.aes_key_n, session_key);
        std::cout << 29 << '\n';

        // Отправляем следующий запрос на формирование ключей дальше по цепочке
        Message output_message = input_message;
        output_message.last_peer_n = ((*my_id_pos - 1) % *members_count + *members_count) % *members_count; // <=> (*my_id_pos - 1) mod *members_count
        output_message.text = dh_params.public_key;
        output.push_back(output_message.get_text_with_options());
        std::cout << 30 << '\n';

        // Запоминаем, что ключ для конкретного пользователя был отправлен
        db.add_aes_sent_in_chat(chat_id_str, my_id_str, output_message.aes_key_n, output_message.last_peer_n);

        // Если мы создали ключ для самого первого пользователя => ключи создали для всех
        if (*my_id_pos == 0) {
            return end_aes(chat_id_str, my_id_str, input_message.aes_key_n, input_message.dh_fastmode);
        }
        // Иначе, продолжаем цепочку
        return output;
    }

    // Ключ не итоговый => домнажаем на свои параметры и передаём ключ дальше
    std::string session_key_continuation = aes_manager.сreate_key_multi(dh_params, input_message.text, false);
    Message output_message = input_message;
    output_message.text = session_key_continuation;
    output.push_back(output_message.get_text_with_options());
    std::cout << 31 << '\n';

    // Запоминаем, что ключ для конкретного пользователя был отправлен
    db.add_aes_sent_in_chat(chat_id_str, my_id_str, output_message.aes_key_n, output_message.last_peer_n);

    return output;
}   


std::vector<std::string> ChatCommandsManager::end_aes(const std::string& chat_id_str, const std::string& my_id_str, const int aes_key_n, const bool dh_fastmode) {
    std::vector<std::string> output;

    // Проверяем, был ли активирован ключ до этого
    std::optional<int> status = db.get_active_param_int(chat_id_str, my_id_str, aes_key_n, KeysTablesDefs::AES, RsaColumnsDefs::STATUS, 0);
    if (!status) { return output; } 
    std::cout << 32 << '\n';

    // Активируем ключ aes
    db.enable_key(chat_id_str, my_id_str, aes_key_n, KeysTablesDefs::AES);

    // Деактивируем ключ rsa
    db.disable_other_keys(chat_id_str, my_id_str, KeysTablesDefs::RSA);
    std::cout << 33 << '\n';

    std::optional<int> my_id_pos = db.get_active_param_int(chat_id_str, KeysTablesDefs::CHATS, ChatsColumnsDefs::MY_ID_POS);
    if (!my_id_pos) { throw std::runtime_error("Ошибка получения параметра"); }

    // Если мы последний, кто получил ключ - объявляем конец в чате
    if (*my_id_pos == 0) {
        Message output_message;
        output_message.aes_init = true;
        output_message.aes_form = true;
        output_message.aes_use = true;
        output_message.aes_key_n = aes_key_n;
        output_message.last_peer_n = *my_id_pos;
        output_message.text = "aes encryption started with key (" + std::to_string(aes_key_n) + ")";
        output.push_back(output_message.get_text_with_options());
    }
    std::cout << 34 << '\n';

    return output;
}