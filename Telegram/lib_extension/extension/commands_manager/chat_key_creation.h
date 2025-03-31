// #include "base/assertion.h"
// #include "ui/text/text_entity.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <functional>
#include "../local_storage/keys_db.h"
#include "../keys_manager/keys_manager.h"
#include "message_options.h"
#include <QtGlobal>
#include <iostream>
#include <cstdint>
#include <string>
#include <vector>

#pragma once

class ChatKeyCreation {
private:
    static std::function<void(const std::string&)> lambda_send_message; // лямбда функция отправки сообщения (передаётся в history_vew_top_bar_widget.cpp)
    
    static std::string my_id_str;
    static std::string chat_id_str;
    static std::vector<std::string> chat_members_str;

    static std::thread thread;
    static std::mutex mtx;
    static std::condition_variable cv;
    static std::string message;
    static bool continue_creation;
    static bool stop_creation;

    // Создание ключа
    static void chat_key_creation();

public:
    static void set_lambda_send_message(const std::function<void(const std::string&)>& send_func);

    // Функция, которая создаёт новый поток создания ключа
    static void start(const quint64& my_id, const quint64& chat_id, const std::vector<quint64>& chat_members);
    
    // Добавляет информацию в поток создания ключа и пробуждает его
    static void add_info(std::string msg);

    // Функция, которая останавливает поток создания ключа
    static void stop();
};