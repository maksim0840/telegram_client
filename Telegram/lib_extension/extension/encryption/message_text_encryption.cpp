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
    std::pair<std::string, std::string> rsa1 = rsa_key_creator.generate();
    std::pair<std::string, std::string> rsa2 = rsa_key_creator.generate();
    std::string aes1 = aes_key_creator.generate();
    std::string aes2 = aes_key_creator.generate();

    std::cout << "rsa1:" << rsa1.first << ' ' << rsa1.second << '\n';
    std::cout << "rsa2:" << rsa2.first << ' ' << rsa2.second << '\n';
    std::cout << "aes1:" << aes1 << '\n';
    std::cout << "aes2:" << aes2 << '\n';


    KeysDataBase db_keys;
    AesParamsFiller aes_params = { .chat_id = "123chat123", .session_key = "kasdfgkjcxv23dsfj2", .initiator = 1 };
    db_keys.add_aes_key(aes_params);

    RsaParamsFiller rsa_params = { .chat_id = "123chat123", .key_len = 5, .total_members = 2, .private_key = "kkjjj", .initiator = 1 };
    db_keys.add_rsa_key(rsa_params);

    std::cout << db_keys.get_active_param_text("123chat123", DbTablesDefs::AES, AesColumnsDefs::NODE_ID, 1);

    RsaParamsFiller rsa_params_more = {.chat_id = "123chat123", .members_public_keys = {"1213"}, .members_ids = {"llkjlsakjdflkjlkj"}};
    db_keys.add_rsa_member_key(rsa_params_more);

 
    return QString::fromStdString(new_text);
}