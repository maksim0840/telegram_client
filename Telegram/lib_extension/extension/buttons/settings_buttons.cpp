#include "settings_buttons.h"

namespace ext {

const style::icon& SettingsButtons::getExtensionSettingsButton() {

	static style::icon extensionButton = [] {
        // Создаём иконку (за основу взята иконка dialogsUnlockIconOver, переделанная под стиль historyOutIconFg)
        style::icon icon = { std::in_place, style::internal::MonoIcon{ &icon_mask_shield, st::menuIconColor, { 0, 0 } } };
        return icon;
    }();

	return extensionButton;
}

}