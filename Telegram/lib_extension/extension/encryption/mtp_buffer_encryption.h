#include "styles/style_info.h"
#include "../commands_manager/chat_commands_manager.h"

#define MESSAGE_TYPE_POSITION 10
#define REQUEST_TYPE_POSITION 8
#define PAYLOAD_LEN_POSITION 7

// Количество элементов в конце запроса, которые идут после самого сообщения
#define ENDING_REQUEST_ELEMENTS 2
// Количество элементво в запросе, которые не входят в Payload
#define NOT_PAYLOAD_ELEMENTS 8

// using mtpBuffer = QVector<mtpPrime>;
void encrypt_the_buffer(mtpBuffer& buffer) {

    const int buffer_len = buffer.size();
    const int buffer_element_size = sizeof(mtpPrime); // размер блока/элемента внутри mtpBuffer

    if (buffer.size() < 16) {
        return;
    }

    // Определяем тип чата
    const uint32_t chat_type = static_cast<uint32_t>(buffer[MESSAGE_TYPE_POSITION]);
    if (!(chat_type == mtpc_inputPeerChat || chat_type == mtpc_inputPeerUser)) {
        return;
    }

    // Определяем тип запроса на сервер
    const uint32_t request_type = static_cast<uint32_t>(buffer[REQUEST_TYPE_POSITION]);
    std::cout << "!!!!!!!!!!!!!" << std::hex << request_type << std::dec << ' ' << (request_type==mtpc_messages_sendMessage) << "!!!!!!!!!" << '\n';
    if (request_type != mtpc_messages_sendMessage) {
        return;
    }

    // Находим границу последнего блока, где содержаться сообщения
    unsigned int end_of_message_bytes = (buffer_len - ENDING_REQUEST_ELEMENTS) * buffer_element_size - 1;

    // Находим последний значащий байт сообщения из последнего блока
    unsigned int byte_mask = 0xFF000000; // маска старшего байта
    unsigned int last_block = static_cast<uint32_t>(buffer[buffer_len - 1 - ENDING_REQUEST_ELEMENTS]); // значение последний блока, содержащее сообщение
    while ((last_block & byte_mask) == 0) {
        byte_mask = byte_mask >> 8;
        --end_of_message_bytes;
    }

    // Находим начало сообщения
    unsigned int start_of_message_bytes;
    byte_mask = 0xFF; // маска младшего байта
    unsigned int first_block;

    if (chat_type == mtpc_inputPeerChat) {
        first_block = static_cast<uint32_t>(buffer[13]);
        start_of_message_bytes = 13 * buffer_element_size;
    }
    else if (chat_type == mtpc_inputPeerUser) {
        first_block = static_cast<uint32_t>(buffer[15]);
        start_of_message_bytes = 15 * buffer_element_size;
    }

    // Извлекаем длинну сообщения (которая находится в начале)
    unsigned int message_len;
    if ((first_block & byte_mask) == 0xFE) { // 0xFE - флаг, что длинна сообщения превышает 0xFE
        // Длинное сообщение (под длинну выделен отдельный элемент буфера)
        message_len = (first_block & (~byte_mask)) >> 8; // берём всё кроме флага 
        start_of_message_bytes += buffer_element_size;
    }
    else {
        // Короткое сообщение (длинна находится в первом байте сообщения)
        message_len = first_block & byte_mask;
        start_of_message_bytes += 1;
    }

    // Извлекаем сообщение
    std::string message;
    unsigned int cur_block_ind = start_of_message_bytes / buffer_element_size;

    for (int i = start_of_message_bytes; i < end_of_message_bytes + 1; ++i) {
        if ((i % buffer_element_size == 0) && (i != start_of_message_bytes)) {
            ++cur_block_ind;
        }

        unsigned int cur_block = static_cast<uint32_t>(buffer[cur_block_ind]);
        unsigned int mask_move = (i % buffer_element_size) * 8;
        char cur_byte = static_cast<char>((cur_block & (byte_mask << mask_move)) >> mask_move);

        message.push_back(cur_byte);
    }

    // Шифруем сообщение
    AesKeyManager aes_manager;
    std::string key_test = "HYyt4p88dZFNhQ4Z+9LOUZqI/m17Arp/MZh76yMj3E4=";
    std::string encrypted_message = aes_manager.encrypt_message(message, key_test);

    // Подменяем сообщение (и его длинну) в новом буфере
    unsigned int encrypted_message_len = encrypted_message.size();
    mtpPrime save_last = buffer[buffer_len - 1]; // сохраняем последнее поле
    mtpPrime save_prelast = buffer[buffer_len - 2]; // сохраняем предпоследнее поле
    
    // Определяем стартовый индекс (с которого пойдёт новая инфомрация)
    unsigned int start_ind;
    if (chat_type == mtpc_inputPeerChat) {
        start_ind = 13;
    }
    else if (chat_type == mtpc_inputPeerUser) {
        start_ind = 15;
    }
    
     // Удаляем инофрмацию, которая будет заменена
     buffer.erase(buffer.begin() + start_ind, buffer.end());

    unsigned int cur_encrypted_message_ind = 0;

    // Записываем длинну
    if (encrypted_message_len >= 0xFE) {
        mtpPrime value = (0xFE | (encrypted_message_len << 8));
        buffer.push_back(value);
    }
    else {
        mtpPrime value = encrypted_message_len |
           (static_cast<mtpPrime>(static_cast<uint8_t>(encrypted_message[2])) << 24) |
           (static_cast<mtpPrime>(static_cast<uint8_t>(encrypted_message[1])) << 16) |
           (static_cast<mtpPrime>(static_cast<uint8_t>(encrypted_message[0])) << 8);

           buffer.push_back(value);
        cur_encrypted_message_ind = 3;
    }

    // Записываем оставшуюся часть сообщения
    for (int i = cur_encrypted_message_ind; i < encrypted_message_len; i += 4) {
        mtpPrime value = 0;

        for (int j = 0; (j < 4) && (i + j < encrypted_message_len); ++j) {
            value = value | (static_cast<mtpPrime>(static_cast<uint8_t>(encrypted_message[i + j])) << (j * 8));
        }
        buffer.push_back(value);
    }

    // Возращаем ранее удалённые конечные значения
    buffer.push_back(save_prelast);
    buffer.push_back(save_last);

    // Изменим длинну Payload
    buffer[PAYLOAD_LEN_POSITION] = (buffer.size() - NOT_PAYLOAD_ELEMENTS) * buffer_element_size;

    std::cout << "start_of_message_bytes: " << start_of_message_bytes << '\n';
    std::cout << "end_of_message_bytes: " << end_of_message_bytes << '\n';
    std::cout << "message_len: " << message_len << '\n';
    std::cout << "message: " << message << '\n';
    std::cout << "encrypted_message: " << encrypted_message << '\n';

}