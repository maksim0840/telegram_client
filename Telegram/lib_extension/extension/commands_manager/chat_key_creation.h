// #include "base/assertion.h"
// #include "ui/text/text_entity.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <functional>
#include "../local_storage/keys_db.h"
#include "../keys_manager/keys_manager.h"
#include "../chat_members/chat_members.h"
#include "message_options.h"
#include <QtGlobal>
#include <iostream>
#include <cstdint>
#include <string>
#include <vector>

#pragma once

enum class KeyCreationStages {
    INIT_ENCRYPTION
};

class ChatKeyCreation {
private:
    static std::function<void(const std::string&)> lambda_send_message; // лямбда функция отправки сообщения (передаётся в history_vew_top_bar_widget.cpp)
    static std::function<void(quint64&, quint64&, std::vector<quint64>&)> lambda_get_chat_ids; // лямбда функция получения id собеседников (передаётся в history_vew_top_bar_widget.cpp)

    static std::string my_id_str;
    static std::string chat_id_str;
    static std::vector<std::string> chat_members_str;

    static Message recieved_message; // сообщение которое было получено
    static std::string sender_id; // id собеседника, который отправил это сообщение
    static KeyCreationStages cur_stage; // текущая стадия создания ключа

    static std::thread thread;
    static std::mutex mtx;
    static std::condition_variable cv;
    static bool continue_creation;
    static bool stop_creation;

    // Создание ключа
    static void chat_key_creation();

public:

    static void set_lambda_functions(
        const std::function<void(const std::string&)>& send_func,
        const std::function<void(quint64&, quint64&, std::vector<quint64>&)>& get_id_func);

    // Функция, которая создаёт новый поток создания ключа
    static void start(const KeyCreationStages start_stage);
    
    // Добавляет информацию в поток создания ключа и пробуждает его
    static void add_info(const Message& rcv_msg, const std::string& snd_id);

    // Функция, которая останавливает поток создания ключа
    static void stop();
};