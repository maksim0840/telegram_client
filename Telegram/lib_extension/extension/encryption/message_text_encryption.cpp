#include "message_text_encryption.h"

QString encrypt_the_message(const QString& text, const quint64 chat_id) {
    
    std::string text_str = text.toStdString(); // перехваченное сообщение
    std::string chat_id_str = std::to_string(chat_id); // id чата
    std::string new_text_str = text_str;

    
    int rsa_key_len = 2048;
    bool dh_fastmode = true;
    if (text == "[/start_rsa_aes]") { // начать формирование общего ключа
        ChatCommandsManager commands;
        new_text_str = commands.start_rsa(chat_id_str, rsa_key_len, dh_fastmode);
    }

    std::cout << '\n';
    std::cout << "Начальная строка: " << text_str << '\n';
    std::cout << "Id пользователя: " << chat_id_str << '\n';
    std::cout << "Изменённая строка: " << new_text_str << '\n';
    std::cout << '\n';

    return QString::fromStdString(new_text_str);
}