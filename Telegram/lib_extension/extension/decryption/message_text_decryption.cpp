#include "message_text_decryption.h"

void MTPDmessage_private_fields_access(const MTPDmessage &msg) {
	// Снимаем const
	auto &mutable_message = const_cast<MTPDmessage&>(msg);
	// Подменяем
	if (msg.vmessage().v.toStdString() == "marko") {
		mutable_message._message.v = QByteArray("polo");
	}
}