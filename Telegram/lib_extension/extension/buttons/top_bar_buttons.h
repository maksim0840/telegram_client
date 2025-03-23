#include "ui/style/style_core_icon.h"
#include "styles/style_info.h"
#include "styles/style_dialogs.h"

// Получить кнопку для начала шифрования
const style::IconButton& getTopBarStartEncryption() {

	static style::IconButton topBarStartEncryption = [] {
        // Создаём кнопку (за основу взята кнопка topBarInfo из файла out/Telegram/gen/styles/style_info.cpp)
        style::IconButton button = st::topBarCall;
        
        // Изменяем картинку на кнопке (за основу взята иконка dialogs_lock_on из файла out/Telegram/gen/styles/style_dialogs.h)
        button.icon = st::dialogsUnlockIcon; // иконка в обычном состоянии
        button.iconOver = st::dialogsUnlockIconOver; // иконка при наведении
        button.iconPosition = {0, -2}; // позиция
        return button;
    }();

	return topBarStartEncryption;
}

// Получить кнопку для сброса шифрования
const style::IconButton& getTopBarStopEncryption() {

	static style::IconButton topBarStartEncryption = [] {
        // Создаём кнопку (за основу взята кнопка topBarInfo из файла out/Telegram/gen/styles/style_info.cpp)
        style::IconButton button = st::topBarCall;
        
        // Изменяем картинку на кнопке (за основу взята иконка dialogs_lock_off из файла out/Telegram/gen/styles/style_dialogs.h)
        button.icon = st::dialogsLock.icon; // иконка в обычном состоянии
        button.iconOver = st::dialogsLock.iconOver; // иконка при наведении
        button.iconPosition = {0, -2}; // позиция
        return button;
    }();

	return topBarStartEncryption;
}

// const style::IconButton& get_button123() {
//     return st::topBarInfo;
// }
// // Кнопка начала шифрования
// inline style::IconButton _topBarStartEncryption = [] {
//     // Создаём кнопку (за основу взята кнопка topBarInfo из файла /out/Telegram/gen/styles/style_info.cpp)
//     style::IconButton button = st::topBarInfo;

//     // // Изменяем картинку на кнопке (за основу взята иконка dialogs_lock_on)
//     // button.icon = st::dialogsUnlockIcon;
//     // button.iconOver = st::dialogsUnlockIconOver;

//     return button;
// }();

// const style::IconButton& topBarStartEncryption(_topBarStartEncryption);

// // Создаём кнопку (за основу взята кнопка topBarInfo из файла /out/Telegram/gen/styles/style_info.cpp)
// style::IconButton topBarStartEncryption = st::topBarInfo;
// // Изменяем картинку на кнопке (за основу взята иконка )
// topBarStartEncryption.icon = st::dialogsUnlockIcon;
// topBarStartEncryption.iconOver = st::dialogsUnlockIconOver;

// Изменяем картинку на кнопке (за основу взята иконка )
// style::MonoIcon myMonoIcon("~/Documents/tdesktop/Telegram/lib_extension/icons/dialogs_lock_off.png", QSize(32, 32));
// style::MonoIcon myMonoIcon("~/Documents/tdesktop/Telegram/lib_extension/icons/dialogs_lock_off.png", QSize(32, 32));
// lock_icon_mask =     &st::dialogsUnlockIcon,
// &st::dialogsUnlockIconOver
// style::internal::MonoIcon lock_icon(style::internal::IconMask(), st::menuIconFg, { px0, px0 } );
// style::MonoIcon lockOver_icon(&iconMask56, st::menuIconFgOver, { px0, px0 } );
// topBarStartEncryption.icon = 
// topBarStartEncryption.iconOver = ~/Documents/tdesktop/Telegram/lib_extension/icons/dialogs_lock_off.png"

// style::icon _dialogsUnlockIcon = { std::in_place, MonoIcon{ &iconMask6, st::dialogsMenuIconFg, { px0, px0 } } };

// const style::color &topBarBg(_palette.topBarBg());


// { std::in_place, MonoIcon{ &iconMask56, st::menuIconFg, { px0, px0 } } },
//  { std::in_place, MonoIcon{ &iconMask56, st::menuIconFgOver, { px0, px0 } } },
//   { px4, px11 }, 0, { px0, px7 }, px40, st::defaultRippleAnimationBgOver }