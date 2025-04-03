#include <iostream>
#include <QtCore/QString>
#include <QtGlobal>
#include <cctype>
#include "scheme.h"
#include "../keys_manager/keys_manager.h"

#pragma once

// Функция, которая имеет доступ к приватным полям класса MTPDmessage (friend-функция)
void MTPDmessage_private_fields_access(const MTPDmessage &msg, const std::string& text);

std::vector<QString> decrypt_the_message(const MTPDmessage &msg, const quint64 chat_id, const quint64 my_id);