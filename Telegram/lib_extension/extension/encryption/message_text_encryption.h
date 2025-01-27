#pragma once
#include <iostream>
#include <QtCore/QString>
#include <QtGlobal> // для quint64
//#include <QtCore/QVector>
#include "keys_creator.h"

#pragma once

QString encrypt_the_message(const QString& text, const quint64 peer_id);
