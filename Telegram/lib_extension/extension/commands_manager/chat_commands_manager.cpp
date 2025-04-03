#include "chat_commands_manager.h"


void ChatCommandsManager::set_lambda_functions(
    const std::function<void(const std::string&)>& send_func,
    const std::function<void(quint64&, quint64&, std::vector<quint64>&)>& get_id_func) {
        
    lambda_send_message = send_func;
    lambda_get_chat_ids = get_id_func;
}


void ChatCommandsManager::set_chat_members_info() {
    // Получаем id-шники
    quint64 my_id;
    quint64 chat_id;
    std::vector<quint64> chat_members;
    lambda_get_chat_ids(my_id, chat_id, chat_members);

    // Записываем в базу и конвертируем в строки
    std::tie(my_id_str, chat_id_str, chat_members_str, my_id_pos) = update_chat_members(my_id, chat_id, chat_members);
    
    // Записываем количество участников чата
    members_len = chat_members.size();
    if (members_len >= MAX_PEERS_IN_CHAT) {
        throw std::runtime_error("Превышен лимит пользователей чата (если это необходимо, то измените его в chat_commands_manager.h)");
    }
}


void ChatCommandsManager::reset_functions_results() {
    members_rsa_public_key = std::vector<std::string>(members_len, "");
    my_rsa_private_key = "";
    my_dh_params = DHParamsStr();
    aes_key = "";
}


KeyCreationStages ChatCommandsManager::init_rsa_encryption(Message& rcv_msg) {
    KeysDataBase db;
    // Получаем последние номера уже созданных ключей и увеличиваем их на 1
    std::optional<int> rsa_key_n = db.get_last_key_n(chat_id_str, my_id_str, KeysTablesDefs::RSA);
    std::optional<int> aes_key_n = db.get_last_key_n(chat_id_str, my_id_str, KeysTablesDefs::AES);
    rsa_key_n = (rsa_key_n) ? (*rsa_key_n + 1) : 1;
    aes_key_n = (aes_key_n) ? (*aes_key_n + 1) : 1;

    // Формируем сообщение для отправки
    Message message_to_send;
    message_to_send.rsa_init = true;
    message_to_send.dh_fastmode = true;
    message_to_send.rsa_key_n = *rsa_key_n;
    message_to_send.aes_key_n = *aes_key_n;
    message_to_send.last_peer_n = my_id_pos; // свой id (тот, кто инициализирует rsa шифрование)
    message_to_send.rsa_key_len = 2048;
    message_to_send.text = "start encryption key forming";

    // Отправляем сообщение
    std::this_thread::sleep_for(std::chrono::milliseconds(SENDING_DELAY));
    lambda_send_message(message_to_send.get_text_with_options());

    // Получаем своё же сообщение
    rcv_msg = message_to_send;

    // Переходим на следующую стадию
    return KeyCreationStages::RSA_SEND_PUBLIC_KEY;
}


