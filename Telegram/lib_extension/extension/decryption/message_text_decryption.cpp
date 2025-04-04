#include "message_text_decryption.h"

std::string decrypt_the_message(const std::string& msg, const std::string& chat_id_str, const std::string& sender_id_str) {
	KeysDataBase db;
	AesKeyManager aes_manager;

    // Проверяем является ли сообщение частью алгоритма передачи ключей
    Message m;
    bool is_Message_type = m.fill_options(msg);
    if (is_Message_type && (m.aes_form || m.aes_init || m.rsa_form || m.rsa_init)) {
        std::cout << "ChatKeyCreation::start from decryption\n";
        if (!ChatKeyCreation::is_started()) {
            ChatKeyCreation::start(KeyCreationStages::RSA_SEND_PUBLIC_KEY);
        }
        ChatKeyCreation::add_info(m, sender_id_str);
    }
    else if (is_Message_type && m.end_key_forming) {
        ChatKeyCreation::stop();
    }
    else if (is_Message_type && m.end_encryption) {
        ChatKeyCreation::end_encryption();
    }
	else if (m.aes_use) {

		// Находим ключ шифрования и дешифруем (если есть)
		std::optional<std::string> aes_key = db.get_active_param_text(chat_id_str, KeysTablesDefs::AES, AesColumnsDefs::SESSION_KEY);
		if (aes_key) {
			return aes_manager.decrypt_message(m.text, *aes_key);
		}
	}

	return msg;
}