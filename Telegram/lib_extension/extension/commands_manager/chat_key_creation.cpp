#include "chat_key_creation.h"

void start_chat_key_creation(const std::function<void(const std::string&)> &send_func) {
    send_func("/[start_encryption]");
}