KeyCreationStages ChatCommandsManager::rsa_send_public_key(Message& rcv_msg) {
    if ((my_id_pos == 0 && rcv_msg.rsa_init == true) || // если мы первый в списке и не отправляли свой ключ (начинаем)
        (my_id_pos == (rcv_msg.last_peer_n + 1) && rcv_msg.rsa_init == false)) { // если мы следующий, кто должен отправлять и первый уже отправил

        // Сохраняем полученный ключ от предыдущего отправившего
        if (rcv_msg.rsa_init == false) { // предыдущее сообщение содержит публичный ключ
            members_rsa_public_key[rcv_msg.last_peer_n] = rcv_msg.text;
        }

        // Создаём rsa ключ
        auto [public_key, private_key] = rsa_manager.create_key(rcv_msg.rsa_key_len);
        
        // Сохраняем ключи
        members_rsa_public_key[my_id_pos] = public_key;
        my_rsa_private_key = private_key;
        
        // Отправляем свой ключ в чат
        Message message_to_send;
        message_to_send.rsa_form = true;
        message_to_send.dh_fastmode = rcv_msg.dh_fastmode;
        message_to_send.rsa_key_n = rcv_msg.rsa_key_n;
        message_to_send.aes_key_n = rcv_msg.aes_key_n;
        message_to_send.last_peer_n = my_id_pos; // свой id (тот, кто отправляет публичный ключ)
        message_to_send.rsa_key_len = rcv_msg.rsa_key_len;
        message_to_send.text = public_key; // передаём свой публичный ключ в чат
 
        // Отправляем сообщение
        std::this_thread::sleep_for(std::chrono::milliseconds(SENDING_DELAY));
        lambda_send_message(message_to_send.get_text_with_options());

        // Переходим на следующиую стадию, если мы были последним
        if (my_id_pos == members_len - 1) {
            return KeyCreationStages::INIT_AES_ENCRYPTION;
        }
    }
    else if (rcv_msg.rsa_init == false) {
        // Сохраняем полученный ключ от предыдущего отправившего
        members_rsa_public_key[rcv_msg.last_peer_n] = rcv_msg.text;

        // Переходим на следующиую стадию, если прочитали сообщение последнего
        if (rcv_msg.last_peer_n == members_len - 1) {
            return KeyCreationStages::INIT_AES_ENCRYPTION;
        }
    }

    // Иначе, остаёмся на своей стадии
    return KeyCreationStages::RSA_SEND_PUBLIC_KEY;
}


KeyCreationStages ChatCommandsManager::init_aes_encryption(Message& rcv_msg) {
    if (my_id_pos == 0) { // если мы первый

        // Создаём и запоминаем свои параметры aes шифрования
        DHParamsStr dh_params = aes_manager.get_dh_params();
        my_dh_params = dh_params;
        std::vector<std::string> shared_params = {dh_params.p, dh_params.g};

        // Собираем сообщение для отправки параметров P и G
        Message message_to_send;
        message_to_send.rsa_use = true;
        message_to_send.aes_init = true;
        message_to_send.dh_fastmode = rcv_msg.dh_fastmode;
        message_to_send.rsa_key_n = rcv_msg.rsa_key_n;
        message_to_send.aes_key_n = rcv_msg.aes_key_n;
        message_to_send.last_peer_n = my_id_pos; // свой id (тот, кто отправляет параметры P и G)
        message_to_send.rsa_key_len = rcv_msg.rsa_key_len;
        message_to_send.text = KeysDataBaseHelper::vector_to_string(shared_params); // отправляем параметры p и g
        
        // Отправляем сообщение
        std::this_thread::sleep_for(std::chrono::milliseconds(SENDING_DELAY));
        lambda_send_message(message_to_send.get_text_with_options());

        // Получаем своё же сообщение
        rcv_msg = message_to_send;

        // Переходим на следующиую стадию
        return KeyCreationStages::AES_FORM_SESSION_KEY;

    }
    else if (rcv_msg.aes_init == true) {
        // Получаем параметры P и G от первого пользователя
        std::vector<std::string> shared_params = KeysDataBaseHelper::string_to_vector(rcv_msg.text);

        // Создаём и запоминаем свои параметры aes шифрования (используя уже сфорированные P и G)
        DHParamsStr dh_params = aes_manager.get_dh_params_secondly(shared_params[0], shared_params[1]);
        my_dh_params = dh_params;

        // Переходим на следующиую стадию
        return KeyCreationStages::AES_FORM_SESSION_KEY;
    }

    // Иначе, остаёмся на своей стадии
    return KeyCreationStages::INIT_AES_ENCRYPTION;
}


