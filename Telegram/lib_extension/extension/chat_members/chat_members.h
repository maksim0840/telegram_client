#include<vector>
#include <iostream>
#include <QtGlobal>
#include <cctype>
#include <algorithm>
#include "../local_storage/keys_db.h"

#pragma once

void update_chat_members(const quint64 chat_id, const quint64 my_id, const std::vector<quint64>& chat_members);