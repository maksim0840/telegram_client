#include <iostream>
#include <QtCore/QString>
#include <QtGlobal>
#include <cctype>
#include "scheme.h"
#include "../commands_manager/chat_key_creation.h"

#pragma once

std::string decrypt_the_message(const std::string& msg, std::string chat_id_str, std::string sender_id_str);