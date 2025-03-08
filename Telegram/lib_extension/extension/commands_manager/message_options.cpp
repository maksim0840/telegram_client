#include "message_options.h"

bool Message::fill_options(const std::string& text_with_options) {
    std::regex re(R"(\[/([01]{6})-(\d+)-(\d+)-(\d+)\] (.*))");
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

    rsa_key_n = std::stoi(match[2].str());
    aes_key_n = std::stoi(match[3].str());
    last_peer_n = std::stoi(match[4].str());

    text = match[5].str();
    return true;
}

std::string Message::get_text_with_options() {
    char text_with_options[MAX_TELEGRAM_MESSAGE_LEN];

    std::snprintf(text_with_options, MAX_TELEGRAM_MESSAGE_LEN, "[/%i%i%i%i%i%i-%i-%i-%i] %s", \
        rsa_init, rsa_form, rsa_use, aes_init, aes_form, aes_use, rsa_key_n, aes_key_n, last_peer_n, text.c_str());

    return std::string(text_with_options);
}


int main(void) {
    Message m;
    std::cout << m.fill_options("[/000111-123-123-342] asdasdadasdasasda");

    std::cout << m.rsa_init << '\n';
    std::cout << m.rsa_form << '\n';
    std::cout << m.rsa_use << '\n';
    std::cout << m.aes_init << '\n';
    std::cout << m.aes_form << '\n';
    std::cout << m.aes_use << '\n';
    std::cout << m.rsa_key_n << '\n';
    std::cout << m.aes_key_n << '\n';
    std::cout << m.last_peer_n << '\n';
    std::cout << m.text << '\n';
    std::cout << m.get_text_with_options() << '\n';

    return 0;
}