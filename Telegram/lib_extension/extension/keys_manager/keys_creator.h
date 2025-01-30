#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/bn.h> 
#include <openssl/err.h>
#include <openssl/rand.h>
#include <utility>
#include "keys_format.h"

#define AES_KEY_LEN 256

#pragma once

class RsaKeyCreator {
private:
    RSA* rsa;             // структура для хранения частей RSA ключей
    BIGNUM* bn;           // большое число для проведения операций
    BIO* public_bio;      // поток для вывода публичного ключа
    BIO* private_bio;     // поток для вывода приватного ключа
    Base64Format encoder;

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
private:
    Base64Format encoder;

public:
    std::string generate();

    std::vector<unsigned char> encrypt(const std::vector<unsigned char>& key_aes, const std::string& plaintext) {
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
};