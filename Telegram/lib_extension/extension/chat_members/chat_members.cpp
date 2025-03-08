#include "chat_members.h"

void update_chat_members(const quint64 chat_id, const quint64 my_id, const std::vector<quint64>& chat_members) {
    std::cout << "chat_id: " << chat_id << '\n';
    std::cout << "my_id: " << my_id << '\n';
    
    for (const auto id : chat_members) {
        std::cout << "chat_member: " << id << '\n';
    }
}