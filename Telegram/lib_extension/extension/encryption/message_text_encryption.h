#pragma once
#include <iostream>
#include <QtCore/QString>
#include <QtGlobal>
#include "keys_creator.h"
#include "../local_storage/local_storage.h"

#pragma once

QString encrypt_the_message(const QString& text, const quint64 peer_id);
