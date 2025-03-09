#include <iostream>
#include <QtCore/QString>
#include <QtGlobal>
#include <cctype>

#include "../commands_manager/chat_commands_manager.h"

#pragma once

QString encrypt_the_message(const QString& text, const quint64 peer_id);