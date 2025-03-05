#include <iostream>
#include <QtCore/QString>
#include <QtGlobal>
#include <cctype>
#include "scheme.h"

#pragma once

// Функция, которая имеет доступ к приватным полям класса MTPDmessage (friend-функция)
void MTPDmessage_private_fields_access(const MTPDmessage &msg, const std::string& text);

std::vector<QString> decrypt_the_message(const MTPDmessage &msg);