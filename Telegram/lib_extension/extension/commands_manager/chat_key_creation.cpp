#include "chat_key_creation.h"

namespace ext {

// Объявляем static переменные
ChatCommandsManager ChatKeyCreation::commands;
Message ChatKeyCreation::recieved_message;
std::string ChatKeyCreation::sender_id_str;
KeyCreationStages ChatKeyCreation::cur_stage;
std::thread ChatKeyCreation::thread;
std::mutex ChatKeyCreation::mtx;
std::condition_variable ChatKeyCreation::cv;
bool ChatKeyCreation::continue_creation;
bool ChatKeyCreation::stop_creation;
bool ChatKeyCreation::is_started_flag = false;


void ChatKeyCreation::chat_key_creation() {
    std::unique_lock<std::mutex> lock(mtx);

    while (true) {
        cv.wait(lock, [] { return continue_creation || stop_creation; });  // ждем разрешения на продолжение (один из bool флагов должен стать true)
        if (stop_creation) break; 

        std::cout << "KeyCreationStages: " << (int) cur_stage << '\n';
        Message rcv_msg = recieved_message;
        std::string snd_id = sender_id_str;

        continue_creation = false;
        lock.unlock();


        /* Начало критической секция */

        // Сюда может попасть только инициатор шифрования
        if (cur_stage == KeyCreationStages::INIT_RSA_ENCRYPTION) { // мы являемся инициатором шифрования
            cur_stage = commands.init_rsa_encryption(rcv_msg);
        }

        if (cur_stage == KeyCreationStages::RSA_SEND_PUBLIC_KEY) {
            cur_stage = commands.rsa_send_public_key(rcv_msg);
        }

        if (cur_stage == KeyCreationStages::INIT_AES_ENCRYPTION) {
            cur_stage =commands.init_aes_encryption(rcv_msg);
        }

        if (cur_stage == KeyCreationStages::AES_FORM_SESSION_KEY) {
            cur_stage = commands.aes_form_session_key(rcv_msg, sender_id_str);
        }
  
        // Сюда может попасть только человек с наименьшем id (my_id_pos = 0)
        if (cur_stage == KeyCreationStages::END_KEY_FORMING) {
            commands.end_key_forming(rcv_msg);

            // Временный костыль (завершаем этот поток через другой поток, который завершит себя) - можно завершить в mtp_buffer_ecnrypt (в теории)
            std::thread([] {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                ChatKeyCreation::stop();  // вызывается из другого потока
            }).detach();
        }

        /* Конец критической секция */

        lock.lock();
    }
}


void ChatKeyCreation::set_lambda_functions(
    const std::function<void(const std::string&)>& send_func,
    const std::function<void(quint64&, quint64&, std::vector<quint64>&)>& get_id_func) {
        
    commands.set_lambda_functions(send_func, get_id_func);
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

    // Получаем id-шники из чата
    commands.set_chat_members_info();

    // Очищаем предыдущие результаты
    commands.reset_functions_results();
   
    // Запускаем новый поток
    thread = std::thread(ChatKeyCreation::chat_key_creation);
}


void ChatKeyCreation::add_info(const Message& rcv_msg, const std::string& snd_id_str) {
    {
        std::lock_guard<std::mutex> lock(mtx); // лочим мьютекс, чтобы другой поток не смог одновременно вызвать функцию создания ключа
        
        // Сохраняем переданные данные в класс
        recieved_message = rcv_msg;
        sender_id_str = snd_id_str;

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


void ChatKeyCreation::end_encryption(bool notify_others) {
    commands.end_encryption(notify_others);
}

} // namespace ext