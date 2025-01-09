#include "message_text_encryption.h"

// QString make_encrypted_message(const TextWithTags& text_with_tags, const PeerData* peer) {
//     const auto& peer_id = peer->id; // uint64
//     const auto& tags = text_with_tags.tags; // QVector<Tag>
//     QString text = text_with_tags.text;

//     /* шифровка если peer_id этого требует */
//     std::string str = "okog";
//     text = QString::fromStdString(str);

//     std::cout << '\n';
//     std::cout << "Изначальная строка: " << text_with_tags.text.toStdString() << '\n';
//     std::cout << "Id пользователя: " << peer_id << '\n';
//     std::cout << "Изменённая строка: " << text.toStdString() << '\n';
//     std::cout << "Количество тэгов: " << tags.size() << '\n';
//     for (const auto& t : tags) {
//         std::cout << "offset " <<  t.offset << ", length " << t.length << ", id " << t.id.text.toStdString() << '\n';
//     }
//     std::cout << '\n';


//     return res;
// }


// QString make_encrypted_message(const QString& text_with_tags, const quint64 peer) {
//     std::string str = "okog";
    
//     std::cout << '\n';
//     std::cout << "Начальная строка: " << text_with_tags.toStdString() << '\n';
//     std::cout << "Id пользователя: " << peer << '\n';
//     std::cout << "Изменённая строка: " << str << '\n';
//     std::cout << '\n';

//     return QString::fromStdString(str);

// }

QString encrypt_the_message(const QString& text, const quint64 peer_id) {
    std::string new_text = "okog";
    
    std::cout << "НовыйX4 Удалить эту строку и всё остальное лишнее\n";

    std::cout << '\n';
    std::cout << "Начальная строка: " << text.toStdString() << '\n';
    std::cout << "Id пользователя: " << peer_id << '\n';
    std::cout << "Изменённая строка: " << new_text << '\n';
    std::cout << '\n';

    return QString::fromStdString(new_text);
}