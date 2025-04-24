#include "keys_manager.h"

namespace ext {

/* class RsaKeyManager */

void RsaKeyManager::fill() {
    rsa = RSA_new();
    if (!rsa) throw std::runtime_error("Ошибка создания RSA");

    bn = BN_new();
    if (!bn) throw std::runtime_error("Ошибка создания BIGNUM");

    public_bio = BIO_new(BIO_s_mem());
    if (!public_bio) throw std::runtime_error("Ошибка создания BIO для публичного ключа");

    private_bio = BIO_new(BIO_s_mem());
    if (!private_bio) throw std::runtime_error("Ошибка создания BIO для приватного ключа");
}

void RsaKeyManager::clear() {
    if (rsa) RSA_free(rsa);
    if (bn) BN_free(bn);
    if (public_bio) BIO_free_all(public_bio);
    if (private_bio) BIO_free_all(private_bio);

    rsa = nullptr;
    bn = nullptr;
    public_bio = nullptr;
    private_bio = nullptr;
}

std::string RsaKeyManager::extract_bio_data(BIO* bio) {
    char* data = nullptr;
    long length = BIO_get_mem_data(bio, &data);
    if (!data || length <= 0) {
        throw std::runtime_error("Ошибка извлечения данных из BIO");
    }
    return std::string(data, length);
}

RsaKeyManager::RsaKeyManager() {
    rsa = nullptr;
    bn = nullptr;
    public_bio = nullptr;
    private_bio = nullptr;
}

RsaKeyManager::~RsaKeyManager() {
    clear();
}

std::pair<std::string, std::string> RsaKeyManager::create_key(const int key_len) {
    if (key_len < 512 || key_len > 8192 || key_len % 64 != 0) {
        throw std::invalid_argument("Неподдерживаемая длина ключа RSA");
    }
    fill();

    // Устанавливаем начальное значение для генерации
    if (!BN_set_word(bn, RSA_F4)) {
        std::string error_message = "Ошибка установки BIGNUM: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        throw std::runtime_error(error_message);
    }

    // Заполняем структуру ключей
    if (!RSA_generate_key_ex(rsa, key_len, bn, nullptr)) {
        std::string error_message = "Ошибка генерации ключа: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        throw std::runtime_error(error_message);
    }

    // Получаем публичный ключ из буфера и преобразуем в строковый формат PEM
    if (!PEM_write_bio_RSAPublicKey(public_bio, rsa)) {
        std::string error_message = "Ошибка записи открытого ключа: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        throw std::runtime_error(error_message);
    }
    std::string public_key_pem = extract_bio_data(public_bio);

    // Получаем приватный ключ из буфера и преобразуем в строковый формат PEM
    if (!PEM_write_bio_RSAPrivateKey(private_bio, rsa, nullptr, nullptr, 0, nullptr, nullptr)) {
        std::string error_message = "Ошибка записи закрытого ключа: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        throw std::runtime_error(error_message);
    }
    std::string private_key_pem = extract_bio_data(private_bio);

    // Кодируем ключи дополнительно в строковый формат Base64
    std::string public_key_pem_base64 = format_converter.encode_to_base64(std::vector<unsigned char>(public_key_pem.begin(), public_key_pem.end()));
    std::string private_key_pem_base64 = format_converter.encode_to_base64(std::vector<unsigned char>(private_key_pem.begin(), private_key_pem.end()));

    clear();
    return {public_key_pem_base64, private_key_pem_base64};
}


std::string RsaKeyManager::encrypt_message(const std::string& message, const std::string& public_key) {
    fill();

    // Перешифровка ключа из формата Base64 в PEM
    std::vector<unsigned char> public_key_pem = format_converter.decode_from_base64(public_key);

    // Запись в буффер публичного ключа
    if (BIO_write(public_bio, public_key_pem.data(), public_key_pem.size()) <= 0) {
        throw std::runtime_error("Ошибка записи в буффер BIO");
    }

    // Создаём структуру rsa с заполненным public ключом
    rsa = PEM_read_bio_RSAPublicKey(public_bio, nullptr, nullptr, nullptr);
    if (!rsa) {
        std::string error_message = "Не удалось загрузить публичный ключ: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        throw std::runtime_error(error_message);
    }

    int rsa_size = RSA_size(rsa);
    std::vector<unsigned char> encrypted_data(rsa_size);

    // Получаем зашифрованное сообщение
    int result = RSA_public_encrypt(
        message.size(),
        reinterpret_cast<const unsigned char*>(message.data()),
        encrypted_data.data(),
        rsa,
        RSA_PKCS1_OAEP_PADDING
    );
    if (result == -1) {
        std::string error_message = "Ошибка шифрования RSA: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        throw std::runtime_error(error_message);
    }

    // Обрезаем лишнее и сохраняем в string в формате Base64
    encrypted_data.resize(result);
    std::string encrypted_data_base64 = format_converter.encode_to_base64(encrypted_data);

    clear();
    return encrypted_data_base64;
}


std::string RsaKeyManager::decrypt_message(const std::string& message, const std::string& private_key) {
    fill();

    // Перешифровка ключа из формата Base64 в PEM
    std::vector<unsigned char> private_key_pem = format_converter.decode_from_base64(private_key);

    // Перешифровка сообщения из формата Base64 в зашифрованное ключом
    std::vector<unsigned char> message_encrypted = format_converter.decode_from_base64(message);

    // Запись в буффер приватного ключа
    if (BIO_write(private_bio, private_key_pem.data(), private_key_pem.size()) <= 0) {
        throw std::runtime_error("Ошибка записи в буффер BIO");
    }

    // Создаём структуру rsa с заполненным private ключом
    rsa = PEM_read_bio_RSAPrivateKey(private_bio, nullptr, nullptr, nullptr);
    if (!rsa) {
        std::string error_message = "Не удалось загрузить приватный ключ: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        throw std::runtime_error(error_message);
    }

    int rsa_size = RSA_size(rsa);
    std::vector<unsigned char> decrypted_data(rsa_size, 0);

    // Получаем расшифрованное сообщение
    int result = RSA_private_decrypt(
        message_encrypted.size(),
        message_encrypted.data(),
        decrypted_data.data(),
        rsa,
        RSA_PKCS1_OAEP_PADDING
    );
    if (result == -1) {
        std::string error_message = "Ошибка расшифровки RSA: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        throw std::runtime_error(error_message);
    }

    decrypted_data.resize(result);
    
    clear();
    return std::string(decrypted_data.begin(), decrypted_data.end());
}

/* class AesKeyManager */

std::string AesKeyManager::create_key_solo() {
    size_t key_len_bytes = AES_KEY_LEN / 8;
    unsigned char key[key_len_bytes];

    // Генерация рандомных байт в количестве key_len_bytes
    if (!RAND_bytes(key, key_len_bytes)) {
        std::string error_message = "Ошибка генерации ключа AES: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        throw std::runtime_error(error_message);
    }

    // Кодируем ключь дополнительно в строковый формат Base64
    return format_converter.encode_to_base64(std::vector<unsigned char>(key, key + key_len_bytes));
}

std::string AesKeyManager::create_key_duo(const DHParamsStr& my_params, const std::string& other_public_key) {
    
    // Декодируем параметры из base64 в бинарный формат
    DHParamsByte my_params_byte;
    my_params_byte.p = format_converter.decode_from_base64(my_params.p);
    my_params_byte.g = format_converter.decode_from_base64(my_params.g);
    my_params_byte.public_key = format_converter.decode_from_base64(my_params.public_key);
    my_params_byte.private_key = format_converter.decode_from_base64(my_params.private_key);
    std::vector<unsigned char> other_public = format_converter.decode_from_base64(other_public_key);
    
    // Восстановиление структур DH
    DH* my_dh = DiffieHellman::fill_dh(my_params_byte);
    
    // Cоставление общего секрета
    std::vector<unsigned char> secret = DiffieHellman::get_shared_secret(my_dh, other_public);
    
    // Конвертация секрета в формат 256 символов
    std::vector<unsigned char> key = DiffieHellman::derive_256bit_key(secret);

    // Преобразуем в формат base64
    std::string key_base64 = format_converter.encode_to_base64(key);
    return key_base64;
}

std::string AesKeyManager::сreate_key_multi(const DHParamsStr& my_params, const std::string& other_public_key, bool is_final) {
    
    // Декодируем параметры из base64 в бинарный формат
    DHParamsByte my_params_byte;
    my_params_byte.p = format_converter.decode_from_base64(my_params.p);
    my_params_byte.g = format_converter.decode_from_base64(my_params.g);
    my_params_byte.public_key = format_converter.decode_from_base64(my_params.public_key);
    my_params_byte.private_key = format_converter.decode_from_base64(my_params.private_key);
    std::vector<unsigned char> other_public = format_converter.decode_from_base64(other_public_key);
    
    // Восстановиление структур DH
    DH* my_dh = DiffieHellman::fill_dh(my_params_byte);
    
    // Cоставление общего секрета
    std::vector<unsigned char> secret = MultiDiffieHellman::compute_shared_secret(my_dh, other_public);
    
    std::string result;
    if (is_final) {
        // Конвертация секрета в формат 256 символов
        std::vector<unsigned char> key = DiffieHellman::derive_256bit_key(secret);

        // Преобразуем общий секрет в формат base64
        result = format_converter.encode_to_base64(key);
    }
    else {
        // Преобразуем в формат base64 промежуточный результат формирования общего секрета
        result = format_converter.encode_to_base64(secret);
    }
    return result;
}

DHParamsStr AesKeyManager::get_dh_params(bool fast_mode, const int p_length, const int g_value) {
    // // Получаем структуру с новыми параметрами алгоритма DH
    DH* dh = nullptr;
    if (fast_mode) {
        dh = DiffieHellman::generate_dh_fast(p_length); // быстрый, но лучше не использовать для сверхсекретных перепесок
    }
    else {
        dh = DiffieHellman::generate_dh(p_length, g_value); // 100% надёжный, но долгий (10-20 секунд)
    }

    // Получаем сгенерированные параметры
    DHParamsByte bytes_params = DiffieHellman::get_params_dh(dh);

    // Преобразуем в формат base64
    DHParamsStr str_params;
    str_params.p = format_converter.encode_to_base64(bytes_params.p);
    str_params.g = format_converter.encode_to_base64(bytes_params.g);
    str_params.public_key = format_converter.encode_to_base64(bytes_params.public_key);
    str_params.private_key = format_converter.encode_to_base64(bytes_params.private_key);

    return str_params;
}

DHParamsStr AesKeyManager::get_dh_params_secondly(const std::string p, const std::string g) {

    // Декодируем параметры из base64 в бинарный формат
    std::vector<unsigned char> p_byte = format_converter.decode_from_base64(p);
    std::vector<unsigned char> g_byte = format_converter.decode_from_base64(g);

    // Генерируем структуру собственных параметров на основе известных
    DH* dh = DiffieHellman::generate_peer_dh(p_byte, g_byte);

    // Получаем сгенерированные параметры
    DHParamsByte bytes_params = DiffieHellman::get_params_dh(dh);

    // Преобразуем в формат base64
    DHParamsStr str_params;
    str_params.p = format_converter.encode_to_base64(bytes_params.p);
    str_params.g = format_converter.encode_to_base64(bytes_params.g);
    str_params.public_key = format_converter.encode_to_base64(bytes_params.public_key);
    str_params.private_key = format_converter.encode_to_base64(bytes_params.private_key);
    
    return str_params;
}

std::string AesKeyManager::encrypt_message(const std::string& message, const std::string& key) {

    // Расшифровка ключа из формата Base64
    std::vector<unsigned char> key_aes = format_converter.decode_from_base64(key);

    if (key_aes.size() != 32) {
        throw std::runtime_error("Неверный размер ключа AES-256.");
    }

    // iv вектор для рандомизации шифрования (чтобы одно и тоже сообщение шифровалось каждый раз по разному)
    std::vector<unsigned char> iv(16);
    if (!RAND_bytes(iv.data(), iv.size())) {
        throw std::runtime_error("Не удалось сгенерировать IV.");
    }

    // Создание структуры для хранения параметров шифрования
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Не удалось создать контекст шифрования.");
    }   

