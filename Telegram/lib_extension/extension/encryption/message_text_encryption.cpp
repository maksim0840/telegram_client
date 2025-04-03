#include "message_text_encryption.h"

QString encrypt_the_message(const QString& text, const quint64 chat_id, const quint64 my_id) {
    
    std::string text_str = text.toStdString(); // перехваченное сообщение
    std::string chat_id_str = std::to_string(chat_id); // id чата
    std::string my_id_str = std::to_string(my_id);
    std::string new_text_str = text_str;

    // std::optional<std::string> session_key = db.get_active_param_text(chat_id_str, KeysTablesDefs::AES, AesColumnsDefs::SESSION_KEY);


    std::cout << '\n';
    std::cout << "Начальная строка: " << text_str << '\n';
    std::cout << "Id пользователя: " << chat_id_str << '\n';
    std::cout << "Изменённая строка: " << new_text_str << '\n';
    std::cout << '\n';

    return QString::fromStdString(new_text_str);
}