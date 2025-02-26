#!/bin/bash

# Очистка старых объектов и библиотек
echo "🧹 Очистка предыдущих компиляций..."
rm -f dh.o keys_format.o libdh.a libkeys_format.a keys_manager

# Компиляция dh.cpp в объектный файл
echo "🛠 Компиляция dh.cpp..."
g++ -std=c++17 -c dh.cpp -o dh.o -I/usr/include -I/usr/include/x86_64-linux-gnu || { echo "❌ Ошибка компиляции dh.cpp"; exit 1; }

# Создание статической библиотеки libdh.a
echo "📦 Создание библиотеки libdh.a..."
ar rcs libdh.a dh.o || { echo "❌ Ошибка создания libdh.a"; exit 1; }

# Компиляция keys_format.cpp в объектный файл
echo "🛠 Компиляция keys_format.cpp..."
g++ -std=c++17 -c keys_format.cpp -o keys_format.o -I/usr/include -I/usr/include/x86_64-linux-gnu || { echo "❌ Ошибка компиляции keys_format.cpp"; exit 1; }

# Создание статической библиотеки libkeys_format.a
echo "📦 Создание библиотеки libkeys_format.a..."
ar rcs libkeys_format.a keys_format.o || { echo "❌ Ошибка создания libkeys_format.a"; exit 1; }

# Компиляция главного файла и линковка с библиотеками
echo "🚀 Компиляция keys_manager..."
g++ -std=c++17 -o keys_manager keys_manager.cpp -L. -ldh -lkeys_format -I/usr/include -I/usr/include/x86_64-linux-gnu -lssl -lcrypto || { echo "❌ Ошибка компиляции keys_manager"; exit 1; }

# Запуск программы
echo "🎯 Запуск keys_manager..."
./keys_manager

# Очистка временных файлов после выполнения
echo "🧹 Удаление временных файлов..."
rm -f dh.o keys_format.o libdh.a libkeys_format.a
