#include "chat_members.h"

void update_chat_members(const quint64 chat_id, const quint64 my_id, const std::vector<quint64>& chat_members) {
    std::cout << "chat_id: " << chat_id << '\n';
    std::cout << "my_id: " << my_id << '\n';
    
    for (const auto id : chat_members) {
        std::cout << "chat_member: " << id << '\n';
    }

    // Преобразуем в строковый формат (т.к. в базе параметры хранится в строковом формате)
    std::string chat_id_str = std::to_string(chat_id);
    std::string my_id_str = std::to_string(my_id);
    std::vector<std::string> chat_members_str;
    for (const auto num : chat_members) {
        chat_members_str.push_back(std::to_string(num));
    }

    // Сортируем (id обязательно должны быть в отсортирвоанном порядке)
    std::sort(chat_members_str.begin(), chat_members_str.end());

    // Получаем, имеющиеся параметры в базе
    KeysDataBase db;
    std::optional<std::string> ids_str = db.get_active_param_text(chat_id_str, KeysTablesDefs::CHATS, ChatsColumnsDefs::MEMBERS_IDS);
    
    if (ids_str) {
        std::vector<std::string> ids = KeysDataBaseHelper::ids_string_to_vector(*ids_str);
        // Сравниваем значения
        if (chat_members_str == ids) {
            return; // дополнительных изменений вносить не трубется
        }
    }
    
    // Значения не совпадают => устанавливаем новые
    ChatsParamsFiller chat_params;
    chat_params.chat_id = chat_id_str;
    chat_params.my_id = my_id_str;
    chat_params.members_ids = chat_members_str;
    db.add_chat_params(chat_params);
}