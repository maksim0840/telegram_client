#include "mtp_buffer_encryption.h"

void Send::encrypt_the_buffer(mtpBuffer& buffer) {

    const int buffer_len = buffer.size();
    const int buffer_element_size = sizeof(mtpPrime); // размер блока/элемента внутри mtpBuffer

    if (buffer_len < CHAT_TYPE_POSITION + 1) {
        return;
    }

    // Определяем дополнительное смещение внутри Payload (если сообщения обёрнуты контейнером)
    int container_bias = 0;
    if (buffer[REQUEST_TYPE_POSITION] == mtpc_msg_container) {
        container_bias = CONTAINER_BIAS;
    }

    // Счётчики имзенённых байтов для подсчёта длинны Payload
    int total_inserted_bytes_count = 0;
    int total_erased_bytes_count = 0;

    for (int i = REQUEST_TYPE_POSITION + container_bias; i < buffer.size() - 1; ++i) {
        if (!(buffer[i] == mtpc_messages_sendMessage && buffer[i + 1] == message_addition_flags)) {
            continue;
        }

        const int bias = i; // найденная позиция типа отправляемого сообщения
        
        // Определяем тип чата
        const int chat_type = static_cast<uint32_t>(buffer[CHAT_TYPE_POSITION + bias]);
        if (!(chat_type == mtpc_inputPeerChat || chat_type == mtpc_inputPeerUser || chat_type == mtpc_inputPeerSelf)) {
            return;
        }

        // Определяем id чата
        uint64_t chat_id = 0; // сообщение самому себе (mtpc_inputPeerSelf)
        if (chat_type == mtpc_inputPeerUser) {
            chat_id =   static_cast<uint32_t>(buffer[CHAT_ID_FIRST_POSITION + bias]) |
                        (static_cast<uint64_t>(static_cast<uint32_t>(buffer[CHAT_ID_SECOND_POSITION + bias])) << 32);
        }
        else if (chat_type == mtpc_inputPeerChat) {
            chat_id =   static_cast<uint32_t>(buffer[CHAT_ID_FIRST_POSITION + bias]) |
                        (static_cast<uint64_t>(static_cast<uint32_t>(buffer[CHAT_ID_SECOND_POSITION + bias])) << 32) |
                        CHAT_TYPE_VALUE;
        }
        std::string chat_id_str = std::to_string(chat_id);

        // Находим начало сообщения
        uint32_t start_block_ind; // индекс блока, где начинается сообщение
        if (chat_type == mtpc_inputPeerChat) {
            start_block_ind = CHAT_MESSAGE_POSITION + bias;
        }
        else if (chat_type == mtpc_inputPeerUser) {
            start_block_ind = USER_MESSAGE_POSITION + bias;
        }
        else if (chat_type == mtpc_inputPeerSelf) {
            start_block_ind = SELF_MESSAGE_POSITION + bias;
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
        uint32_t cur_block_ind = start_message_byte / buffer_element_size;

        for (int j = start_message_byte; j < end_message_byte + 1; ++j) {
            if ((j % buffer_element_size == 0) && (j != start_message_byte)) {
                ++cur_block_ind;
            }

            uint32_t cur_block = static_cast<uint32_t>(buffer[cur_block_ind]);
            uint32_t mask_move = (j % buffer_element_size) * 8;
            char cur_byte = static_cast<char>((cur_block & (LEAST_BYTE_MASK << mask_move)) >> mask_move);

            message.push_back(cur_byte);
        }

        // Шифруем сообщение
        std::string encrypted_message = encrypt_the_message(message, chat_id_str);
        uint32_t encrypted_message_len = encrypted_message.size();

        // Если ничего не поменялось, то выходим
        if (encrypted_message == message) {
            return;
        }

        // Дополняем длинну строки минимум до трёх пустыми байтами
        if (encrypted_message_len < 3) {
            encrypted_message += std::string(3 - encrypted_message_len, '\0');
            encrypted_message_len = 3;
        }

        /*
        
        //while (последние байты != 0x80mtpc_messages_sendMessage) { перешифровываем}
        
        */

        // Копируем информацию после сообщения
        uint32_t copy_from = end_message_byte / buffer_element_size + 1;
        mtpBuffer saved_postfix(buffer.begin() + copy_from, buffer.end());

        // Счётчики имзенённых байтов для подсчёта длинны сообщения
        int inserted_bytes_count = (saved_postfix.size() + 1) * buffer_element_size; // информация которую мы вернём + информация о длинне сообщения + ...
        const int erased_bytes_count = (buffer.size() - start_block_ind) * buffer_element_size;

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
        for (int j = start_encrypted_message_ind; j < encrypted_message_len; j += 4) {
            mtpPrime value = 0;

            for (int k = 0; (k < 4) && (j + k < encrypted_message_len); ++k) {
                value = value | (static_cast<mtpPrime>(static_cast<uint8_t>(encrypted_message[j + k])) << (k * 8));
            }
            buffer.push_back(value);
            inserted_bytes_count += buffer_element_size;
        }

        // Возращаем ранее удалённые конечные значения
        buffer.append(saved_postfix);

        i += (inserted_bytes_count - erased_bytes_count);
        total_inserted_bytes_count += inserted_bytes_count;
        total_erased_bytes_count += erased_bytes_count;

        std::cout << "start_message_byte: " << start_message_byte << '\n';
        std::cout << "end_message_byte: " << end_message_byte << '\n';
        std::cout << "message_len: " << message_len << '\n';
        std::cout << "message: " << message << '\n';
        std::cout << "encrypted_message: " << encrypted_message << '\n';
        std::cout << "chat_id_str: " << chat_id_str << '\n'; 
    }

    // Изменим длинну Payload
    buffer[PAYLOAD_LEN_POSITION] += (total_inserted_bytes_count - total_erased_bytes_count);

    // Изменим длинну контейнера сообщения
    if (container_bias != 0) {
        buffer[PAYLOAD_LEN_POSITION + container_bias] += (total_inserted_bytes_count - total_erased_bytes_count);
    }

}


std::string Send::encrypt_the_message(const std::string& msg, std::string chat_id_str) {
    KeysDataBase db;
	AesKeyManager aes_manager;
    
    // Определяем свой id и заменяем id-шники собседников (если они не определены)
    std::optional<std::string> my_id_str = db.get_my_id();
    if (chat_id_str == "" || chat_id_str == "0") {
        if (!my_id_str) { throw std::runtime_error("Ошибка получения параметра (собственного id )"); }
        else { chat_id_str = *my_id_str; }
    } 
    
    // Проверяем является ли сообщение частью алгоритма передачи ключей
    Message m;
    bool is_Message_type = m.fill_options(msg);
    if (!is_Message_type) {
        
        // Находим ключ шифрования и шифруем (если есть)
		std::optional<std::string> aes_key = db.get_active_param_text(chat_id_str, KeysTablesDefs::AES, AesColumnsDefs::SESSION_KEY);
        std::optional<int> aes_key_n = db.get_active_param_int(chat_id_str, KeysTablesDefs::AES, AesColumnsDefs::KEY_N);
        if (aes_key && aes_key_n) {
            m.aes_use = true;
            m.aes_key_n = *aes_key_n;
            m.text = aes_manager.encrypt_message(msg, *aes_key);
            std::cout << "!!!! to: " << chat_id_str << '\n';
            std::cout << "!!!! encrypt_message: " << msg << "; by: " << *aes_key << '\n';
			return m.get_text_with_options();
		}
    }

    return msg;
}