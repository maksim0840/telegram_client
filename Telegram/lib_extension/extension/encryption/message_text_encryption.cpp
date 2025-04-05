#include "message_text_encryption.h"

std::string encrypt_the_message(const std::string& msg, std::string chat_id_str) {
    KeysDataBase db;
	AesKeyManager aes_manager;
    
    // Определяем свой id и заменяем id-шники собседников (если они не определены)
    std::optional<std::string> my_id_str = db.get_my_id();
    if (chat_id_str == "" || chat_id_str == "0") {
        if (!my_id_str) { throw std::runtime_error("Ошибка получения параметра (собственного id )"); }
        else { chat_id_str = *my_id_str; }
    } 
    
    // Проверяем является ли сообщение частью алгоритма передачи ключей
    Message m;
    bool is_Message_type = m.fill_options(msg);
    if (!is_Message_type) {
        
        // Находим ключ шифрования и шифруем (если есть)
		std::optional<std::string> aes_key = db.get_active_param_text(chat_id_str, KeysTablesDefs::AES, AesColumnsDefs::SESSION_KEY);
        std::optional<int> aes_key_n = db.get_active_param_int(chat_id_str, KeysTablesDefs::AES, AesColumnsDefs::KEY_N);
        if (aes_key && aes_key_n) {
            m.aes_use = true;
            m.aes_key_n = *aes_key_n;
            m.text = aes_manager.encrypt_message(msg, *aes_key);
            std::cout << "!!!! to: " << chat_id_str << '\n';
            std::cout << "!!!! encrypt_message: " << msg << "; by: " << *aes_key << '\n';
			return m.get_text_with_options();
		}
    }

    return msg;
}