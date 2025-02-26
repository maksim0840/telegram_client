#include "message_text_encryption.h"

int extract_num(const std::string& str, const int start, const int end) {
    if (start < 0 || start > end) {
        throw std::invalid_argument("Ошибка: Число не найдено");
    }
    std::string sub_str = str.substr(start, end - start + 1);
    // for (const char c : sub_str) {
    //     if (!std::isdigit(c)) {
    //         throw std::invalid_argument("Ошибка: Число содержит недопустимые символы.");
    //     }
    // }
    return std::stoi(sub_str);
}

std::string get_command_result(const std::string& text_str, const std::string& chat_id_str) {
    ChatCommandsManager commands_manager(chat_id_str);

    if (text_str.find("[extension]/start_rsa") == 0) {
        // ключ 2048 бит
        if (text_str == "[extension]/start_rsa") {
            return commands_manager.start_rsa();
        }
        // ключ пользовательской длинны
        else {
            int start = text_str.find(" ");
            int end = text_str.length() - 1;
            std::cout << start << ' ' << end;
            int key_len = extract_num(text_str, start, end);
            if (key_len < 512 || key_len > 8192 || key_len % 64 != 0) {
                throw std::invalid_argument("Неподдерживаемая длина ключа RSA");
            }
            return commands_manager.start_rsa(key_len);
        }
    }
    else if (text_str == "[extension]/start_aes") {
        return commands_manager.start_aes();
    }
    else if (text_str == "[extension]/finish_aes") {
        return commands_manager.finish_aes();
    }
    else if (text_str == "[extension]/stop_encryption") {
        return commands_manager.stop_encryption();
    }
    return "";
}

QString encrypt_the_message(const QString& text, const quint64 chat_id) {
    
    std::string text_str = text.toStdString(); // перехваченное сообщение
    std::string chat_id_str = std::to_string(chat_id); // id чата
    
    //std::string new_text_str = get_command_result(text_str, chat_id_str); // сообщение на отправку
    std::string new_text_str = "";

    if (new_text_str == "") { // в тексте отсутсвуют команды (сообщение должно быть зашифровано)
        try {
            KeysDataBase db;
            std::string key_aes = db.get_active_param_text(chat_id_str, DbTablesDefs::AES, AesColumnsDefs::SESSION_KEY);

            AesKeyManager aes_manager;
            new_text_str = aes_manager.encrypt_message(text_str, key_aes);
        }
        catch (const std::exception& e) {
            new_text_str = text_str;
        }
    }

    std::cout << '\n';
    std::cout << "Начальная строка: " << text_str << '\n';
    std::cout << "Id пользователя: " << chat_id_str << '\n';
    std::cout << "Изменённая строка: " << new_text_str << '\n';
    std::cout << '\n';

    return QString::fromStdString(new_text_str);
}