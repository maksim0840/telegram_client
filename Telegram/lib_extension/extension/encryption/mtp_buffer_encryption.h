#include "message_text_encryption.h"
#include "base/basic_types.h"
#include "styles/style_info.h"
#include "mtproto/core_types.h"
#include "scheme.h"

#pragma once

class Send {
private:
    // Позиции элементов из mtpBuffer
    static const uint32_t PAYLOAD_LEN_POSITION = 7; // последний элемент вне Payload
    // Секция Payload (позиции указаны для сообщений, которые отправляются напрямую)
    static const uint32_t REQUEST_TYPE_POSITION = 8;
    static const uint32_t CHAT_TYPE_POSITION = 10;
    static const uint32_t CHAT_ID_FIRST_POSITION = 11;
    static const uint32_t CHAT_ID_SECOND_POSITION = 12;
    static const uint32_t SELF_MESSAGE_POSITION = 11;
    static const uint32_t CHAT_MESSAGE_POSITION = 13;
    static const uint32_t USER_MESSAGE_POSITION = 15;

    // Дополнения к значениям типов чата (нужны для определения id чата из буфера) - дополнения установлены самим телеграммом, чтобы не путать их типы
    static const uint64_t CHAT_TYPE_VALUE = 0x0001000000000000;

    // Если сообщение отправлено не напрямую (а обёрнуто в контейнер), то все позиции из Payload дополнительно смещаются
    static const uint32_t CONTAINER_BIAS = 6;

    // Маска младшего байта
    static const uint32_t LEAST_BYTE_MASK = 0xFF;

public:
    // using mtpBuffer = QVector<mtpPrime>;
    static void encrypt_the_buffer(mtpBuffer& buffer);
};