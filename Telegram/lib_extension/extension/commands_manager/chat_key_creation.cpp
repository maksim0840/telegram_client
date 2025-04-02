#include "chat_key_creation.h"

// Объявляем static переменные

std::function<void(const std::string&)> ChatKeyCreation::lambda_send_message = 
    [](const std::string&) {}; // заглушка, чтобы компилятор не ругался
std::function<void(quint64&, quint64&, std::vector<quint64>&)> ChatKeyCreation::lambda_get_chat_ids = 
    [](quint64& my_id, quint64& chat_id, std::vector<quint64>& chat_members) {}; // заглушка, чтобы компилятор не ругался

std::string ChatKeyCreation::my_id_str;
std::string ChatKeyCreation::chat_id_str;
std::vector<std::string> ChatKeyCreation::chat_members_str;
int ChatKeyCreation::my_id_pos;
int ChatKeyCreation::members_len;
Message ChatKeyCreation::recieved_message;
std::string ChatKeyCreation::sender_id;
KeyCreationStages ChatKeyCreation::cur_stage;
std::thread ChatKeyCreation::thread;
std::mutex ChatKeyCreation::mtx;
std::condition_variable ChatKeyCreation::cv;
bool ChatKeyCreation::continue_creation;
bool ChatKeyCreation::stop_creation;
bool ChatKeyCreation::is_started_flag = false;


