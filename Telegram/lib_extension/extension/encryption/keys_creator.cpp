#include "keys_creator.h"

void RsaKeyCreator::fill() {
    rsa = RSA_new();
    if (!rsa) throw std::runtime_error("Ошибка создания RSA");

    bn = BN_new();
    if (!bn) throw std::runtime_error("Ошибка создания BIGNUM");

    public_bio = BIO_new(BIO_s_mem());
    if (!public_bio) throw std::runtime_error("Ошибка создания BIO для публичного ключа");

    private_bio = BIO_new(BIO_s_mem());
    if (!private_bio) throw std::runtime_error("Ошибка создания BIO для приватного ключа");
}

void RsaKeyCreator::clear() {
    if (rsa) RSA_free(rsa);
    if (bn) BN_free(bn);
    if (public_bio) BIO_free_all(public_bio);
    if (private_bio) BIO_free_all(private_bio);

    rsa = nullptr;
    bn = nullptr;
    public_bio = nullptr;
    private_bio = nullptr;
}

std::string RsaKeyCreator::extract_bio_data(BIO* bio) {
    char* data = nullptr;
    long length = BIO_get_mem_data(bio, &data);
    if (!data || length <= 0) {
        throw std::runtime_error("Ошибка извлечения данных из BIO");
    }
    return std::string(data, length);
}

RsaKeyCreator::RsaKeyCreator() {
    rsa = nullptr;
    bn = nullptr;
    public_bio = nullptr;
    private_bio = nullptr;
}

~RsaKeyCreator::RsaKeyCreator() {
    clear();
}

std::pair<std::string, std::string> RsaKeyCreator::generate(const int key_len) {
    if (key_len < 512 || key_len > 8192 || key_len % 64 != 0) {
        throw std::invalid_argument("Неподдерживаемая длина ключа RSA");
    }
    fill();

    // Устанавливаем начальное значение для генерации
    if (!BN_set_word(bn, RSA_F4)) {
        std::string error_message = "Ошибка установки BIGNUM: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        throw std::runtime_error(error_message);
    }

    // Заполняем структуру ключей
    if (!RSA_generate_key_ex(rsa, key_len, bn, nullptr)) {
        std::string error_message = "Ошибка генерации ключа: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        throw std::runtime_error(error_message);
    }

    // Получаем публичный ключ из буфера и преобразуем в строковый формат PEM
    if (!PEM_write_bio_RSAPublicKey(public_bio, rsa)) {
        std::string error_message = "Ошибка записи открытого ключа: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        throw std::runtime_error(error_message);
    }
    std::string public_key = extract_bio_data(public_bio);

    // Получаем приватный ключ из буфера и преобразуем в строковый формат PEM
    if (!PEM_write_bio_RSAPrivateKey(private_bio, rsa, nullptr, nullptr, 0, nullptr, nullptr)) {
        std::string error_message = "Ошибка записи закрытого ключа: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        throw std::runtime_error(error_message);
    }
    std::string private_key = extract_bio_data(private_bio);

    clear();
    return {public_key, private_key};
}


std::string AesKeyCreator::generate() {
    size_t key_len_bytes = AES_KEY_LEN / 8;
    unsigned char key[key_len_bytes];

    // Генерация рандомных байт в количестве key_len_bytes
    if (!RAND_bytes(key, key_len_bytes)) {
        std::string error_message = "Ошибка генерации ключа AES: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        throw std::runtime_error(error_message);
    }

    return std::string(reinterpret_cast<const char*>(key), key_len_bytes);
}