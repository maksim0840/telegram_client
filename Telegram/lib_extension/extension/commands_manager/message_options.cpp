#include "message_options.h"

namespace ext {

bool Message::fill_options(const std::string& text_with_options) {
    std::regex re(R"(\[/([01]{9})-(\d+)-(\d+)-(\d+)-(\d+)\] (.*))");
    std::smatch match;

    if (!std::regex_match(text_with_options, match, re)) {
        return false;
    }

    std::string match1_str = match[1].str();
    rsa_init = match1_str[0] - '0';
    rsa_form = match1_str[1] - '0';
    rsa_use = match1_str[2] - '0';
    aes_init = match1_str[3] - '0';
    aes_form = match1_str[4] - '0';
    aes_use = match1_str[5] - '0';
    end_key_forming = match1_str[6] - '0';
    end_encryption = match1_str[7] - '0';
    dh_fastmode = match1_str[8] - '0';

    rsa_key_n = std::stoi(match[2].str());
    aes_key_n = std::stoi(match[3].str());
    last_peer_n = std::stoi(match[4].str());
    rsa_key_len = std::stoi(match[5].str());

    text = match[6].str();
    return true;
}

std::string Message::get_text_with_options() {
    char text_with_options[MAX_TELEGRAM_MESSAGE_LEN];

    std::snprintf(text_with_options, MAX_TELEGRAM_MESSAGE_LEN, "[/%i%i%i%i%i%i%i%i%i-%i-%i-%i-%i] %s", \
        rsa_init, rsa_form, rsa_use, aes_init, aes_form, aes_use, end_key_forming, end_encryption, dh_fastmode, rsa_key_n, aes_key_n, last_peer_n, rsa_key_len, text.c_str());

    return std::string(text_with_options);
}

} // namespace ext