void ChatKeyCreation::chat_key_creation() {
    std::unique_lock<std::mutex> lock(mtx);
    KeysDataBase db;
    RsaKeyManager rsa_manager;
    AesKeyManager aes_manager;

    std::vector<std::string> members_rsa_public_key(members_len, "");
    std::string my_rsa_private_key;
    DHParamsStr my_dh_params;

    while (true) {
        cv.wait(lock, [] { return continue_creation || stop_creation; });  // ждем разрешения на продолжение (один из bool флагов должен стать true)
        if (stop_creation) break; 

        std::cout << "KeyCreationStages: " << (int) KeyCreationStages::INIT_RSA_ENCRYPTION << '\n';
        Message rcv_msg = recieved_message;
        std::string snd_id = sender_id;

        continue_creation = false;
        lock.unlock();

        /* Критическая секция */
std::cout << "__"  << 0 << '\n';
        if (cur_stage == KeyCreationStages::INIT_RSA_ENCRYPTION) { // мы являемся инициатором шифрования

            // Получаем последние номера уже созданных ключей и увеличиваем их на 1
            std::optional<int> rsa_key_n = db.get_last_key_n(chat_id_str, my_id_str, KeysTablesDefs::RSA);
            std::optional<int> aes_key_n = db.get_last_key_n(chat_id_str, my_id_str, KeysTablesDefs::AES);
            rsa_key_n = (rsa_key_n) ? (*rsa_key_n + 1) : 1;
            aes_key_n = (aes_key_n) ? (*aes_key_n + 1) : 1;

            // Собираем сообщение для отправки
            Message message_to_send;
            message_to_send.rsa_init = true;
            message_to_send.dh_fastmode = true;
            message_to_send.rsa_key_n = *rsa_key_n;
            message_to_send.aes_key_n = *aes_key_n;
            message_to_send.last_peer_n = my_id_pos; // свой id (тот, кто инициализирует rsa шифрование)
            message_to_send.rsa_key_len = 2048;
            message_to_send.text = "start_encryption";

            // Отправляем сообщение
            lambda_send_message(message_to_send.get_text_with_options());

            // Переходим на следующую стадию
            cur_stage = KeyCreationStages::RSA_SEND_PUBLIC_KEY;

            // Получаем своё же сообщение
            rcv_msg = message_to_send;
        }
std::cout << "__"  << 1 << '\n';
std::cout << my_id_pos << ' ' << rcv_msg.last_peer_n + 1 << ' ' << rcv_msg.rsa_init << ' ' << members_len << '\n';
        if (cur_stage == KeyCreationStages::RSA_SEND_PUBLIC_KEY) {
            if ((my_id_pos == 0 && rcv_msg.rsa_init == true) || // если мы первый в списке и не отправляли свой ключ (начинаем)
                (my_id_pos == (rcv_msg.last_peer_n + 1) && rcv_msg.rsa_init == false)) { // если мы следующий кто должен отправлять и первый уже отправил
std::cout << "__"  << 2 << '\n';
                // Сохраняем полученный ключ от предыдущего отправившего
                if (rcv_msg.rsa_init == false) { // предыдущее сообщение содержит публичный ключ
std::cout << "__" << 2.1 << '\n';
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
                lambda_send_message(message_to_send.get_text_with_options());
std::cout << "__" << 2.2 << '\n';
                // Переходим на следующиую стадию, если мы были последним
                if (my_id_pos == members_len - 1) {
std::cout << "__"  << 3 << '\n';
                    cur_stage = KeyCreationStages::INIT_AES_ENCRYPTION;
                }
            }
            else if (rcv_msg.rsa_init == false) {
                // Сохраняем полученный ключ от предыдущего отправившего
                members_rsa_public_key[rcv_msg.last_peer_n] = rcv_msg.text;
std::cout << "__"  << 4 << '\n';
                // Переходим на следующиую стадию, если прочитали сообщение последнего
                if (rcv_msg.last_peer_n == members_len - 1) {
std::cout << "__"  << 5 << '\n';
                    cur_stage = KeyCreationStages::INIT_AES_ENCRYPTION;
                }
            }
        }

        if (cur_stage == KeyCreationStages::INIT_AES_ENCRYPTION) {
            if (my_id_pos == 0) { // если мы первый
std::cout << "__"  << 6 << '\n';
                // Создаём и запоминаем свои параметры aes шифрования
                DHParamsStr dh_params = aes_manager.get_dh_params();
                my_dh_params = dh_params;
                std::vector<std::string> shared_params = {dh_params.p, dh_params.g};

                // Собираем сообщение для отправки параметров P и G
                Message message_to_send;
                message_to_send.rsa_use = true;
                message_to_send.aes_form = true;
                message_to_send.dh_fastmode = rcv_msg.dh_fastmode;
                message_to_send.rsa_key_n = rcv_msg.rsa_key_n;
                message_to_send.aes_key_n = rcv_msg.aes_key_n;
                message_to_send.last_peer_n = my_id_pos; // свой id (тот, кто отправляет параметры P и G)
                message_to_send.rsa_key_len = rcv_msg.rsa_key_len;
                message_to_send.text = KeysDataBaseHelper::vector_to_string(shared_params); // отправляем параметры p и g
                
                // Отправляем сообщение
                lambda_send_message(message_to_send.get_text_with_options());

                // Переходим на следующиую стадию
                cur_stage = KeyCreationStages::AES_FORM_SESSION_KEY;

                // Получаем своё же сообщение
                rcv_msg = message_to_send;
            }
            else {
std::cout << "__"  << 7 << '\n';
                if (rcv_msg.aes_form == true) {
std::cout << "__" << 8 << '\n';
                    // Получаем параметры P и G от первого пользователя
                    std::vector<std::string> shared_params = KeysDataBaseHelper::string_to_vector(rcv_msg.text);

                    // Создаём и запоминаем свои параметры aes шифрования (используя уже сфорированные P и G)
                    DHParamsStr dh_params = aes_manager.get_dh_params_secondly(shared_params[0], shared_params[1]);
                    my_dh_params = dh_params;

                    // Переходим на следующиую стадию
                    cur_stage = KeyCreationStages::AES_FORM_SESSION_KEY;
                }
            }
        }
std::cout << "__"  << 9 << '\n';
        if (cur_stage == KeyCreationStages::AES_FORM_SESSION_KEY) {
            if (my_id_pos == 0 && rcv_msg.aes_init == true) {
                    
                // Формируем сообщение
                Message message_to_send;
                message_to_send.rsa_use = true;
                message_to_send.aes_form = true;
                message_to_send.dh_fastmode = rcv_msg.dh_fastmode;
                message_to_send.rsa_key_n = rcv_msg.rsa_key_n;
                message_to_send.aes_key_n = rcv_msg.aes_key_n;
                message_to_send.last_peer_n = ((my_id_pos - 1) % members_len + members_len) % members_len; // <=> (my_id_pos - 1) mod members_count  (((id человека, для которого формируентся ключ)))
                message_to_send.rsa_key_len = rcv_msg.rsa_key_len;
                message_to_send.text = my_dh_params.public_key; // передаём свой публичный ключ в чат (т.к. мы первый отправляющий)

                // Отправляем сообщение
                lambda_send_message(message_to_send.get_text_with_options());
            }
        }

        lock.lock();
    }
}


void ChatKeyCreation::set_lambda_functions(
    const std::function<void(const std::string&)>& send_func,
    const std::function<void(quint64&, quint64&, std::vector<quint64>&)>& get_id_func) {
        
    lambda_send_message = send_func;
    lambda_get_chat_ids = get_id_func;
}


void ChatKeyCreation::start(const KeyCreationStages start_stage) {
    // Если поток уже запущен — остановим его
    if (thread.joinable()) {
        stop();
    }

    // Обновляем флаги
    stop_creation = false;
    continue_creation = false;
    is_started_flag = true;

    cur_stage = start_stage;

    // Получаем id-шники
    quint64 my_id;
	quint64 chat_id;
	std::vector<quint64> chat_members;
    lambda_get_chat_ids(my_id, chat_id, chat_members);

    // Получаем размер вектора chat_members
    members_len = chat_members.size();
    if (members_len >= MAX_PEERS_IN_CHAT) {
        throw std::runtime_error("Превышен лимит пользователей чата (если это необходимо, то измените его в chat_key_creation.h)");
    }

    // Сохраняем информацию о чате в базе данных
    update_chat_members(chat_id, my_id, chat_members);
    
    // Конвертируем полученные параметры id пользователей в строки
    my_id_str = std::to_string(my_id); 
    chat_id_str = std::to_string(chat_id); 
    chat_members_str.clear();
    for (const auto& id : chat_members) {
        chat_members_str.push_back(std::to_string(id));
    }

    // Сортируем id участников чата по возрастанию
    std::sort(chat_members_str.begin(), chat_members_str.end());

    // Получаем индекс нашего id в chat_members_str
    auto it = std::find(chat_members_str.begin(), chat_members_str.end(), my_id_str);
    my_id_pos = std::distance(chat_members_str.begin(), it);

    std::cout << "my_id_str: " << my_id_str << '\n';
    std::cout << "chat_id_str: " << chat_id_str << '\n';

    thread = std::thread(ChatKeyCreation::chat_key_creation);
    //thread = std::thread(ChatKeyCreation::chat_key_creation, std::ref(send_func));
}


void ChatKeyCreation::add_info(const Message& rcv_msg, const std::string& snd_id) {
    {
        std::lock_guard<std::mutex> lock(mtx); // лочим мьютекс, чтобы другой поток не смог одновременно вызвать функцию создания ключа
        recieved_message = rcv_msg;
        sender_id = (snd_id == "0") ? my_id_str : snd_id; // если передали id=0 => это сообщение отправили мы сами (особбеность дешифровки и запросов в mtpBuffer телеграм)
        continue_creation = true;  // говорим функции, что можно разблокироваться (с целью продолжить цикл)
    }
    cv.notify_one(); // разбудить поток
}


void ChatKeyCreation::stop() {
    // Если поток не был запущен - игнорируем
    if (!thread.joinable()) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mtx); // лочим мьютекс, чтобы другой поток не смог одновременно вызвать функцию создания ключа
        stop_creation = true;  // говорим функции, что можно разблокироваться (с целью завершить цикл)
    }
    cv.notify_one(); // разбудить поток
    if (thread.joinable()) thread.join(); // завершить поток
    thread = std::thread();

    // Убираем флаг работы
    is_started_flag = false;
}

