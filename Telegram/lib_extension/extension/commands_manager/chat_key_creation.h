// #include "base/assertion.h"
// #include "ui/text/text_entity.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <functional>

#pragma once

class ChatKeyCreation {
private:
    static std::thread thread;
    static std::mutex mtx;
    static std::condition_variable cv;
    static std::string message;
    static bool continue_creation;
    static bool stop_creation;

    // Создание ключа
    static void chat_key_creation(const std::function<void(const std::string&)>& send_func);

public:

    // Функция, которая создаёт новый поток создания ключа
    static void start(const std::function<void(const std::string&)>& send_func);
    
    // Добавляет информацию в поток создания ключа и пробуждает его
    static void add_info(std::string msg);

    // Функция, которая останавливает поток создания ключа
    static void stop(const std::function<void(const std::string&)>& send_func);
};