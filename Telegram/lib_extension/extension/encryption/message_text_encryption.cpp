#include "message_text_encryption.h"

QString encrypt_the_message(const QString& text, const quint64 peer_id) {
    std::string new_text = "___!!!encrypted message!!!___";

    std::cout << '\n';
    std::cout << "Начальная строка: " << text.toStdString() << '\n';
    std::cout << "Id пользователя: " << peer_id << '\n';
    std::cout << "Изменённая строка: " << new_text << '\n';
    std::cout << '\n';

    RsaKeyCreator rsa_key_creator;
    AesKeyCreator aes_key_creator;
    std::string rsa1 = rsa_key_creator.generate() << '\n';
    std::string rsa2 = rsa_key_creator.generate() << '\n';
    std::string aes1 = aes_key_creator.generate() << '\n';
    std::string aes2 = aes_key_creator.generate() << '\n';

    std::cout << "rsa1:" << rsa1 << '\n';
    std::cout << "rsa2:" << rsa2 << '\n';
    std::cout << "aes1:" << aes1 << '\n';
    std::cout << "aes2:" << aes2 << '\n';

    return QString::fromStdString(new_text);
}