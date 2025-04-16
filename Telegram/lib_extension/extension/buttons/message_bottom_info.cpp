#include "message_bottom_info.h"

namespace ext {

const style::icon& MessageBottomInfo::getEncryptedMessageReadIcon() {

	static style::icon encryptedMessageIcon = [] {
        // Создаём иконку (за основу взята иконка dialogsUnlockIconOver, переделанная под стиль historyOutIconFg)
        style::icon icon = { std::in_place, style::internal::MonoIcon{ &icon_mask_lock, st::historyOutIconFg, { 0, -2 } } };
        return icon;
    }();

	return encryptedMessageIcon;
}

// const style::color &historyIconFgInverted
const style::icon& MessageBottomInfo::getEncryptedMessageNotReadIcon() {

	static style::icon encryptedMessageInvertedIcon = [] {
        // Создаём иконку (за основу взята иконка dialogsUnlockIconOver, переделанная под стиль menuFgDisabled)
        style::icon icon = { std::in_place, style::internal::MonoIcon{ &icon_mask_lock, st::menuFgDisabled, { 0, -2 } } };
        return icon;
    }();

	return encryptedMessageInvertedIcon;
}

} // namespace ext