    // Заполняем структуру контекстом, необходимым для шифрования с помощью AES-256
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key_aes.data(), iv.data()) != 1) {
        std::string error_message = "Не удалось инициализировать шифрование AES-256 CBC: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error(error_message);
    }

    std::vector<unsigned char> encrypted_data(message.size() + 16);
    int len = 0;
    int encrypted_len = 0;

    // Шифровка сообщения
    if (EVP_EncryptUpdate(ctx, encrypted_data.data(), &len, reinterpret_cast<const unsigned char*>(message.data()), message.size()) != 1) {
        std::string error_message = "Не удалось зашифровать данные: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error(error_message);
    }
    encrypted_len += len;

    // Добавляем смещение по общей кратности длинны сообщения = 16 байт
    if (EVP_EncryptFinal_ex(ctx, encrypted_data.data() + encrypted_len, &len) != 1) {
        std::string error_message = "Не удалось выполнить финальную стадию шифрования: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error(error_message);
    }
    encrypted_len += len;


    // Обрезаем лишнее и сохраняем в string в формате Base64
    encrypted_data.resize(encrypted_len);
    encrypted_data.insert(encrypted_data.begin(), iv.begin(), iv.end());
    std::string encrypted_data_base64 = format_converter.encode_to_base64(encrypted_data);

    EVP_CIPHER_CTX_free(ctx);
    return encrypted_data_base64;
}


