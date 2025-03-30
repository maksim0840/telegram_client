#include "chat_key_creation.h"

// Объявляем static переменные
std::thread ChatKeyCreation::thread;
std::mutex ChatKeyCreation::mtx;
std::condition_variable ChatKeyCreation::cv;
std::string ChatKeyCreation::message;
bool ChatKeyCreation::continue_creation;
bool ChatKeyCreation::stop_creation;


void ChatKeyCreation::chat_key_creation(const std::function<void(const std::string&)>& send_func) {
    std::unique_lock<std::mutex> lock(mtx);
    while (true) {
        cv.wait(lock, [] { return continue_creation || stop_creation; });  // ждем разрешения на продолжение цикла (один из bool флагов должен стать true)
        if (stop_creation) break; 

        std::string msg = message;
        continue_creation = false;
        lock.unlock();

        // ... что-то делаю с msg

        lock.lock();
    }
}


void ChatKeyCreation::start(const std::function<void(const std::string&)>& send_func) {

    // Если поток уже запущен — остановим его
    if (thread.joinable()) {
        stop(send_func);
    }

    // Обнуляем флаги
    stop_creation = false;
    continue_creation = false;

    thread = std::thread(ChatKeyCreation::chat_key_creation, send_func);
    //thread = std::thread(ChatKeyCreation::chat_key_creation, std::ref(send_func));
    send_func("start thread");
}


void ChatKeyCreation::add_info(std::string msg) {
    {
        std::lock_guard<std::mutex> lock(mtx); // лочим мьютекс, чтобы другой поток не смог одновременно вызвать функцию создания ключа
        message = msg;
        continue_creation = true;  // говорим функции, что можно разблокироваться (с целью продолжить цикл)
    }
    cv.notify_one(); // разбудить поток
}


void ChatKeyCreation::stop(const std::function<void(const std::string&)>& send_func) {
    
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

    send_func("stop thread");
}

