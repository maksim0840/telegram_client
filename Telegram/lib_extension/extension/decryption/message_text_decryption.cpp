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
	
	
	return res_qstring;
}