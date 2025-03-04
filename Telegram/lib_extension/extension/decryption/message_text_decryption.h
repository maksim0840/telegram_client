#include <iostream>
#include <QtCore/QString>
#include <QtGlobal>
#include <cctype>
#include "scheme.h"

#pragma once

void MTPDmessage_private_fields_access(const MTPDmessage &msg);

QString decrypt_the_message(const QString& text);