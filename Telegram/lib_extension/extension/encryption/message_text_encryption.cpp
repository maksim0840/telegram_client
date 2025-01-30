#include "message_text_encryption.h"
#include <openssl/evp.h>
#include <openssl/rand.h>

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


std::vector<unsigned char> aes256_encrypt(const std::vector<unsigned char>& key_aes, const std::string& plaintext) {
    if (key_aes.size() != 32) {
        throw std::runtime_error("Ошибка: Неверный размер ключа AES. Должен быть 256 бит (32 байта).");
    }

    std::vector<unsigned char> iv(16);
    if (!RAND_bytes(iv.data(), iv.size())) {
        throw std::runtime_error("Ошибка: Не удалось сгенерировать IV.");
    }

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Ошибка: Не удалось создать контекст шифрования.");
    }

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key_aes.data(), iv.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Ошибка: Не удалось инициализировать шифрование AES-256 CBC.");
    }

    std::vector<unsigned char> ciphertext(plaintext.size() + 16);
    int len = 0, ciphertext_len = 0;

    if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len, reinterpret_cast<const unsigned char*>(plaintext.data()), plaintext.size()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Ошибка: Не удалось зашифровать данные.");
    }
    ciphertext_len += len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + ciphertext_len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error("Ошибка: Не удалось выполнить финальную стадию шифрования.");
    }
    ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);

    ciphertext.resize(ciphertext_len);
    ciphertext.insert(ciphertext.begin(), iv.begin(), iv.end());

    return ciphertext;
}


QString encrypt_the_message(const QString& text, const quint64 chat_id) {
    
    std::string text_str = text.toStdString();
    std::string chat_id_str = std::to_string(chat_id);
    std::string new_text_str = get_command_result(text_str, chat_id_str);
    
    if (new_text_str == "") {
        try {
            KeysDataBase db;
            std::string key_aes_base64 = db.get_active_param_text(chat_id_str, DbTablesDefs::AES, AesColumnsDefs::SESSION_KEY);

            Base64Format format;
            std::vector<unsigned char> key_aes = format.decode(key_aes_base64);
            std::vector<unsigned char> text_by_aes = aes256_encrypt(key_aes, text_str);

            new_text_str = format.encode(text_by_aes);
        } catch (const std::exception& e) {
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