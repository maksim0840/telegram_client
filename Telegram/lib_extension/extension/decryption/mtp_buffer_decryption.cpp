#include "mtp_buffer_decryption.h"

void Receive::decrypt_the_buffer(mtpBuffer& buffer, std::function<mtpBuffer(const mtpPrime*, const mtpPrime*)> ungzip_lambda) {
    
    const int buffer_len = buffer.size();
    const int buffer_element_size = sizeof(mtpPrime); // размер блока/элемента внутри mtpBuffer

    Positions positions;
    mtpBuffer buf = buffer;
    uint32_t wrap_type = 0;
    uint32_t wrap_begin_ind = 0;
    uint32_t wrap_end_ind = 0;
    uint32_t container_type = 0;

    if (buffer_len < positions.REQUEST_TYPE + 1) {
        return;
    }

    // Проверяем упаковано сообщение или нет
    if (buf[positions.REQUEST_TYPE] == mtpc_gzip_packed || buf[positions.REQUEST_TYPE] == mtpc_rpc_result) {

        wrap_type = buf[positions.REQUEST_TYPE];

        mtpBuffer ungzip_data = mtpBuffer();
		const mtpPrime* ungzip_from = buf.data() + positions.REQUEST_TYPE;
		const mtpPrime* ungzip_end = ungzip_from + (static_cast<uint32_t>(buf[PAYLOAD_LEN_POSITION]) / buffer_element_size);

		MTPlong reqMsgId;
		if (wrap_type == mtpc_gzip_packed) {
            wrap_begin_ind = ungzip_from - buf.data() + 1;
            wrap_end_ind = ungzip_end - buf.data();
			buf = ungzip_lambda(++ungzip_from, ungzip_end);
            positions.fill_by_gzip_packed();
		}
		else if (wrap_type == mtpc_rpc_result && reqMsgId.read(++ungzip_from, ungzip_end) && ungzip_from[0] == mtpc_gzip_packed) {
            wrap_begin_ind = ungzip_from - buf.data() + 1;
            wrap_end_ind = ungzip_end - buf.data();
			buf = ungzip_lambda(++ungzip_from, ungzip_end);
            positions.fill_by_rpc_result();
		}

        std::cout << "ungzip_data "<< ungzip_data.size() << "\n";
		for (size_t i = 0; i < buf.size(); ++i) {
			std::cout << "  [" << i << "] = 0x" << std::hex << static_cast<uint32_t>(buf[i]) << std::dec << '\n';
		}
    }
    
    if (buf.size() < positions.REQUEST_TYPE + 1) {
        return;
    }

    // Проверяем лежит ли сообщение в контейнере из нескольких или нет
    if (buf[positions.REQUEST_TYPE] == mtpc_msg_container || buf[positions.REQUEST_TYPE] == mtpc_messages_messagesSlice) {
        container_type = buf[positions.REQUEST_TYPE];
        if (container_type == mtpc_msg_container) { positions.fill_by_msg_container(); }
        else if (container_type == mtpc_messages_messagesSlice) { positions.fill_by_messages_messagesSlice(); }
    }

    int total_delta = 0; // итоговая разница между длиннами зашифрованных и расшифрованнхы сообщений
    int bias = 0; // если сообщений несколько, то увеличвает bias на размер предыдущего
    while (true) {

        // Определяем тип сообщения
        uint32_t message_type = buf[positions.REQUEST_TYPE + bias];
        std::cout << "message_type: 0x" << std::hex << static_cast<uint32_t>(message_type) << std::dec << '\n';
        if (message_type != mtpc_message && message_type != mtpc_updateShortChatMessage && message_type != mtpc_updateShortMessage) {
            if (bias == 0) { return; }
            break;
        }

        // Определяем тип чата
        uint32_t chat_type = buf[positions.CHAT_TYPE + bias];
        
        // Определяем id отправителя
        uint64_t user_id = static_cast<uint32_t>(buf[positions.USER_ID_FIRST + bias]) |
                            (static_cast<uint64_t>(static_cast<uint32_t>(buf[positions.USER_ID_SECOND + bias])) << 32); 
        std::string user_id_str = std::to_string(user_id);

        // Определяем id чата
        uint64_t chat_id;
        if ((message_type == mtpc_message && chat_type != mtpc_peerUser && chat_type != mtpc_peerChat) ||
            message_type == mtpc_updateShortMessage) {

            chat_id = user_id; // диалоговые сообщения, отправленные собеседником (id чата = id отправителя)
        }
        else {
            chat_id = CHAT_TYPE_VALUE |
                static_cast<uint32_t>(buf[positions.CHAT_ID_FIRST + bias]) |
                (static_cast<uint64_t>(static_cast<uint32_t>(buf[positions.CHAT_ID_SECOND + bias])) << 32);
        }
        std::string chat_id_str = std::to_string(chat_id);

        // Находим начало сообщения
        uint32_t start_block_ind; // индекс блока, где начинается сообщение
        if ((message_type == mtpc_message && chat_type != mtpc_peerUser && chat_type != mtpc_peerChat) ||
            message_type == mtpc_updateShortMessage) {

            start_block_ind = positions.USER_MESSAGE + bias; // укороченный запрос (без id чата)
        }
        else {
            start_block_ind = positions.CHAT_MESSAGE + bias;
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
        std::string decrypted_message = decrypt_the_message(message, chat_id_str, user_id_str);
        uint32_t decrypted_message_len = decrypted_message.size();
 
        // Дополняем длинну строки минимум до трёх пустыми байтами
        if (decrypted_message_len < 3) {
            decrypted_message += std::string(3 - decrypted_message_len, '\0');
            decrypted_message_len = 3;
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
        int pushes_cout = 0; // сколько раз мы произвели операций buf.push_back

        // Записываем длинну
        if (decrypted_message_len < 0xFE) {
            // Короткое сообщение (длинна находится в первом байте сообщения)
            mtpPrime value = decrypted_message_len |    // записываем длинну и дополняем началом сообщения
            (static_cast<mtpPrime>(static_cast<uint8_t>(decrypted_message[2])) << 24) |
            (static_cast<mtpPrime>(static_cast<uint8_t>(decrypted_message[1])) << 16) |
            (static_cast<mtpPrime>(static_cast<uint8_t>(decrypted_message[0])) << 8);

            buf.push_back(value);
            start_decrypted_message_ind = 3;
            ++pushes_cout;
        }
        else {
            // Длинное сообщение (под длинну выделен отдельный элемент буфера)
            mtpPrime value = (0xFE | (decrypted_message_len << 8));
            buf.push_back(value);
            ++pushes_cout;
        }

        // Записываем оставшуюся часть сообщения
        for (int i = start_decrypted_message_ind; i < decrypted_message_len; i += 4) {
            mtpPrime value = 0;

            for (int j = 0; (j < 4) && (i + j < decrypted_message_len); ++j) {
                value = value | (static_cast<mtpPrime>(static_cast<uint8_t>(decrypted_message[i + j])) << (j * 8));
            }
            buf.push_back(value);
            inserted_bytes_count += buffer_element_size;
            ++pushes_cout;
        }
  
        // Возращаем ранее удалённые конечные значения
        buf.append(saved_postfix);

        total_delta += (inserted_bytes_count - erased_bytes_count);
        bias = start_block_ind + pushes_cout - positions.REQUEST_TYPE;

        std::cout <<"bias: " << bias << '\n';
        // Проверяем условия для выходы из цикла (сообщение должно быть одно или оно последнее)
        if (wrap_type == 0 || ((positions.REQUEST_TYPE + bias) >= buf.size())) {
            break;
        }
    }

    // Подменяем данные
    if (wrap_type == mtpc_gzip_packed || wrap_type == mtpc_rpc_result) {
        // Определяем длинну оригинальной запакованной секции и нашей заменённой
        uint32_t buffer_zip_len = (wrap_end_ind - wrap_begin_ind) * buffer_element_size;
        uint32_t buf_zip_len = buf.size() * buffer_element_size;

        // Обратно запаковываем то, что распаковали и подменили
        mtpBuffer gzip = Gzip::gzip(buf.data(), buf.data() + buf.size());

        // Удаляем старый gzip из итогового буфера
        buffer.erase(buffer.begin() + wrap_begin_ind, buffer.begin() + wrap_end_ind);

        // Вставляем новый gzip 
        auto insert_pos = buffer.begin() + wrap_begin_ind;
        for (auto it = gzip.constBegin(); it != gzip.constEnd(); ++it) {
            insert_pos = buffer.insert(insert_pos, *it);
            ++insert_pos;
        }

        // Изменим длинну Payload на текущую
        buffer[PAYLOAD_LEN_POSITION] += (buf_zip_len - buffer_zip_len); // payload = содержимое gzip и тип запроса
    }
    else {
        buffer = buf;
        // Изменим длинну Payload на текущую
        buffer[PAYLOAD_LEN_POSITION] += (total_delta);

        // Изменим Payload контейнера
        if (container_type == mtpc_msg_container) {
            buffer[PAYLOAD_LEN_POSITION + MSG_CONTAINER_BIAS] += (total_delta);
        }
    } 
}


std::string Receive::decrypt_the_message(const std::string& msg, std::string chat_id_str, std::string sender_id_str) {
	KeysDataBase db;
	AesKeyManager aes_manager;

    // Определяем свой id и заменяем id-шники собседников (если они не определены)
    std::optional<std::string> my_id_str = db.get_my_id();
    if (chat_id_str == "" || chat_id_str == "0") {
        if (!my_id_str) { throw std::runtime_error("Ошибка получения параметра (собственного id )"); }
        else { chat_id_str = *my_id_str; }
    } 
    if (sender_id_str == "" || sender_id_str == "0") {
        if (!my_id_str) { throw std::runtime_error("Ошибка получения параметра (собственного id )"); }
        else { sender_id_str = *my_id_str; }
    }
    std::optional<int> aes_key_n = db.get_last_key_n(chat_id_str, *my_id_str, KeysTablesDefs::AES);
    aes_key_n = (aes_key_n) ? (*aes_key_n) : 0;

    // Проверяем является ли сообщение частью алгоритма передачи ключей
    Message m;
    bool is_Message_type = m.fill_options(msg);
    std::cout << "msg: " << msg << '\n';
    std::cout << "is_Message_type: " << is_Message_type << '\n';
    if (!is_Message_type) {
        return msg;
    }

    if ((m.aes_form || m.aes_init || m.rsa_form || m.rsa_init) && (m.aes_key_n > *aes_key_n) && (sender_id_str != *my_id_str)) {
        std::cout << "ChatKeyCreation::start from decryption\n";
        if (!ChatKeyCreation::is_started()) {
            ChatKeyCreation::start(KeyCreationStages::RSA_SEND_PUBLIC_KEY);
        }
        ChatKeyCreation::add_info(m, sender_id_str);
    }
    else if (m.end_key_forming && (m.aes_key_n > *aes_key_n) && (sender_id_str != *my_id_str)) {
        ChatKeyCreation::stop();
    }
    else if (m.end_encryption && (m.aes_key_n > *aes_key_n) && (sender_id_str != *my_id_str)) {
        ChatKeyCreation::stop();
        ChatKeyCreation::end_encryption();
    }
	else if (m.aes_use) {

		// Находим ключ шифрования и дешифруем (если он есть)
		std::optional<std::string> aes_key;
		std::optional<std::string> aes_key_active = db.get_key_n_active_param_text(chat_id_str, m.aes_key_n, KeysTablesDefs::AES, AesColumnsDefs::SESSION_KEY, 1);
		aes_key = (aes_key_active) ? aes_key_active : aes_key;
		std::optional<std::string> aes_key_not_active = db.get_key_n_active_param_text(chat_id_str, m.aes_key_n, KeysTablesDefs::AES, AesColumnsDefs::SESSION_KEY, -1);
		aes_key = (aes_key_not_active) ? aes_key_not_active : aes_key;

		if (aes_key) {
            std::cout << "!!!! decrypt_message: " << m.text << "; by: " << *aes_key << '\n';
			return aes_manager.decrypt_message(m.text, *aes_key);
		}
	}

	return msg;
}