#include "mtp_buffer_decryption.h"

void Recieve::decrypt_the_buffer(mtpBuffer& buffer, mtpBuffer& ungzip_data) {
    
    const int buffer_len = buffer.size();
    const int buffer_element_size = sizeof(mtpPrime); // размер блока/элемента внутри mtpBuffer

    if (buffer_len < REQUEST_TYPE_POSITION + 1) {
        return;
    }

    // Определяем запакована ли информация в какую-либо доп. обёртку
    uint32_t wrap_type = 0;
    mtpBuffer buf = buffer;
    int bias = 0;
    
    if (buffer[REQUEST_TYPE_POSITION] == mtpc_gzip_packed) { // сообщение дополнительно запаковано
        wrap_type = mtpc_gzip_packed;
        buf = ungzip_data;
        bias = GZIP_BIAS;
    }
    else if (buffer[REQUEST_TYPE_POSITION] == mtpc_msg_container) {
        wrap_type = mtpc_msg_container;
        bias = CONTAINER_BIAS;
    }

    // Определяем тип запроса
    const uint32_t request_type = static_cast<uint32_t>(buf[REQUEST_TYPE_POSITION + bias]);
    if (!(request_type == mtpc_updateShortMessage || request_type == mtpc_updateShortChatMessage)) {
        return;
    }

    // Определяем id отправителя
    uint64_t user_id = static_cast<uint32_t>(buf[USER_ID_FIRST_POSITION + bias]) |
        (static_cast<uint64_t>(static_cast<uint32_t>(buf[USER_ID_SECOND_POSITION + bias])) << 32); 
    std::string user_id_str = std::to_string(user_id);

    // Определяем id чата
    uint64_t chat_id;
    if (request_type == mtpc_updateShortMessage) {
        chat_id = user_id;
    }
    else if (request_type == mtpc_updateShortChatMessage) {
        chat_id = CHAT_TYPE_VALUE |
            static_cast<uint32_t>(buf[CHAT_ID_FIRST_POSITION + bias]) |
            (static_cast<uint64_t>(static_cast<uint32_t>(buf[CHAT_ID_SECOND_POSITION + bias])) << 32); 
    }
    std::string chat_id_str = std::to_string(chat_id);


    // Находим начало сообщения
    uint32_t start_block_ind; // индекс блока, где начинается сообщение
    if (request_type == mtpc_updateShortChatMessage) {
        start_block_ind = CHAT_MESSAGE_POSITION + bias;
    }
    else if (request_type == mtpc_updateShortMessage) {
        start_block_ind = USER_MESSAGE_POSITION + bias;
    }
    
    uint32_t start_message_byte = start_block_ind * buffer_element_size; // индекс первого байта в сообщении
    uint32_t first_block = static_cast<uint32_t>(buf[start_block_ind]); // значение первого блока
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
    uint32_t cur_block_ind = start_message_byte / buffer_element_size;

    for (int i = start_message_byte; i < end_message_byte + 1; ++i) {
        if ((i % buffer_element_size == 0) && (i != start_message_byte)) {
            ++cur_block_ind;
        }

        uint32_t cur_block = static_cast<uint32_t>(buf[cur_block_ind]);
        uint32_t mask_move = (i % buffer_element_size) * 8;
        char cur_byte = static_cast<char>((cur_block & (LEAST_BYTE_MASK << mask_move)) >> mask_move);
        message.push_back(cur_byte);
    }

    // Расшифруем сообщение
    AesKeyManager aes_manager;
    std::string key_test = "HYyt4p88dZFNhQ4Z+9LOUZqI/m17Arp/MZh76yMj3E4=";
    std::string decrypted_message = message;

    // Проверяем является ли сообщение частью алгоритма передачи ключей
    std::cout << "check for Message type\n";
    Message m;
    bool is_Message_type = m.fill_options(message);
    std::cout << "end checking for Message type\n";
    if (is_Message_type && (m.aes_form || m.aes_init || m.rsa_form || m.rsa_init)) {
        std::cout << "ChatKeyCreation::start from decryption\n";
        if (!ChatKeyCreation::is_started()) {
            ChatKeyCreation::start(KeyCreationStages::RSA_SEND_PUBLIC_KEY);
        }
        ChatKeyCreation::add_info(m, user_id_str);
    }
    // std::string decrypted_message = aes_manager.decrypt_message(message, key_test);
    // decrypted_message = "test test test!!??";
    uint32_t decrypted_message_len = decrypted_message.size();
    
    if (decrypted_message == message) {
        return;
    }

    // Копируем информацию после сообщения
    uint32_t copy_from = end_message_byte / buffer_element_size + 1;
    mtpBuffer saved_postfix(buf.begin() + copy_from, buf.end());

    // Счётчики имзенённых байтов для подсчёта длинны сообщения
    int inserted_bytes_count = (saved_postfix.size() + 1) * buffer_element_size; // информация которую мы вернём + информация о длинне сообщения + ...
    const int erased_bytes_count = (buf.size() - start_block_ind) * buffer_element_size;

    // Удаляем сообщение и всю информацию после него
    buf.erase(buf.begin() + start_block_ind, buf.end());
    
    uint32_t start_decrypted_message_ind = 0; // индекс с которого начнётся считывание расифрованной строки

    // Записываем длинну
    if (decrypted_message_len < 0xFE) {
        // Короткое сообщение (длинна находится в первом байте сообщения)
        mtpPrime value = decrypted_message_len |    // записываем длинну и дополняем началом сообщения
           (static_cast<mtpPrime>(static_cast<uint8_t>(decrypted_message[2])) << 24) |
           (static_cast<mtpPrime>(static_cast<uint8_t>(decrypted_message[1])) << 16) |
           (static_cast<mtpPrime>(static_cast<uint8_t>(decrypted_message[0])) << 8);

        buf.push_back(value);
        start_decrypted_message_ind = 3;
    }
    else {
        // Длинное сообщение (под длинну выделен отдельный элемент буфера)
        mtpPrime value = (0xFE | (decrypted_message_len << 8));
        buf.push_back(value);
    }

    // Записываем оставшуюся часть сообщения
    for (int i = start_decrypted_message_ind; i < decrypted_message_len; i += 4) {
        mtpPrime value = 0;

        for (int j = 0; (j < 4) && (i + j < decrypted_message_len); ++j) {
            value = value | (static_cast<mtpPrime>(static_cast<uint8_t>(decrypted_message[i + j])) << (j * 8));
        }
        buf.push_back(value);
        inserted_bytes_count += buffer_element_size;
    }

    // Возращаем ранее удалённые конечные значения
    buf.append(saved_postfix);

    // Подменяем данные
    if (wrap_type == mtpc_gzip_packed) {
        // Обратно запаковываем то, что распаковали и подменили
        mtpBuffer gzip = Gzip::gzip(buf.data(), buf.data() + buf.size());

        // Удаляем старый gzip из итогового буфера
        int del_from = REQUEST_TYPE_POSITION + 1;
        int del_end= REQUEST_TYPE_POSITION + (buffer[PAYLOAD_LEN_POSITION] / buffer_element_size);
        buffer.erase(buffer.begin() + del_from, buffer.begin() + del_end);

        // Вставляем новый gzip 
        auto insert_pos = buffer.begin() + del_from;
        for (auto it = gzip.constBegin(); it != gzip.constEnd(); ++it) {
            insert_pos = buffer.insert(insert_pos, *it);
            ++insert_pos;
        }

        // Изменим длинну Payload на текущую
        buffer[PAYLOAD_LEN_POSITION] = (gzip.size() + 1) * buffer_element_size; // payload = содержимое gzip и тип запроса
    }
    else {
        buffer = buf;
        // Изменим длинну Payload на текущую
        buffer[PAYLOAD_LEN_POSITION] += (inserted_bytes_count - erased_bytes_count);

        // Изменим Payload контейнера
        if (buffer[REQUEST_TYPE_POSITION] == mtpc_msg_container) {
            buffer[PAYLOAD_LEN_POSITION + bias] += (inserted_bytes_count - erased_bytes_count);
        }
    }

    mtpBuffer new_buffer(buffer.begin(), buffer.end());
    std::cout << "start_message_byte: " << start_message_byte << '\n';
    std::cout << "end_message_byte: " << end_message_byte << '\n';
    std::cout << "message_len: " << message_len << '\n';
    std::cout << "message: " << message << '\n';
    std::cout << "decrypted_message: " << decrypted_message << '\n';
    std::cout << "chat_id_str: " << chat_id_str << '\n';    
}