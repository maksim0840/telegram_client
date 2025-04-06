#include<vector>
#include <iostream>
#include <QtGlobal>
#include <cctype>
#include <algorithm>
#include <tuple>
#include "../local_storage/keys_db.h"

#pragma once

class ChatMembers {
public:
    // Обновить пользователей в базе данных и вернуть (my_id_str, chat_id_str, chat_members_str, my_id_pos)
    static std::tuple<std::string, std::string, std::vector<std::string>, int> update_chat_members(const quint64 my_id, const quint64 chat_id, const std::vector<quint64>& chat_members);
};