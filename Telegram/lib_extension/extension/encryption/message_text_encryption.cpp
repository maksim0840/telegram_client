#include "message_text_encryption.h"

QString encrypt_the_message(const QString& text, const quint64 chat_id, const quint64 my_id) {
    
    std::string text_str = text.toStdString(); // перехваченное сообщение
    std::string chat_id_str = std::to_string(chat_id); // id чата
    std::string my_id_str = std::to_string(my_id);
    std::string new_text_str = text_str;

    KeysDataBase db;
    std::optional<std::string> session_key = db.get_active_param_text(chat_id_str, KeysTablesDefs::AES, AesColumnsDefs::SESSION_KEY);

    AesKeyManager aes_manager;

    int rsa_key_len_default = 2048;
    bool dh_fastmode_default = true;
    std::regex re_options(R"(\[/start_encryption-(\d+)-([01])\])");
    std::smatch match;

    // Подставляем дополнительные опции, если они есть
    if (std::regex_match(text_str, match, re_options)) {
        rsa_key_len_default = std::stoi(match[1].str());
        dh_fastmode_default = std::stoi(match[2].str());
        text_str = "[/start_encryption]";
    }
    // Запускаем процесс шифрования
    if (text_str == "[/start_encryption]") { // начать формирование общего ключа
        ChatCommandsManager commands;
        new_text_str = commands.start_rsa(chat_id_str, my_id_str, rsa_key_len_default, dh_fastmode_default);
    }
    // else if (session_key) {
    //     std::cout << *session_key << '\n';
    //     Message message;
    //     message.text = aes_manager.encrypt_message(new_text_str, *session_key);
    //     new_text_str = message.get_text_with_options();
    // }

    std::cout << '\n';
    std::cout << "Начальная строка: " << text_str << '\n';
    std::cout << "Id пользователя: " << chat_id_str << '\n';
    std::cout << "Изменённая строка: " << new_text_str << '\n';
    std::cout << '\n';

    return QString::fromStdString(new_text_str);
}