std::string AesKeyManager::decrypt_message(const std::string& message, const std::string& key) {

    // Расшифровка ключа из формата Base64
    std::vector<unsigned char> key_aes = format_converter.decode_from_base64(key);

    // Перешифровка сообщения из формата Base64 в зашифрованное ключом
    std::vector<unsigned char> message_encrypted = format_converter.decode_from_base64(message);

    if (key_aes.size() != 32) {
        throw std::runtime_error("Неверный размер AES-256 ключа.");
    }
    if (message_encrypted.size() < 16) {
        throw std::runtime_error("Сообщение слишком короткое или не содержит IV");
    }

    // Разбить сообщение на 2 части (рандомизированный вектор из 16 байт и само зашифрованное сообщение)
    std::vector<unsigned char> iv(message_encrypted.begin(), message_encrypted.begin() + 16);
    std::vector<unsigned char> encrypted_data(message_encrypted.begin() + 16, message_encrypted.end());

    // Создание структуры для хранения параметров шифрования
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("Не удалось создать контекст расшифровки.");
    }

    // Заполняем структуру контекстом, необходимым для шифрования с помощью AES-256
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key_aes.data(), iv.data()) != 1) {
        std::string error_message = "Не удалось инициализировать шифрование AES-256 CBC: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error(error_message);
    }

    std::vector<unsigned char> decrypted_data(encrypted_data.size());
    int len = 0;
    int decrypted_len = 0;

    // Расшифровка сообщения
    if (EVP_DecryptUpdate(ctx, decrypted_data.data(), &len, encrypted_data.data(), encrypted_data.size()) != 1) {
        std::string error_message = "Не удалось расшифровать данные: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error(error_message);
    }
    decrypted_len += len;

    // Удаляет лишнее смещение, которое осталось после шифровки
    if (EVP_DecryptFinal_ex(ctx, decrypted_data.data() + decrypted_len, &len) != 1) {
        std::string error_message = "Не удалось выполнить финальную стадию расшифровки: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        EVP_CIPHER_CTX_free(ctx);
        throw std::runtime_error(error_message);
    }
    decrypted_len += len;

    decrypted_data.resize(decrypted_len);

    EVP_CIPHER_CTX_free(ctx);
    return std::string(decrypted_data.begin(), decrypted_data.end());
}

} // namespace ext