KeyCreationStages ChatCommandsManager::aes_form_session_key(Message& rcv_msg, std::string snd_id_str) {
    
    // Определим индекс id-шника человека, который отправил это сообщение
    snd_id_str = (snd_id_str == "0" || snd_id_str == "") ? my_id_str : snd_id_str; // если передали id=0 => это сообщение отправили мы сами (особбеность дешифровки и запросов в mtpBuffer телеграм)
    auto it = std::find(chat_members_str.begin(), chat_members_str.end(), snd_id_str);
    int snd_id_pos = (it != chat_members_str.end()) ? std::distance(chat_members_str.begin(), it) : -1;

    std::string key_to_send = ""; // ключ, который будет отправлен следующему участнику чата
    int last_peer_n_to_send = -1; // позиция человека, для которого формируется ключ

    // Если мы первый и едниственный участник чата
    if (members_len == 1) {
        aes_key = aes_manager.create_key_solo(); // создаём ключ в соло
        return KeyCreationStages::END_KEY_FORMING; // завершаем шифрование
    }
    // Если мы первый, но не единственный участник чата
    else if (my_id_pos == 0 && rcv_msg.aes_init) {
        key_to_send = my_dh_params.public_key; // наш публичный ключ является началом фомирования ключа
        last_peer_n_to_send = members_len - 1; // для последнего человека
    }
    // Если мы тот, для кого формируется ключ
    else if (my_id_pos == (snd_id_pos + 1) % members_len && rcv_msg.aes_form && my_id_pos == rcv_msg.last_peer_n) {
        aes_key = aes_manager.сreate_key_multi(my_dh_params, rcv_msg.text, true); // формируем ключ из отправленного нам

        // Начинаем формирование ключа для следующего пользователя
        key_to_send = my_dh_params.public_key; // наш публичный ключ является началом фомирования ключа
        last_peer_n_to_send = my_id_pos - 1; // для человека перед нами

        // Если мы являемся последним, кто получил aes_key
        if (my_id_pos == 0) {
            return KeyCreationStages::END_KEY_FORMING; // завершаем шифрование
        }
    }
    // Если мы тот, кто является промежуточным этапом в формировании ключа
    else if (my_id_pos == (snd_id_pos + 1) % members_len && rcv_msg.aes_form) { // мы являемся следующим человеком по возрастанию id после того, кто отправил сообщение
        key_to_send = aes_manager.сreate_key_multi(my_dh_params, rcv_msg.text); // домнажаем ключ отправившего на свою приватную степень
        last_peer_n_to_send = rcv_msg.last_peer_n; // позиция человека, для которого формируется ключ
    }

    // Если мы предварительно не завершили шифрование и у нас есть, что отправить
    if (last_peer_n_to_send != -1 && key_to_send != "") {
        // Формируем сообщение
        Message message_to_send;
        message_to_send.rsa_use = true;
        message_to_send.aes_form = true;
        message_to_send.dh_fastmode = rcv_msg.dh_fastmode;
        message_to_send.rsa_key_n = rcv_msg.rsa_key_n;
        message_to_send.aes_key_n = rcv_msg.aes_key_n;
        message_to_send.last_peer_n = last_peer_n_to_send; // указываем собеседника, определённого нами ранее
        message_to_send.rsa_key_len = rcv_msg.rsa_key_len;
        message_to_send.text = key_to_send; // передаём ключ, определённый нами ранее

        // Отправляем сообщение
        std::this_thread::sleep_for(std::chrono::milliseconds(SENDING_DELAY));
        lambda_send_message(message_to_send.get_text_with_options());
    }

    // Иначе, остаёмся на своей стадии
    return KeyCreationStages::AES_FORM_SESSION_KEY;
}


void ChatCommandsManager::end_key_forming(Message& rcv_msg) {

    // Формируем сообщение
    Message message_to_send;
    message_to_send.end_key_forming = true;
    message_to_send.dh_fastmode = rcv_msg.dh_fastmode;
    message_to_send.rsa_key_n = rcv_msg.rsa_key_n;
    message_to_send.aes_key_n = rcv_msg.aes_key_n;
    message_to_send.last_peer_n = my_id_pos;
    message_to_send.rsa_key_len = rcv_msg.rsa_key_len;
    message_to_send.text = "stop encryption key forming";

    // Отправляем сообщение
    std::this_thread::sleep_for(std::chrono::milliseconds(SENDING_DELAY));
    lambda_send_message(message_to_send.get_text_with_options());
}