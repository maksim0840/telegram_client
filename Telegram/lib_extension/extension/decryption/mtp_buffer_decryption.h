#include "../commands_manager/chat_key_creation.h"
#include "base/basic_types.h"
#include "styles/style_info.h"
#include "mtproto/core_types.h"
#include "scheme.h"
#include <zlib.h>

#pragma once

namespace ext {

class Receive {
private:
    // Позиции элементов из mtpBuffer
    static const int PAYLOAD_LEN_POSITION = 7; // последний элемент вне Payload для входящего буфера

    // Смещение, занимаемое объявлением контейнера, если сообщений в 1 response несколько
    static const int MSG_CONTAINER_BIAS = 6;
    static const int MESSAGES_MESSAGESSLICE_BIAS = 6;
    static const int MESSAGES_DIALOGS_BIAS = 3;
    static const int UPDATES_BIAS = 8;

    // Дополнения к значениям типов чата (нужны для определения id чата из буфера) - дополнения установлены самим телеграммом, чтобы не путать их типы
    static const uint64_t CHAT_TYPE_VALUE = 0x0001000000000000;

    // Маска младшего байта
    static const uint32_t LEAST_BYTE_MASK = 0xFF;

    struct Positions {
        int REQUEST_ID_FIRST = -1; // id запроса (только для fill_by_check_id)
        int REQUEST_ID_SECOND = -1; // id запроса (только для fill_by_check_id)
        int MESSAGE_ID_SELF = -1; // id личного сообщения (только для fill_by_check_id)

        int REQUEST_TYPE = 8; // тип сообщения 
        int CHAT_TYPE = 8; // тип чата (где-то указан в типе сообщения, а где-то отдельно)
        int MESSAGE_ID = 10; // id сообщения
        int USER_ID_FIRST = 11; // (id отправителя)
        int USER_ID_SECOND = 12; // (id отправителя)
        int CHAT_ID_FIRST = 13; // (id чата)
        int CHAT_ID_SECOND = 14; // (id чата)
        int USER_MESSAGE = 13; // (начало сообщения, если оно отправлено в личной беседе)
        int CHAT_MESSAGE = 15; // (начало сообщения, если оно отправлено в чате)
        
        void fill_by_gzip_packed() {
            REQUEST_TYPE = 0;
            CHAT_TYPE = 0;
            MESSAGE_ID = 2;
            USER_ID_FIRST = 3;
            USER_ID_SECOND = 4;
            CHAT_ID_FIRST = 5;
            CHAT_ID_SECOND = 6;
            USER_MESSAGE = 5;
            CHAT_MESSAGE = 7;
        }
        void fill_by_rpc_result() {
            REQUEST_TYPE = 0;
            MESSAGE_ID = 3;
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
            MESSAGE_ID += MSG_CONTAINER_BIAS;
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
            MESSAGE_ID += MESSAGES_MESSAGESSLICE_BIAS;
            CHAT_TYPE += MESSAGES_MESSAGESSLICE_BIAS;
            USER_ID_FIRST += MESSAGES_MESSAGESSLICE_BIAS;
            USER_ID_SECOND += MESSAGES_MESSAGESSLICE_BIAS;
            CHAT_ID_FIRST += MESSAGES_MESSAGESSLICE_BIAS;
            CHAT_ID_SECOND += MESSAGES_MESSAGESSLICE_BIAS;
            USER_MESSAGE += MESSAGES_MESSAGESSLICE_BIAS;
            CHAT_MESSAGE += MESSAGES_MESSAGESSLICE_BIAS;
        }
        void fill_by_messages_dialogs() {
            REQUEST_TYPE += MESSAGES_DIALOGS_BIAS;
            MESSAGE_ID += MESSAGES_DIALOGS_BIAS;
            CHAT_TYPE += MESSAGES_DIALOGS_BIAS;
            USER_ID_FIRST += MESSAGES_DIALOGS_BIAS;
            USER_ID_SECOND += MESSAGES_DIALOGS_BIAS;
            CHAT_ID_FIRST += MESSAGES_DIALOGS_BIAS;
            CHAT_ID_SECOND += MESSAGES_DIALOGS_BIAS;
            USER_MESSAGE += MESSAGES_DIALOGS_BIAS;
            CHAT_MESSAGE += MESSAGES_DIALOGS_BIAS;
        }
        void fill_by_updates() {
            REQUEST_TYPE += UPDATES_BIAS;
            MESSAGE_ID += UPDATES_BIAS;
            CHAT_TYPE += UPDATES_BIAS;
            USER_ID_FIRST += UPDATES_BIAS;
            USER_ID_SECOND += UPDATES_BIAS;
            CHAT_ID_FIRST += UPDATES_BIAS;
            CHAT_ID_SECOND += UPDATES_BIAS;
            USER_MESSAGE += UPDATES_BIAS;
            CHAT_MESSAGE += UPDATES_BIAS;
        }
        void fill_by_check_id() {
            REQUEST_ID_FIRST = 9;
            REQUEST_ID_SECOND = 10;
            REQUEST_TYPE = 11;
            MESSAGE_ID = 13;
            MESSAGE_ID_SELF = 15;
        }
    };

public:
    // using mtpBuffer = QVector<mtpPrime>;
    static void decrypt_the_buffer(mtpBuffer& buffer, std::function<mtpBuffer(const mtpPrime*, const mtpPrime*)> ungzip_lambda);

    static std::string decrypt_the_message(const std::string& msg, const std::string& msg_id, std::string chat_id_str, std::string sender_id_str, const bool is_recieved);

    // При ОТПРАВКЕ сообщения, его id неизвестно, т.к. его устанавливает сервер. Здесь мы проверяем ответ от сервера о назначении id для сообщения
    static void check_id_for_accepted_messages(const mtpBuffer& buf);
};


class Gzip {
public:
    static mtpBuffer gzip(const mtpPrime *from, const mtpPrime *end);

private:
    static QByteArray wrap_mtp_string(const QByteArray &data);
};

} // namespace ext