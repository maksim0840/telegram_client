#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/bn.h> 
#include <openssl/err.h>
#include <openssl/rand.h>
#include <utility>
#include "keys_format.h"
#include <array>
#include "dh.h"

#define AES_KEY_LEN 256

#pragma once

namespace ext {

class RsaKeyManager {
private:
    RSA* rsa;             // структура для хранения частей RSA ключей
    BIGNUM* bn;           // большое число для проведения операций
    BIO* public_bio;      // поток для вывода публичного ключа
    BIO* private_bio;     // поток для вывода приватного ключа
    Base64Format format_converter;

    void fill(); // создание необходимых структур для генерации ключей
    void clear(); // освобождение ресурсов
    std::string extract_bio_data(BIO* bio); // извлечение данных из bio в строку

public:
    RsaKeyManager();
    ~RsaKeyManager();

    // Генерация ключей RSA заданной длины и их возврат в паре (публичный, приватный ключ)
    std::pair<std::string, std::string> create_key(const int key_len = 2048);
    // Шифрует сообщения по public RSA ключу
    std::string encrypt_message(const std::string& message, const std::string& public_key);
    // Расшифровывает сообщения по private RSA ключу
    std::string decrypt_message(const std::string& message, const std::string& private_key);
};


class AesKeyManager {
private:
    Base64Format format_converter;

public:
    // Получить ключ для личных сообщений
    std::string create_key_solo();
    // Получить ключ для двух собеседников
    std::string create_key_duo(const DHParamsStr& my_params, const std::string& other_public_key);
    // Постепенно сфорировать ключ для нескольких собеседников
    std::string сreate_key_multi(const DHParamsStr& my_params, const std::string& other_public_key, bool is_final = false);

    // Сгенерировать параметры для алгоритм dh
    DHParamsStr get_dh_params(bool fast_mode = true, const int p_length = 2048, const int g_value = DH_GENERATOR_2);
    // Сгенерировать параметры для алгоритм dh на основе общедоступных параметров собеседника
    DHParamsStr get_dh_params_secondly(const std::string p, const std::string g);

    // Шифрует сообщения по AES-256 ключу
    std::string encrypt_message(const std::string& message, const std::string& key);
    // Расшифровывает сообщения по AES-256 ключу
    std::string decrypt_message(const std::string& message, const std::string& key);
};

} // namespace ext