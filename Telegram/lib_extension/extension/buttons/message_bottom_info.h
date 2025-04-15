#include <utility>
#include <rpl/event_stream.h>
#include "base/basic_types.h"
#include "ui/style/style_core_icon.h"
#include "styles/style_info.h"
#include "styles/style_dialogs.h"
#include "styles/style_chat.h"
#include "ui/chat/chat_style.h"
#include "icons.h"

class MessageBottomInfo {
public:
    // Получить иконку для обозначения зашифрованных сообщений
    static const style::icon& getEncryptedMessageReadIcon();

    // Получить инвертированную иконку для обозначения зашифрованных сообщений
    static const style::icon& getEncryptedMessageNotReadIcon();
};
