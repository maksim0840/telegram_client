#include "message_text_decryption.h"

void MTPDmessage_private_fields_access(const MTPDmessage &msg, const std::string& text) {
	// Снимаем const
	auto &mutable_message = const_cast<MTPDmessage&>(msg);
	// Подменяем
	mutable_message._message.v = QByteArray(text);
}

std::vector<QString> decrypt_the_message(const MTPDmessage &msg) {
	std::vector<QString> res;
	std::string text = msg.vmessage().v.toStdString();

	if (text == "marko") {
		text = "polo";
	}
	else if (text == "Marko") {
		res.push_back(QString::fromStdString("Polo1"));
		res.push_back(QString::fromStdString("Polo2"));
		res.push_back(QString::fromStdString("Polo3"));
	}
	MTPDmessage_private_fields_access(msg, text);

	return res;
}