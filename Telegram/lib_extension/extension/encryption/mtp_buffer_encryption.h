#include "message_text_encryption.h"
#include "base/basic_types.h"
#include "styles/style_info.h"
#include "mtproto/core_types.h"
#include "scheme.h"
using flota64 = double;

// Позиции элементов из mtpBuffer
#define PAYLOAD_LEN_POSITION 7 // последний элемент вне Payload
// Секция Payload (позиции указаны для сообщений, которые отправляются напрямую)
#define REQUEST_TYPE_POSITION 8
#define CHAT_TYPE_POSITION 10
#define SELF_MESSAGE_POSITION 11
#define CHAT_MESSAGE_POSITION 13
#define USER_MESSAGE_POSITION 15

// Если сообщение отправлено не напрямую (а обёрнуто в контейнер), то все позиции из Payload дополнительно смещаются
#define CONTAINER_BIAS 6

// Маска младшего байта
#define LEAST_BYTE_MASK 0xFF

// using mtpBuffer = QVector<mtpPrime>;
void encrypt_the_buffer(mtpBuffer& buffer);