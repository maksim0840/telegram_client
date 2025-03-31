#include "chat_key_creation.h"

// Объявляем static переменные
std::function<void(const std::string&)> ChatKeyCreation::lambda_send_message = [](const std::string&) {}; // заглушка, чтобы компилятор не ругался
std::string ChatKeyCreation::my_id_str;
std::string ChatKeyCreation::chat_id_str;
std::vector<std::string> ChatKeyCreation::chat_members_str;
std::thread ChatKeyCreation::thread;
std::mutex ChatKeyCreation::mtx;
std::condition_variable ChatKeyCreation::cv;
std::string ChatKeyCreation::message;
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

        std::string msg = message;
        continue_creation = false;
        lock.unlock();

        // ... что-то делаю с msg

        lock.lock();
    }
}


void ChatKeyCreation::set_lambda_send_message(const std::function<void(const std::string&)>& send_func) {
    lambda_send_message = send_func;
}


void ChatKeyCreation::start(const quint64& my_id, const quint64& chat_id, const std::vector<quint64>& chat_members) {
    // Если поток уже запущен — остановим его
    if (thread.joinable()) {
        stop();
    }

    // Обнуляем флаги
    stop_creation = false;
    continue_creation = false;

    // Конвертируем полученные параметры id пользователей в строки
    my_id_str = std::to_string(my_id); 
    chat_id_str = std::to_string(chat_id); 
    chat_members_str.clear();
    for (const auto& id : chat_members) {
        chat_members_str.push_back(std::to_string(id));
    }

    std::cout << "my_id_str: " << my_id_str << '\n';
    std::cout << "chat_id_str: " << chat_id_str << '\n';

    thread = std::thread(ChatKeyCreation::chat_key_creation);
    //thread = std::thread(ChatKeyCreation::chat_key_creation, std::ref(send_func));
    lambda_send_message("start thread!!!");
}


void ChatKeyCreation::add_info(std::string msg) {
    {
        std::lock_guard<std::mutex> lock(mtx); // лочим мьютекс, чтобы другой поток не смог одновременно вызвать функцию создания ключа
        message = msg;
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

