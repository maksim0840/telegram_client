#include "dh.h"

namespace ext {

/* DiffieHellman */

DH* DiffieHellman::generate_dh_fast(const int p_length) {

    // Используем предустановленные параметры OpenSSL
    DH* dh = nullptr;
    switch (p_length) {
        case 1024: dh = DH_get_1024_160(); break;
        case 2048: dh = DH_get_2048_256(); break;
        default: throw std::runtime_error("Неподдерживаемая длина модуля p для БЫСТРОГО алгоритма DH");
    }

    if (!dh) {
        throw std::runtime_error("Не удалось получить предустановленные параметры DH");
    }

    // Генерация public и private ключей в структуре DH
    if (DH_generate_key(dh) != 1) {
        std::string error_message = "Ошибка генерации ключей DH: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        DH_free(dh);
        throw std::runtime_error(error_message);
    }

    return dh;
}


DH* DiffieHellman::generate_dh(const int p_lenght, const int g_value) {
    if (valid_p_lengths.find(p_lenght) == valid_p_lengths.end()) {
        throw std::runtime_error("Неподдерживаемая длина модуля p для алгоримта DH");
    }
    if (valid_g_values.find(g_value) == valid_g_values.end()) {
        throw std::runtime_error("Неподдерживаемое значение группы g для алгоримта DH");
    }

    // Создание структуры для алгоритма DH
    DH* dh = DH_new();
    if (!dh) {
        throw std::runtime_error("Не удалось создать структуру DH");
    }

    // Заполнение параметрами g (группа) и p (модуль) из выражения g^a mod p
    if (DH_generate_parameters_ex(dh, p_lenght, DH_GENERATOR_2, nullptr) != 1) { 
        std::string error_message = "Ошибка генерации параметров DH" + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        DH_free(dh);
        throw std::runtime_error(error_message);
    }

    // Генерация public и private ключей в структуру DH
    if (DH_generate_key(dh) != 1) {
        std::string error_message = "Ошибка генерации ключей DH" + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        DH_free(dh);
        throw std::runtime_error(error_message);
    }

    return dh;
}

DH* DiffieHellman::generate_peer_dh(const std::vector<unsigned char>& p, const std::vector<unsigned char>& g) {
    // Создание структуры для алгоритма DH
    DH* dh = DH_new();
    if (!dh) {
        throw std::runtime_error("Не удалось создать структуру DH");
    }

    // Преобразуем векторы байтов в BIGNUM-ы
    BIGNUM* p_bn = BN_bin2bn(p.data(), static_cast<int>(p.size()), nullptr);
    BIGNUM* g_bn = BN_bin2bn(g.data(), static_cast<int>(g.size()), nullptr);
    if (!p_bn || !g_bn) {
        if (p_bn) BN_free(p_bn);
        if (g_bn) BN_free(g_bn);
        DH_free(dh);
        throw std::runtime_error("Ошибка преобразования параметров p и g в BIGNUM");
    }

    // Устанавливаем параметры p и g в структуру DH.
    if (DH_set0_pqg(dh, p_bn, nullptr, g_bn) != 1) {
        BN_free(p_bn);
        BN_free(g_bn);
        DH_free(dh);
        throw std::runtime_error("Ошибка установки параметров p и g");
    }

    // Генерируем пару ключей в структуру DH (приватный и публичный)
    if (DH_generate_key(dh) != 1) {
        std::string error_message = "Ошибка генерации ключей DH" + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        DH_free(dh);
        throw std::runtime_error(error_message);
    }

    return dh;
}

DH* DiffieHellman::fill_dh(const DHParamsByte& dh_params) {
    if (dh_params.p.empty() || dh_params.g.empty() || dh_params.public_key.empty() || dh_params.private_key.empty()) {
        throw std::runtime_error("Передача пустого параметра в структуру DH");
    }

    // Создаём пустую структуру DH
    DH* dh = DH_new();
    if (!dh) {
        throw std::runtime_error("Не удалось создать структуру DH");
    }

    // Преобразуем векторы в BIGNUM
    BIGNUM* p_bn = BN_bin2bn(dh_params.p.data(), dh_params.p.size(), nullptr);
    BIGNUM* g_bn = BN_bin2bn(dh_params.g.data(), dh_params.g.size(), nullptr);
    BIGNUM* public_key_bn = BN_bin2bn(dh_params.public_key.data(), dh_params.public_key.size(), nullptr);
    BIGNUM* private_key_bn = BN_bin2bn(dh_params.private_key.data(), dh_params.private_key.size(), nullptr);

    if (!p_bn || !g_bn || !public_key_bn || !private_key_bn) {
        BN_free(p_bn);
        BN_free(g_bn);
        BN_free(public_key_bn);
        BN_free(private_key_bn);
        DH_free(dh);
        throw std::runtime_error("Не удалось преобразовать параметры в BIGNUM");
    }

    // Устанавливаем параметры p и g в структуру DH
    if (DH_set0_pqg(dh, p_bn, nullptr, g_bn) != 1) {
        BN_free(p_bn);
        BN_free(g_bn);
        BN_free(public_key_bn);
        BN_free(private_key_bn);
        DH_free(dh);
        throw std::runtime_error("Не удалось установить параметры p и g в DH");
    }

    // Устанавливаем public и private ключи в структуру DH
    if (DH_set0_key(dh, public_key_bn, private_key_bn) != 1) {
        BN_free(public_key_bn);
        BN_free(private_key_bn);
        DH_free(dh);
        throw std::runtime_error("Не удалось установить ключи в DH");
    }

    return dh;
}


DHParamsByte DiffieHellman::get_params_dh(DH* dh) {
    // Генерируем ключи, если они ещё не созданы
    if (!DH_generate_key(dh)) {
        std::string error_message = "Ошибка на этапе генерации DH ключей" + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        DH_free(dh);
        throw std::runtime_error(error_message);
    }
    
    const BIGNUM* p = nullptr;
    const BIGNUM* g = nullptr;
    const BIGNUM* public_key = nullptr;
    const BIGNUM* private_key = nullptr;

    // Получаем параметры и ключи
    DH_get0_pqg(dh, &p, nullptr, &g);
    DH_get0_key(dh, &public_key, &private_key);
    if (!p || !g || !public_key || !private_key) {
        throw std::runtime_error("Параметры DH сгенерированы некорректно");
    }

    // Размеры параметров
    int p_size = BN_num_bytes(p);
    int g_size = BN_num_bytes(g);
    int public_key_size = BN_num_bytes(public_key);
    int private_key_size = BN_num_bytes(private_key);

    DHParamsByte params;
    params.p.resize(p_size);
    params.g.resize(g_size);
    params.public_key.resize(public_key_size);
    params.private_key.resize(private_key_size);

    // Преобразование BigNum-ов в вектор байтов с использованием BN_bn2binpad для сохранения ведущих нулей
    if (BN_bn2binpad(p, params.p.data(), p_size) < 0) {
        throw std::runtime_error("Ошибка при конвертации параметра p");
    }
    if (BN_bn2binpad(g, params.g.data(), g_size) < 0) {
        throw std::runtime_error("Ошибка при конвертации параметра g");
    }
    if (BN_bn2binpad(public_key, params.public_key.data(), public_key_size) < 0) {
        throw std::runtime_error("Ошибка при конвертации публичного ключа");
    }
    if (BN_bn2binpad(private_key, params.private_key.data(), private_key_size) < 0) {
        throw std::runtime_error("Ошибка при конвертации приватного ключа");
    }

    return params;
}

std::vector<unsigned char> DiffieHellman::get_shared_secret(DH* dh, const std::vector<unsigned char>& other_public_key) {

    // Преобразуем вектор в BIGNUM
    BIGNUM* other_key_bn = BN_bin2bn(other_public_key.data(), other_public_key.size(), nullptr);
    if (!other_key_bn) {
        throw std::runtime_error("Ошибка преобразования ключа в BIGNUM");
    }

    // Создание секрета
    std::vector<unsigned char> shared_secret(DH_size(dh));

    int secret_len = DH_compute_key(shared_secret.data(), other_key_bn, dh);
    BN_free(other_key_bn);
    if (secret_len == -1) {
        std::string error_message = "Ошибка вычисления общего секрета" + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        DH_free(dh);
        throw std::runtime_error(error_message);
    }
    
    shared_secret.resize(secret_len);
    return shared_secret;
}

std::vector<unsigned char> DiffieHellman::derive_256bit_key(const std::vector<unsigned char>& shared_secret) {
    std::vector<unsigned char> key(SHA256_DIGEST_LENGTH);

    // Применяем хэш функцию SHA256 для изменения размера секрета ровно до 256 бит
    SHA256(shared_secret.data(), shared_secret.size(), key.data());
    return key;
}

/* MultiDiffieHellman */

std::vector<unsigned char> MultiDiffieHellman::compute_shared_secret(DH* dh, const std::vector<unsigned char>& other_public_key) {

    // Получаем свой приватный ключ
    const BIGNUM* my_private_key = DH_get0_priv_key(dh);
    if (!my_private_key) {
        throw std::runtime_error("Ошибка получения приватного ключа");
    }

    // Преобразуем публчиный ключ собеседника в BIGNUM
    BIGNUM* other_key_bn = BN_bin2bn(other_public_key.data(), other_public_key.size(), nullptr);
    if (!other_key_bn) {
        throw std::runtime_error("Ошибка преобразования ключа в BIGNUM");
    }

    // Инициализируем контекст
    BN_CTX* ctx = BN_CTX_new();
    if (!ctx) {
        BN_free(other_key_bn);
        throw std::runtime_error("Ошибка выделения памяти для BN_CTX");
    }

    // Инициализируем shared_secret
    BIGNUM* shared_secret = BN_new();
    if (!shared_secret) {
        BN_free(other_key_bn);
        BN_CTX_free(ctx);
        throw std::runtime_error("Ошибка выделения памяти для BIGNUM shared_secret");
    }

    // Вычисляем промежуточный shared_secret (окончательным он станет после возведения в степени всех приватных ключей)
    if (!BN_mod_exp(shared_secret, other_key_bn, my_private_key, DH_get0_p(dh), ctx)) {
        std::string error_message = "Ошибка возведения в степень: " + std::string(ERR_error_string(ERR_get_error(), nullptr));
        ERR_clear_error();
        BN_free(other_key_bn);
        BN_free(shared_secret);
        BN_CTX_free(ctx);
        throw std::runtime_error(error_message);
    }

    // Конвертируем BIGNUM в вектор байтов
    std::vector<unsigned char> shared_secret_bytes(BN_num_bytes(shared_secret));
    BN_bn2bin(shared_secret, shared_secret_bytes.data());

    BN_free(other_key_bn);
    BN_free(shared_secret);
    BN_CTX_free(ctx);

    return shared_secret_bytes;
}

} // namespace ext