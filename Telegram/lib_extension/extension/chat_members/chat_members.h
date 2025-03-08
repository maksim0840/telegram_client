#include<vector>
#include <iostream>
#include <QtGlobal>
#include <cctype>

#pragma once

void update_chat_members(const quint64 chat_id, const quint64 my_id, const std::vector<quint64>& chat_members);