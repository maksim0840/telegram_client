#include "message_text_decryption.h"
#include "base/basic_types.h"
#include "styles/style_info.h"
#include "mtproto/core_types.h"
#include "scheme.h"

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

    // Маска младшего байта
    static const uint32_t LEAST_BYTE_MASK = 0xFF;

public:
    // using mtpBuffer = QVector<mtpPrime>;
    static void decrypt_the_buffer(mtpBuffer& buffer, mtpBuffer& ungzip_data);
};