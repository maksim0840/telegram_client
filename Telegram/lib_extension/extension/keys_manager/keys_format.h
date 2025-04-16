#include <stdexcept>
#include <string>
#include <vector> 
#include <openssl/buffer.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

#pragma once

namespace ext {

class Base64Format {
private:
    BIO* bio;               // буффер для записи результата кодирования
    BIO* b64;               // буффер для кодирования
    BUF_MEM* bufferPtr;     // структура с информацией о буффере

    void fill(); // создание буферов и их настройка
    void clear(); // освобождение ресурсов

public:
    Base64Format();
    ~Base64Format();
    
    // Функция для кодирования в формат Base64
    std::string encode_to_base64(const std::vector<unsigned char>& data);

    // Функция для декодирования из формата Base64
    std::vector<unsigned char> decode_from_base64(const std::string& data);
};

} // namespace ext
