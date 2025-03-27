#include "mtp_buffer_encryption.h"

void encrypt_the_buffer(mtpBuffer& buffer) {

    const int buffer_len = buffer.size();
    const int buffer_element_size = sizeof(mtpPrime); // размер блока/элемента внутри mtpBuffer

    if (buffer_len < CHAT_TYPE_POSITION + 1) {
        return;
    }

    // Определяем смещение внутри Payload
    uint32_t payload_bias = 0;
    if (buffer[REQUEST_TYPE_POSITION] == mtpc_msg_container) {
        payload_bias = CONTAINER_BIAS;
    }

    // Определяем тип чата
    const uint32_t chat_type = static_cast<uint32_t>(buffer[CHAT_TYPE_POSITION + payload_bias]);
    if (!(chat_type == mtpc_inputPeerChat || chat_type == mtpc_inputPeerUser || chat_type == mtpc_inputPeerSelf)) {
        return;
    }

    // Определяем id чата
    uint64_t chat_id = 0; // сообщение самому себе (mtpc_inputPeerSelf)
    if (chat_type == mtpc_inputPeerUser) {
        chat_id =   static_cast<uint32_t>(buffer[CHAT_ID_FIRST_POSITION + payload_bias]) |
                    (static_cast<uint64_t>(static_cast<uint32_t>(buffer[CHAT_ID_SECOND_POSITION + payload_bias])) << 32);
    }
    else if (chat_type == mtpc_inputPeerChat) {
        chat_id =   static_cast<uint32_t>(buffer[CHAT_ID_FIRST_POSITION + payload_bias]) |
                    (static_cast<uint64_t>(static_cast<uint32_t>(buffer[CHAT_ID_SECOND_POSITION + payload_bias])) << 32) |
                    CHAT_TYPE_VALUE;
    }
    std::string chat_id_str = std::to_string(chat_id);
    
    // Определяем тип запроса на сервер
    const uint32_t request_type = static_cast<uint32_t>(buffer[REQUEST_TYPE_POSITION + payload_bias]);
    std::cout << "!!!!!!!!!!!!!" << std::hex << request_type << std::dec << ' ' << (request_type==mtpc_messages_sendMessage) << "!!!!!!!!!" << '\n';
    if (request_type != mtpc_messages_sendMessage) {
        return;
    }

    // Находим начало сообщения
    uint32_t start_block_ind; // индекс блока, где начинается сообщение
    if (chat_type == mtpc_inputPeerChat) {
        start_block_ind = CHAT_MESSAGE_POSITION + payload_bias;
    }
    else if (chat_type == mtpc_inputPeerUser) {
        start_block_ind = USER_MESSAGE_POSITION + payload_bias;
    }
    else if (chat_type == mtpc_inputPeerSelf) {
        start_block_ind = SELF_MESSAGE_POSITION + payload_bias;
    }
    
    uint32_t start_message_byte = start_block_ind * buffer_element_size; // индекс первого байта в сообщении
    uint32_t first_block = static_cast<uint32_t>(buffer[start_block_ind]); // значение первого блока

    // Извлекаем длинну сообщения (которая находится в начале)
    uint32_t message_len;
    if ((first_block & LEAST_BYTE_MASK) == 0xFE) { // 0xFE - флаг, что длинна сообщения превышает (или равна) 0xFE
        // Длинное сообщение (под длинну выделен отдельный элемент буфера)
        message_len = (first_block & (~LEAST_BYTE_MASK)) >> 8; // берём всё кроме флага 
        start_message_byte += buffer_element_size;
    }
    else {
        // Короткое сообщение (длинна находится в первом байте сообщения)
        message_len = first_block & LEAST_BYTE_MASK;
        start_message_byte += 1;
    }

    uint32_t end_message_byte = start_message_byte + message_len - 1; // индекс последнего байта в сообщении

    // Извлекаем сообщение
    std::string message;
    uint32_t cur_block_ind = start_block_ind;

    for (int i = start_message_byte; i < end_message_byte + 1; ++i) {
        if ((i % buffer_element_size == 0) && (i != start_message_byte)) {
            ++cur_block_ind;
        }

        uint32_t cur_block = static_cast<uint32_t>(buffer[cur_block_ind]);
        uint32_t mask_move = (i % buffer_element_size) * 8;
        char cur_byte = static_cast<char>((cur_block & (LEAST_BYTE_MASK << mask_move)) >> mask_move);

        message.push_back(cur_byte);
    }

    // Шифруем сообщение
    AesKeyManager aes_manager;
    std::string key_test = "HYyt4p88dZFNhQ4Z+9LOUZqI/m17Arp/MZh76yMj3E4=";
    std::string encrypted_message = aes_manager.encrypt_message(message, key_test);
    uint32_t encrypted_message_len = encrypted_message.size();
    
    // Копируем информацию после сообщения
    uint32_t copy_from = end_message_byte / buffer_element_size + 1;
    mtpBuffer saved_postfix(buffer.begin() + copy_from, buffer.end());

    // Удаляем сообщение и всю информацию после него
    buffer.erase(buffer.begin() + start_block_ind, buffer.end());
    
    uint32_t start_encrypted_message_ind = 0; // индекс с которого начнётся считывание зашифрованной строки

    // Записываем длинну
    if (encrypted_message_len < 0xFE) {
        // Короткое сообщение (длинна находится в первом байте сообщения)
        mtpPrime value = encrypted_message_len |    // записываем длинну и дополняем началом сообщения
           (static_cast<mtpPrime>(static_cast<uint8_t>(encrypted_message[2])) << 24) |
           (static_cast<mtpPrime>(static_cast<uint8_t>(encrypted_message[1])) << 16) |
           (static_cast<mtpPrime>(static_cast<uint8_t>(encrypted_message[0])) << 8);

        buffer.push_back(value);
        start_encrypted_message_ind = 3;
    }
    else {
        // Длинное сообщение (под длинну выделен отдельный элемент буфера)
        mtpPrime value = (0xFE | (encrypted_message_len << 8));
        buffer.push_back(value);
    }

    // Записываем оставшуюся часть сообщения
    for (int i = start_encrypted_message_ind; i < encrypted_message_len; i += 4) {
        mtpPrime value = 0;

        for (int j = 0; (j < 4) && (i + j < encrypted_message_len); ++j) {
            value = value | (static_cast<mtpPrime>(static_cast<uint8_t>(encrypted_message[i + j])) << (j * 8));
        }
        buffer.push_back(value);
    }

    // Возращаем ранее удалённые конечные значения
    buffer.append(saved_postfix);

    // Изменим длинну Payload
    uint32_t not_payload_elements = PAYLOAD_LEN_POSITION + 1;
    buffer[PAYLOAD_LEN_POSITION] = (buffer.size() - not_payload_elements) * buffer_element_size;

    // Изменим длинну контейнера (если сообщение не обёрнуто в контейнер, то просто перезапишем Payload на то же значение)
    uint32_t not_container_elements = PAYLOAD_LEN_POSITION + payload_bias + 1;
    buffer[PAYLOAD_LEN_POSITION + payload_bias] = (buffer.size() - not_container_elements) * buffer_element_size;

    std::cout << "start_message_byte: " << start_message_byte << '\n';
    std::cout << "end_message_byte: " << end_message_byte << '\n';
    std::cout << "message_len: " << message_len << '\n';
    std::cout << "message: " << message << '\n';
    std::cout << "encrypted_message: " << encrypted_message << '\n';
    std::cout << "chat_id_str: " << chat_id_str << '\n'; 
}