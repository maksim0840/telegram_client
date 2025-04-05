#include "message_text_decryption.h"

std::string decrypt_the_message(const std::string& msg, std::string chat_id_str, std::string sender_id_str) {
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