#include "message_text_decryption.h"
#include "../commands_manager/chat_key_creation.h"
#include "base/basic_types.h"
#include "styles/style_info.h"
#include "mtproto/core_types.h"
#include "scheme.h"
#include <zlib.h>

#pragma once

class Recieve {
private:
    // Позиции элементов из mtpBuffer
    static const int PAYLOAD_LEN_POSITION = 7; // последний элемент вне Payload
    // Секция Payload (позиции указаны для сообщений, которые отправляются напрямую)
    static const int REQUEST_TYPE_POSITION = 8;
    static const int USER_ID_FIRST_POSITION = 11; // (id отправителя)
    static const int USER_ID_SECOND_POSITION = 12; // (id отправителя)
    static const int CHAT_ID_FIRST_POSITION = 13;
    static const int CHAT_ID_SECOND_POSITION = 14;
    static const int USER_MESSAGE_POSITION = 13;
    static const int CHAT_MESSAGE_POSITION = 15;

    // Дополнения к значениям типов чата (нужны для определения id чата из буфера) - дополнения установлены самим телеграммом, чтобы не путать их типы
    static const uint64_t CHAT_TYPE_VALUE = 0x0001000000000000;

    // Если сообщение отправлено не напрямую (а запаковано), то получаем данные из ungzip_data со смещением назад
    static const int GZIP_BIAS = -8;
    static const uint32_t CONTAINER_BIAS = 6;

    // Маска младшего байта
    static const uint32_t LEAST_BYTE_MASK = 0xFF;

public:
    // using mtpBuffer = QVector<mtpPrime>;
    static void decrypt_the_buffer(mtpBuffer& buffer, mtpBuffer& ungzip_data);
};


class Gzip {
public:
    static mtpBuffer gzip(const mtpPrime *from, const mtpPrime *end) {
        mtpBuffer result;
    
        z_stream stream;
        stream.zalloc = 0;
        stream.zfree = 0;
        stream.opaque = 0;
        if (deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 16 + MAX_WBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
            return result;
        }
    
        mtpBuffer input(from, end);
        stream.avail_in = input.size() * sizeof(mtpPrime);
        stream.next_in = reinterpret_cast<Bytef*>(input.data());
    
        result.resize(input.size() * 2); // Предварительно выделяем место
        stream.avail_out = result.size() * sizeof(mtpPrime);
        stream.next_out = reinterpret_cast<Bytef*>(result.data());
    
        int res = deflate(&stream, Z_FINISH);
        if (res != Z_STREAM_END) {
            deflateEnd(&stream);
            return mtpBuffer();
        }
    
        result.resize(result.size() - (stream.avail_out / sizeof(mtpPrime)));
        deflateEnd(&stream);
    
        return result;
    }
};