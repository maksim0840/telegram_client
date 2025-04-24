# [Telegram Desktop][telegram_desktop] – Official Messenger

# Пользователский клиент
-является дополнением к основнуму клиенту talegram версии 5.9.0 Клиент создан для безопасной работы, отпраки и полчучения сообщений.

# Изменяемый код
Чтобы быть точно уверенным, что отправляемые сообщения шифруются и телеграм не имеет к ним доступ, требуется перехватывать сообщение на более ранних стадиях его формирования. Сообщения будут шифроваться на этапе захвата графическим интерфейсом строки из input_field.

# Сборка оиргинального клиента
Оригинальный клиент и инструкция по билду находится в ветке telegram-5.9.0 После сборки готовый клиент будет находится в папке tdesktop/out/Release

# Ход работы:

1) При нажатии на кнопку "отправить", вызывается функция getTextWithAppliedMarkdown (находится в Telegram/lib_ui/ui/widgets/fields/input_field.cpp). Она возрващает объект input-поля, содержащий отправляемое сообщение.

2) Для реализации шифровки сообщений, которые отправляются через набор текста в input-поле требуется знание id пользователя (группы), которому было отправлено это сообщение. Эта информация содержится в методе send (находится в Telegram/SourceFiles/history/history_widget.cpp). Эта функиця вызывает getTextWithAppliedMarkdown из первого пункта, но так же ещё позволяет получить id пользователя/группы к которому отправляется сообщение) - с помощью этой функции реализована шифровка сообщений, которые отправляются в большой групповой, парный и saved masseges чаты. Поддерживается отправка сообщений следующих видов написания: обычная отправка, ответ, редактирование (за редактирование отвечает другая функция этого же файла - prepareTextForEditMsg).

3) Не все типы отправляемых сообщений поддерживаются функцией getTextWithAppliedMarkdown: Шифровка пересылаемых сообщений (forwared messages) не связана с функцией getTextWithAppliedMarkdown, надо находить другую функцию или смириться с тем фактом, что пересылать сообщенния из зашифрованных чатов является не лучшей идеей.

4) Была создана папка (Telegram/lib_extension) для основного кода расширения. CMakeLists расширения был создан на основе CMakeLists из папки Telegram/codegen/codegen/common. На данный момент папка проекта использует только базовые библиотеки c++ и библиотеку qt, когторая подключается благодаря связывания папки со всей структурой проекта (данная папка была добавлена в главный cmakelists файл проекта telegram).

5) В качестве функции, которая шифрует сообщение в файл с расширением к основному клиенту была добавлена функция encrypt_the_message (папки Telegram/lib_extension/extension/encryption). На вход она принимает ссылку на текст сообщения и константый id пользователя, по которому определяется способ шифровки и изменяется сообщение)

6) За сообщение связанное с прикреплёнными файлами в диалоге отвечает другой метод другого файла проекта: send_files_box.cpp из папки Telegram/SourceFiles/boxes/send_files_box.cpp

7) В папку с расширением был добавлен код для реализации шифрования и генерации ключей при помощи библиотеки openssl, которая теперь также подключается к файлам расширения. (файлы: Telegram/lib_extension/extension/encryption/keys_creator)

8) В папку с расширением был добавлен код для реализации базы данных sqllite, хранящей информацию о ключах. Сама база временно хранится в папке с расширением по пути Telegram/lib_extension/db/keys.db, а сам код Telegram/lib_extension/extension/local_storage/local_storage





*9) Telegram/lib_ui/ui/text/text_renderer.cpp - метод draw в файле Telegram/lib_ui/ui/text/text_renderer.cpp отвечает за отрисовку сообщений в графическом интрфейсе и постоянно вызывается, что означает она не подходит для эффективного декодирования

10) решил снова проверить файл history_widget и нашёл несколько функций отвечающие за загрузку сообщений в кэш отвечает функция preloadHistoryByScroll. Данная функция вызывает loadMessages если в кэше нет сообщений требуемого чата или функцию loadMessagesDown которая вызвается каждый раз при появлении нового сообщения или при скролле страницы.

