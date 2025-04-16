#include "base/basic_types.h"
#include "ui/style/style_core_icon.h"
#include "styles/style_info.h"
#include "styles/style_dialogs.h"
#include "icons.h"

namespace ext {

class TopBarButtons {
public:
    // Получить кнопку для начала шифрования
    static const style::IconButton& getTopBarStartEncryption();

    // Получить кнопку для сброса шифрования
    static const style::IconButton& getTopBarStopEncryption();
};

} // namespace ext