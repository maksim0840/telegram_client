#include "message_text_decryption.h"

void MTPDmessage_private_fields_access(const MTPDmessage &msg, const std::string& text) {
	// Снимаем const
	auto &mutable_message = const_cast<MTPDmessage&>(msg);
	// Подменяем
	mutable_message._message.v = QByteArray(text);
}

std::vector<QString> decrypt_the_message(const MTPDmessage &msg, const quint64 chat_id, const quint64 my_id) {
	std::vector<std::string> res;
	std::vector<QString> res_qstring;

	std::string text = msg.vmessage().v.toStdString();
	std::string chat_id_str = std::to_string(chat_id);
	std::string my_id_str = std::to_string(my_id);

	std::cout << "decrypt chat by id: " << chat_id_str << '\n';
	
	// MTPDmessage_private_fields_access(msg, text);
	
	Message input_message;
	if (!input_message.fill_options(text)) {
		return res_qstring;
	}
	ChatCommandsManager commands;

	if (input_message.rsa_init && input_message.rsa_form && input_message.rsa_use) { // признак окончания ввода rsa ключа
		commands.continue_rsa(chat_id_str, my_id_str, input_message);
		res = commands.end_rsa(chat_id_str, my_id_str, input_message.rsa_key_n, input_message.dh_fastmode);
	}
	else if (input_message.rsa_init || input_message.rsa_form) { // обрабатываем сообщение о создании / продолжении создания rsa ключа
		//try {
			res = commands.continue_rsa(chat_id_str, my_id_str, input_message);
		//} catch (const std::exception& e) {}
	}
	else if (input_message.aes_init && input_message.aes_form && input_message.aes_use) {
		commands.end_aes(chat_id_str, my_id_str, input_message.aes_key_n, input_message.dh_fastmode);
	}
	else if (input_message.aes_init || input_message.aes_form) {
		res = commands.continue_aes(chat_id_str, my_id_str, input_message);
	}
	else {
		KeysDataBase db;
		std::optional<std::string> session_key = db.get_active_param_text(chat_id_str, KeysTablesDefs::AES, AesColumnsDefs::SESSION_KEY);
		if (!session_key) { return res_qstring; }

		AesKeyManager aes_manager;
		std::cout << *session_key << '\n';
		std::string decrypted_text = aes_manager.decrypt_message(input_message.text, *session_key);
		MTPDmessage_private_fields_access(msg, decrypted_text);
	}

	
	for (const auto& r : res) {
		std::cout << r << '\n';
		res_qstring.push_back(QString::fromStdString(r));
	}
	std::cout << 0 << '\n';
	return res_qstring;
}