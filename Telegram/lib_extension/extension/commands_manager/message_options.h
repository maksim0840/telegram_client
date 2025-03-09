#include <string>
#include <regex>
#include <iostream>

#define MAX_TELEGRAM_MESSAGE_LEN 4096

#pragma once

struct Message {
    // Опции перед сообщением
    bool rsa_init = false;
    bool rsa_form = false;
    bool rsa_use = false;
    bool aes_init = false;
    bool aes_form = false;
    bool aes_use = false;
    bool dh_fastmode = true;
    int rsa_key_n = 0;
    int aes_key_n = 0;
    int last_peer_n = 0;
    int rsa_key_len = 0;

    // Сообщение
    std::string text;

    // Заполнить структуру параметрами сообщения из строки
    bool fill_options(const std::string& text_with_options);

    // Получить строку с параметрами для отрпавки
    std::string get_text_with_options();
};
