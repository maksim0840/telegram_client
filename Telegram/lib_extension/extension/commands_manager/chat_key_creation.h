// #include "base/assertion.h"
// #include "ui/text/text_entity.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <functional>

#pragma once

/*
class ChatKeyCreation {
private:
    static std::thread thread;
    static std::mutex mtx;
    static std::condition_variable cv;
    static std::string message;
    static bool continue_creation = false;
    static bool stop_creation = false;

    // Создание ключа
    static void chat_key_creation() {
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

public:
    // Функция, которая создаёт новый поток создания ключа
    static void start() {
        thread = std::thread(chat_key_creation);
    }
    
    // Добавляет информацию в поток создания ключа и пробуждает его
    static void add_info(std::string msg) {
        {
            std::lock_guard<std::mutex> lock(mtx); // лочим мьютекс, чтобы другой поток не смог одновременно вызвать функцию создания ключа
            message = msg;
            continue_creation = true;  // говорим функции, что можно разблокироваться (с целью продолжить цикл)
        }
        cv.notify_one(); // разбудить поток
    }

    // Функция, которая останавливает поток создания ключа
    static void stop() {
        {
            std::lock_guard<std::mutex> lock(mtx); // лочим мьютекс, чтобы другой поток не смог одновременно вызвать функцию создания ключа
            stop_creation = true;  // говорим функции, что можно разблокироваться (с целью завершить цикл)
        }
        cv.notify_one(); // разбудить поток
        if (thread.joinable()) thread.join(); // завершить поток
    }
};
*/

void start_chat_key_creation(const std::function<void(const std::string&)> &send_func);