Функция loadMessages вызывает messagesReceived которая подгружает предыдущие сообщения пачкой по несколько штук (вывод сообщений в программе крашится если в чате есть звонки).
Функция loadMessagesDown запрашивает данные о сообщениях через session().sponsoredMessages().request(_history, nullptr). request вызывает функцию parse где обрабатываются сообщения чата.

почему то не работает поэтому я пробую через метод history.cpp createItem.
В функции parse файла Telegram/SourceFiles/data/components/sponsored_messages.cpp Я сделал чтение сообщений


/SourceFiles/history/view/history_view_message.cpp
Telegram/SourceFiles/history/history_widget.cpp

11) Было принято решение дополнительно все ключи шифровать в строковом формате Base64. Т.к. данный тип исключает специальные символы и легко пердаются через сообщения в телеграм.

12) Шифрование и обмен ключами по алгоритму DH

13) Получение сообщений через функцию history.cpp (инофмрация о сообщении содержится в классе MTPDmessage в приватном поле)

14) Класс MTPDmessage автогенерируется в /out/Telegram/gen/scheme.h (при каждой новой сборке генерируется заново), так что для получения доступа к приватным полям данного класса было принято решение дописать несколько строк в файл, который генерирует scheme.h

15) Файл который генерирует MTPDmessage лежит по пути Telegram/lib_tl/tl/generate_tl.py - в данный файл были добавлены строчки, которые создают внутри класса friend с функцией MTPDmessage_private_fields_access - она отвечает за подмену сообщения в данном классе

16) Появилась необходимость автоматически в headless режиме отправлять сообщения (для формирования ключа) - для этого хорошо подходит функция sendMessage из файла apiwrap.cpp (доступ к которой был организован в коде history.cpp)

17) Отправка сообщений через apiwrap работает, но слишком хорошо (настолько что отправленное собеседником сообщение отображается на экране позже, чем ответ на него) - было принято решение добавить задержку 1 сек перед отправкой. - это только из эстетических побуждений, т.к. для всех других пользователей последовательность отправляемых сообщений правильная и тем более при перезагрузке клиента она также исправляется. Данное решение прелполагает изменение кода оригинального клиента на +3/4 строчки. (также в телеграмме есть встроенная функция отложенных сообщение scheduled messages, но при отправки данного типа сообщений открывается специальное меню, вызов которого нельзя отменить, не исправляя большое кол-во оригинального кода, и оно также не радует глаз, поэтому задержка была написана втупую через потоки thread)

18) Добавил включение в файле history_widget.cpp в функции handlePeerUpdate, для обновления состояния чата (сбор инофрмации о пользователях чата)

19) перед каждым сообщением (которое обрабатывается расширением) будет стоять флаги в формате [/000000-...-...-...]

20) добавил кнопки начала шифровки и его конца (были изменены файлы Telegram/SourceFiles/history/view/history_view_top_bar_widget.cpp/.h)

Стоит изучить:

Инофрмация из файла Telegram/SourceFiles/config.h (дополнения по api_id, api_hash):

```
#if defined TDESKTOP_API_ID && defined TDESKTOP_API_HASH

constexpr auto ApiId = TDESKTOP_API_ID;
constexpr auto ApiHash = QT_STRINGIFY(TDESKTOP_API_HASH);

#else // TDESKTOP_API_ID && TDESKTOP_API_HASH

// To build your version of Telegram Desktop you're required to provide
// your own 'api_id' and 'api_hash' for the Telegram API access.
//
// How to obtain your 'api_id' and 'api_hash' is described here:
// https://core.telegram.org/api/obtaining_api_id
//
// If you're building the application not for deployment,
// but only for test purposes you can comment out the error below.
//
// This will allow you to use TEST ONLY 'api_id' and 'api_hash' which are
// very limited by the Telegram API server.
//
// Your users will start getting internal server errors on login
// if you deploy an app using those 'api_id' and 'api_hash'.

#error You are required to provide API_ID and API_HASH.

constexpr auto ApiId = 17349;
constexpr auto ApiHash = "344583e45741c457fe1862106095a5eb";
```
ApiId используется в файлах:

