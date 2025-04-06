#include "../commands_manager/chat_key_creation.h"
#include "base/basic_types.h"
#include "styles/style_info.h"
#include "mtproto/core_types.h"
#include "scheme.h"

#pragma once

class Send {
private:
    // Позиции элементов из mtpBuffer
    static const uint32_t PAYLOAD_LEN_POSITION = 7; // последний элемент вне Payload
    static const uint32_t REQUEST_TYPE_POSITION = 8;

    // Позиции относительно найденного типа запроса (REQUEST_TYPE_POSITION)
    static const uint32_t CHAT_TYPE_POSITION = 2;
    static const uint32_t CHAT_ID_FIRST_POSITION = 3;
    static const uint32_t CHAT_ID_SECOND_POSITION = 4;
    static const uint32_t SELF_MESSAGE_POSITION = 3;
    static const uint32_t CHAT_MESSAGE_POSITION = 5;
    static const uint32_t USER_MESSAGE_POSITION = 7;

    // Дополнения к значениям типов чата (нужны для определения id чата из буфера) - дополнения установлены самим телеграммом, чтобы не путать их типы
    static const uint64_t CHAT_TYPE_VALUE = 0x0001000000000000;

    // Если сообщение отправлено не напрямую (а обёрнуто в контейнер), то все позиции из Payload дополнительно смещаются
    static const uint32_t CONTAINER_BIAS = 6;

    // Маски байтов
    static const uint32_t LEAST_BYTE_MASK = 0xFF;

    // Дополнительный флаг который всегда идёт после типа запроса
    static const uint32_t message_addition_flags = 0x80;
    
public:
    // using mtpBuffer = QVector<mtpPrime>;
    static void encrypt_the_buffer(mtpBuffer& buffer);
    
    static std::string encrypt_the_message(const std::string& msg, std::string chat_id_str);
};