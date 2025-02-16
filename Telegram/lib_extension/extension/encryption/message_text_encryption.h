#pragma once
#include <iostream>
#include <QtCore/QString>
#include <QtGlobal>
#include <cctype>
#include "../commands_manager/chat_commands_manager.h"

#pragma once

int extract_num(const std::string& str, const int start, const int end);
std::string get_command_result(const std::string& text_str, const std::string& chat_id_str);
QString encrypt_the_message(const QString& text, const quint64 peer_id);