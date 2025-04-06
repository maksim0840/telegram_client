#include "top_bar_buttons.h"


const style::IconButton& TopBarButtons::getTopBarStartEncryption() {

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

const style::IconButton& TopBarButtons::getTopBarStopEncryption() {

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