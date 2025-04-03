#include <thread>
#include <mutex>
#include <condition_variable>
#include <string>
#include <functional>
#include "chat_commands_manager.h"

#pragma once

class ChatKeyCreation {
private:
    static ChatCommandsManager commands;

    static Message recieved_message; // сообщение которое было получено
    static std::string sender_id_str; // id собеседника, который отправил это сообщение
    static KeyCreationStages cur_stage;

    static std::thread thread;
    static std::mutex mtx;
    static std::condition_variable cv;
    static bool continue_creation;
    static bool stop_creation;
    static bool is_started_flag; // флаг, что создание ключей было начато

    // Создание ключа
    static void chat_key_creation();

public:

    static bool is_started() {
        return is_started_flag;
    }

    static void set_lambda_functions(
        const std::function<void(const std::string&)>& send_func, // лямбда функция отправки сообщения (передаётся в history_vew_top_bar_widget.cpp)
        const std::function<void(quint64&, quint64&, std::vector<quint64>&)>& get_id_func); // лямбда функция получения id собеседников (передаётся в history_vew_top_bar_widget.cpp)

    // Функция, которая создаёт новый поток создания ключа
    static void start(const KeyCreationStages start_stage);
    
    // Добавляет информацию в поток создания ключа и пробуждает его
    static void add_info(const Message& rcv_msg, const std::string& snd_id_str);

    // Функция, которая останавливает поток создания ключа
    static void stop();
};