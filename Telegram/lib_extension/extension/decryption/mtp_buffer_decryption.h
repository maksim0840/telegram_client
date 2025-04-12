#include "../commands_manager/chat_key_creation.h"
#include "base/basic_types.h"
#include "styles/style_info.h"
#include "mtproto/core_types.h"
#include "scheme.h"
#include <zlib.h>

#pragma once

class Receive {
private:
    // Позиции элементов из mtpBuffer
    static const int PAYLOAD_LEN_POSITION = 7; // последний элемент вне Payload для входящего буфера

    // Смещение, занимаемое объявлением контейнера, если сообщений в 1 response несколько
    static const int MSG_CONTAINER_BIAS = 6;
    static const int MESSAGES_MESSAGESSLICE_BIAS = 6;

    // Дополнения к значениям типов чата (нужны для определения id чата из буфера) - дополнения установлены самим телеграммом, чтобы не путать их типы
    static const uint64_t CHAT_TYPE_VALUE = 0x0001000000000000;

    // Маска младшего байта
    static const uint32_t LEAST_BYTE_MASK = 0xFF;

public:
    // using mtpBuffer = QVector<mtpPrime>;
    static void decrypt_the_buffer(mtpBuffer& buffer, mtpBuffer& ungzip_data);

    static std::string decrypt_the_message(const std::string& msg, std::string chat_id_str, std::string sender_id_str);

private:
    struct Positions {
        int REQUEST_TYPE = 8; // тип сообщения
        int CHAT_TYPE = 8; // тип чата (где-то указан в типе сообщения, а где-то отдельно)
        int USER_ID_FIRST = 11; // (id отправителя)
        int USER_ID_SECOND = 12; // (id отправителя)
        int CHAT_ID_FIRST = 13; // (id чата)
        int CHAT_ID_SECOND = 14; // (id чата)
        int USER_MESSAGE = 13; // (начало сообщения, если оно отправлено в личной беседе)
        int CHAT_MESSAGE = 15; // (начало сообщения, если оно отправлено в чате)

        void fill_by_gzip_packed() {
            REQUEST_TYPE = 0;
            CHAT_TYPE = 0;
            USER_ID_FIRST = 3;
            USER_ID_SECOND = 4;
            CHAT_ID_FIRST = 5;
            CHAT_ID_SECOND = 6;
            USER_MESSAGE = 5;
            CHAT_MESSAGE = 7;
        }
        void fill_by_rpc_result() {
            REQUEST_TYPE = 0;
            CHAT_TYPE = 7;
            USER_ID_FIRST = 5;
            USER_ID_SECOND = 6;
            CHAT_ID_FIRST = 8;
            CHAT_ID_SECOND = 9;
            USER_MESSAGE = 8;
            CHAT_MESSAGE = 11;
        }
        void fill_by_msg_container() {
            REQUEST_TYPE += MSG_CONTAINER_BIAS;
            CHAT_TYPE += MSG_CONTAINER_BIAS;
            USER_ID_FIRST += MSG_CONTAINER_BIAS;
            USER_ID_SECOND += MSG_CONTAINER_BIAS;
            CHAT_ID_FIRST += MSG_CONTAINER_BIAS;
            CHAT_ID_SECOND += MSG_CONTAINER_BIAS;
            USER_MESSAGE += MSG_CONTAINER_BIAS;
            CHAT_MESSAGE += MSG_CONTAINER_BIAS;
        }
        void fill_by_messages_messagesSlice() {
            REQUEST_TYPE += MESSAGES_MESSAGESSLICE_BIAS;
            CHAT_TYPE += MESSAGES_MESSAGESSLICE_BIAS;
            USER_ID_FIRST += MESSAGES_MESSAGESSLICE_BIAS;
            USER_ID_SECOND += MESSAGES_MESSAGESSLICE_BIAS;
            CHAT_ID_FIRST += MESSAGES_MESSAGESSLICE_BIAS;
            CHAT_ID_SECOND += MESSAGES_MESSAGESSLICE_BIAS;
            USER_MESSAGE += MESSAGES_MESSAGESSLICE_BIAS;
            CHAT_MESSAGE += MESSAGES_MESSAGESSLICE_BIAS;
        }
    };
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