- ./Telegram/SourceFiles/api/api_authorizations.cpp
- ./Telegram/SourceFiles/core/crash_reports.cpp
- ./Telegram/SourceFiles/core/crash_report_window.cpp
- ./Telegram/SourceFiles/mtproto/session_private.cpp
- ./Telegram/SourceFiles/intro/intro_qr.cpp
- ./Telegram/SourceFiles/intro/intro_phone.cpp
- ./Telegram/configure.py

ApiHash используется в файлах: 

- ./Telegram/SourceFiles/intro/intro_qr.cpp
- ./Telegram/SourceFiles/intro/intro_phone.cpp
- ./Telegram/configure.py

# Установка и запуск

[Скачать последнюю версию (Release)](https://github.com/maksim0840/telegram_client/releases/tag/v1.0)


# Сборка исходных файлов пользовательского клиента + рекомендации

Расширенный клиент телеграмма находится в main ветке. Сборка происходит почти так же, как и в оригинальном клиенте. Сборка тяжёлого проекта занимает большое количество времени и ресурсов. Для наиболее эфективной сборки выставите подходящее количество CPU, RAM и выделите достаточное количество памяти на диске для сборки проекта.

Рекомендации по выставлению CPU, RAM для сборки:
```
nproc # CPUS (Для проекта рекомендуется выставить на 1-2 ядра меньше, чтобы система не зависла во время сборки)

free -h | awk '/Mem:/ {print $7}' # RAM (Для проекта доступную оперативную память рекомендуется выставить на 1-2 GB меньше, чтобы система не убила процесс из-за нехватки памяти)

```

Рекомендации по свободной памяти на диске:

Клиент собирается в 2 этапа: сборка внешних библиотек и компиляция файлов самого проекта. Эти этапы требуют намного больше памяти, чем занимает итоговый результат, т.к. происходит сборка тяжелых библиотек, которые требуют большое количество памяти для временных сборочных данных. Ниже привидены графики занимаемой памяти и примерное время сборки всего проекта.

Аппаратная конфигурация тестовой среды:
- CPU: AMD Ryzen 7 3750H (8 ядер, из которых в сборке учавствовало 6)
- RAM: DDR4 3200MHz SODIMM Micron 8GB X 2 (available 11 GB, из которых в сборке учавствовало 10)
- STORAGE (SSD): INTEL SSDPEKNW512G (468 GB, из которых свободны 354 GB)

![stage1](https://github.com/user-attachments/assets/9033ebba-0f02-411c-8e00-ab178ac6057d)

![stage2](https://github.com/user-attachments/assets/302ac3fe-1893-4081-a19c-49b6231d5cc8)

- Итоговое время сборки: 5 часов 8 минут
- Итоговый размер проекта: 95 GB
- Максимальное потребление памяти: 121 GB

Инструкция по сборке:

0) Перед началом сборки должны быть установлены git, gcc, cmake, python, poetry, docker

1) Скачать файлы проекта
```
git clone --recursive https://github.com/maksim0840/telegram_client
```
2) Задать параметры CPU и RAM, удовлетворяющие вашей конфигурации в файле telegram_client/config.env

3) Запустить установку внешних библиотек
```
./telegram_client/Telegram/build/prepare/linux.sh
```

4) Запустить сборку проекта (circle_build.sh - бесконечный цикл запуска сборки с игнорированием ошибок (использовать, если уверены в корректности всех файлов, build.sh - обычый запуск сборки с остановкой контейнера при возникновении даже незначительных ошибок)
```
./telegram_client/circle_build.sh
```
 
```
