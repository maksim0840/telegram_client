#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <string>
#include <utility>
#include <stdexcept>
#define AES_KEY_LEN 256

#pragma once


class RsaKeyCreator {
private:
    RSA* rsa;             // структура для хранения частей RSA ключей
    BIGNUM* bn;           // большое число для проведения операций
    BIO* public_bio;      // поток для вывода публичного ключа
    BIO* private_bio;     // поток для вывода приватного ключа

    void fill(); // создание необходимых структур для генерации ключей
    void clear(); // освобождение ресурсов
    std::string extract_bio_data(BIO* bio); // извлечение данных из bio в строку

public:
    RsaKeyCreator();
    ~RsaKeyCreator();

    // Генерация ключей RSA заданной длины и их возврат в паре (публичный, приватный ключ)
    std::pair<std::string, std::string> generate(const int key_len = 2048);
};


class AesKeyCreator {
public:
    std::string generate();
};