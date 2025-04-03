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
std::string ChatKeyCreation::sender_id_str;
int ChatKeyCreation::sender_id_pos;
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
    std::string aes_key = ""; // итоговый ключ шифрования (одинаковый для всех)

    while (true) {
        cv.wait(lock, [] { return continue_creation || stop_creation; });  // ждем разрешения на продолжение (один из bool флагов должен стать true)
        if (stop_creation) break; 

        std::cout << "KeyCreationStages: " << (int) KeyCreationStages::INIT_RSA_ENCRYPTION << '\n';
        Message rcv_msg = recieved_message;
        int snd_id_pos = sender_id_pos;

        continue_creation = false;
        lock.unlock();

        // Чтобы полученное сообщение успело прогрузиться раньше, чем отправленные ответ
        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        /* Критическая секция */
std::cout << "__"  << -1 << '\n';

std::cout << "__"  << 0 << '\n';
        if (cur_stage == KeyCreationStages::INIT_RSA_ENCRYPTION) { // мы являемся инициатором шифрования

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
std::cout << "__" << 2.15 << '\n';
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
                message_to_send.aes_init = true;
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
                if (rcv_msg.aes_init == true) {
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
            
std::cout << "__"  << 10 << '\n';
            std::string key_to_send = ""; // ключ, который будет отправлен следующему участнику чата
            int last_peer_n_to_send = -1; // позиция человека, для которого формируется ключ

            // Если мы первый и едниственный участник чата
            if (members_len == 1) {
std::cout << "__"  << 11 << '\n';
                aes_key = aes_manager.create_key_solo(); // создаём ключ в соло
                std::cout << "!!! aes_key: " << aes_key << '\n';
                cur_stage = KeyCreationStages::END_KEY_FORMING; // завершаем шифрование
            }
            // Если мы первый, но не единственный участник чата
            else if (rcv_msg.aes_init == true) {
std::cout << "__"  << 12 << '\n';
                key_to_send = my_dh_params.public_key; // наш публичный ключ является началом фомирования ключа
                last_peer_n_to_send = members_len - 1; // для последнего человека
            }
            // Если мы тот, для кого формируется ключ
            else if (my_id_pos == rcv_msg.last_peer_n) {
std::cout << "__"  << 13 << '\n';
                aes_key = aes_manager.сreate_key_multi(my_dh_params, rcv_msg.text, true); // формируем ключ из отправленного нам
                std::cout << "!!! aes_key: " << aes_key << '\n';
                // Начинаем формирование ключа для следующего пользователя
                key_to_send = my_dh_params.public_key; // наш публичный ключ является началом фомирования ключа
                last_peer_n_to_send = my_id_pos - 1; // для человека перед нами

                // Если мы являемся последним, кто получил aes_key
                if (my_id_pos == 0) {
                    cur_stage = KeyCreationStages::END_KEY_FORMING; // завершаем шифрование
                }
            }
            // Если мы тот, кто является промежуточным этапом в формировании ключа
            else if (my_id_pos == (snd_id_pos + 1) % members_len) { // мы являемся следующим человеком по возрастанию id после того, кто отправил сообщение
std::cout << "__"  << 14 << '\n';
                key_to_send = aes_manager.сreate_key_multi(my_dh_params, rcv_msg.text); // домнажаем ключ отправившего на свою приватную степень
                last_peer_n_to_send = rcv_msg.last_peer_n; // позиция человека, для которого формируется ключ
            }

            // Если мы предварительно не завершили шифрование и у нас есть, что отправить
            if (cur_stage != KeyCreationStages::END_KEY_FORMING && last_peer_n_to_send != -1 && key_to_send != "") {
std::cout << "__"  << 15 << '\n';
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
                lambda_send_message(message_to_send.get_text_with_options());
            }
        }
std::cout << "__"  << 16 << '\n';   
        // Сюда может попасть толькол pos=0
        if (cur_stage == KeyCreationStages::END_KEY_FORMING) {
std::cout << "__"  << 17 << '\n';

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
            lambda_send_message(message_to_send.get_text_with_options());

            // Временный костыль (завершаем этот поток через другой поток, который завершит себя) - можно завершить в mtp_buffer_ecnrypt (в теории)
            std::thread([] {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                ChatKeyCreation::stop();  // вызывается из другого потока
            }).detach();
        }
std::cout << "__"  << 18 << '\n';

        lock.lock();
    }
std::cout << "__"  << 19 << '\n';
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
        
        // Сохраняем переданные данные в класс
        recieved_message = rcv_msg;
        sender_id_str = (snd_id == "0" || snd_id == "") ? my_id_str : snd_id; // если передали id=0 => это сообщение отправили мы сами (особбеность дешифровки и запросов в mtpBuffer телеграм)
        
        // Получаем индекс id отправителя из chat_members_str
        auto it = std::find(chat_members_str.begin(), chat_members_str.end(), sender_id_str);
        sender_id_pos = std::distance(chat_members_str.begin(), it);

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

    std::cout << "_Поток был остановлен_" << '\n';
}

