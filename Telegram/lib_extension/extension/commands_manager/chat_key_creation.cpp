#include "chat_key_creation.h"

// Объявляем static переменные

std::function<void(const std::string&)> ChatKeyCreation::lambda_send_message = 
    [](const std::string&) {}; // заглушка, чтобы компилятор не ругался
std::function<void(quint64&, quint64&, std::vector<quint64>&)> ChatKeyCreation::lambda_get_chat_ids = 
    [](quint64& my_id, quint64& chat_id, std::vector<quint64>& chat_members) {}; // заглушка, чтобы компилятор не ругался

std::string ChatKeyCreation::my_id_str;
std::string ChatKeyCreation::chat_id_str;
std::vector<std::string> ChatKeyCreation::chat_members_str;
Message ChatKeyCreation::recieved_message;
std::string ChatKeyCreation::sender_id;
KeyCreationStages ChatKeyCreation::cur_stage;
std::thread ChatKeyCreation::thread;
std::mutex ChatKeyCreation::mtx;
std::condition_variable ChatKeyCreation::cv;
bool ChatKeyCreation::continue_creation;
bool ChatKeyCreation::stop_creation;


void ChatKeyCreation::chat_key_creation() {
    std::unique_lock<std::mutex> lock(mtx);
    KeysDataBase db;
    RsaKeyManager rsa_manager;
    AesKeyManager aes_manager;

    while (true) {
        cv.wait(lock, [] { return continue_creation || stop_creation; });  // ждем разрешения на продолжение (один из bool флагов должен стать true)
        if (stop_creation) break; 

        Message rcv_msg = recieved_message;
        std::string snd_id = sender_id;

        continue_creation = false;
        lock.unlock();

        /* Критическая секция */


        if (cur_stage == KeyCreationStages::INIT_ENCRYPTION) { // мы являемся инициатором шифрования

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
            message_to_send.last_peer_n = 0; // человек, который должен первым отправить свой rsa ключ
            message_to_send.rsa_key_len = 2048;
            message_to_send.text = "start_encryption";

            // Отправляем сообщение
            lambda_send_message(message_to_send.get_text_with_options());

            // Переходим на следующую стадию
            cur_stage = KeyCreationStages::RSA_SEND_PUBLIC_KEY;
        }

        if (cur_stage == KeyCreationStages::RSA_SEND_PUBLIC_KEY)
        // if (stage == 0 || stage == 1) {

        // }

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

    // Обнуляем флаги
    stop_creation = false;
    continue_creation = false;

    cur_stage = start_stage;

    // Получаем id-шники
    quint64 my_id;
	quint64 chat_id;
	std::vector<quint64> chat_members;
    lambda_get_chat_ids(my_id, chat_id, chat_members);

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

    std::cout << "my_id_str: " << my_id_str << '\n';
    std::cout << "chat_id_str: " << chat_id_str << '\n';

    thread = std::thread(ChatKeyCreation::chat_key_creation);
    //thread = std::thread(ChatKeyCreation::chat_key_creation, std::ref(send_func));
    lambda_send_message("start thread!!!");
}


void ChatKeyCreation::add_info(const Message& rcv_msg, const std::string& snd_id) {
    {
        std::lock_guard<std::mutex> lock(mtx); // лочим мьютекс, чтобы другой поток не смог одновременно вызвать функцию создания ключа
        recieved_message = rcv_msg;
        sender_id = snd_id;
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

    lambda_send_message("stop thread");
}

