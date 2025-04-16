#include "keys_format.h"

namespace ext {

/* class Base64Format */

void Base64Format::fill() {
    b64 = BIO_new(BIO_f_base64());
    if (!b64) throw std::runtime_error("Ошибка создания BIO_f_base64");
    
    bio = BIO_new(BIO_s_mem());
    if (!bio) throw std::runtime_error("Ошибка создания BIO_s_mem");

    bio = BIO_push(b64, bio); // связывание буфферов (перед записью в bio данные проходят шифровку в b64)
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // отключение переноса на новую строку
}

void Base64Format::clear() {
    if (bio) {
        BIO_free_all(bio);
        bio = nullptr;
    }
    bufferPtr = nullptr;
}

Base64Format::Base64Format() {
    bio = nullptr;
    b64 = nullptr;
    bufferPtr = nullptr;
}

Base64Format::~Base64Format() {
    clear();
}

std::string Base64Format::encode_to_base64(const std::vector<unsigned char>& data) {
    fill();

    // Запись данных в буфер bio (предварительно кодируясь через b64)
    if (BIO_write(bio, data.data(), data.size()) <= 0) {
        throw std::runtime_error("Ошибка записи в BIO");
    }

    // Завершить процесс записи данных
    if (BIO_flush(bio) != 1) {
        clear();
        throw std::runtime_error("Ошибка при завершении записи в BIO");
    }

    // Извлекаем данные в структуру bufferPtr
    BIO_get_mem_ptr(bio, &bufferPtr);
    if (!bufferPtr || !bufferPtr->data) {
        throw std::runtime_error("Ошибка получения данных из BIO");
    }

    std::string result(bufferPtr->data, bufferPtr->length); // получаем данные из bufferPtr в строку
    clear();
    return result;
}

std::vector<unsigned char> Base64Format::decode_from_base64(const std::string& data) {
    std::vector<unsigned char> result(data.size());


    BIO* bio = BIO_new_mem_buf(data.data(), data.size()); // буфер входных данных
    BIO* b64 = BIO_new(BIO_f_base64()); // буфер для декодирования

    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // отключение переноса на новую строку
    bio = BIO_push(b64, bio); // связывание буфферов (перед записью в bio данные проходят шифровку в b64)

    // Получение информации из буфера
    int result_len = BIO_read(bio, result.data(), result.size());
    if (result_len > 0) {
        result.resize(result_len);
    }
    else {
        result.clear();
    }

    BIO_free_all(b64);
    return result;
}

} // namespace ext