#include "chat_members.h"

std::tuple<std::string, std::string, std::vector<std::string>, int> update_chat_members(const quint64 my_id, const quint64 chat_id, const std::vector<quint64>& chat_members) {

    // Преобразуем в строковый формат (т.к. в базе параметры хранится в строковом формате)
    std::string chat_id_str = std::to_string(chat_id);
    std::string my_id_str = std::to_string(my_id);
    std::vector<std::string> chat_members_str;
    for (const auto num : chat_members) {
        chat_members_str.push_back(std::to_string(num));
    }

    // Сортируем (id обязательно должны быть в отсортирвоанном порядке)
    std::sort(chat_members_str.begin(), chat_members_str.end());

    // Получаем позицию нашего id в векторе
    auto it = std::find(chat_members_str.begin(), chat_members_str.end(), my_id_str);
    int my_id_pos = (it != chat_members_str.end()) ? std::distance(chat_members_str.begin(), it) : -1;

    // Формируем возвращаемый результат
    std::tuple<std::string, std::string, std::vector<std::string>, int> res = std::make_tuple(my_id_str, chat_id_str, chat_members_str, my_id_pos);

    // Получаем, имеющиеся параметры в базе
    KeysDataBase db;
    std::optional<std::string> ids_str = db.get_active_param_text(chat_id_str, KeysTablesDefs::CHATS, ChatsColumnsDefs::MEMBERS_IDS);
    
    if (ids_str) {
        std::vector<std::string> ids = KeysDataBaseHelper::string_to_vector(*ids_str);
        // Сравниваем значения
        if (chat_members_str == ids) {
            return res; // дополнительных изменений вносить не трубется
        }
    }

    // Значения не совпадают => устанавливаем новые
    ChatsParamsFiller chat_params;
    chat_params.chat_id = chat_id_str;
    chat_params.my_id_pos = my_id_pos;
    chat_params.members_ids = chat_members_str;
    db.add_chat_params(chat_params);
    return res;
}