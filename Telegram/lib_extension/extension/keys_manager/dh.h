#include <openssl/dh.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <vector>
#include <unordered_set>
#include <stdexcept>
#include <iostream>

namespace ext {

// Структуры для хранения параметров DH
struct DHParamsStr {
    std::string p;
    std::string g;
    std::string public_key;
    std::string private_key;
};
struct DHParamsByte {
    std::vector<unsigned char> p;
    std::vector<unsigned char> g;
    std::vector<unsigned char> public_key;
    std::vector<unsigned char> private_key;
};

class DiffieHellman {
private:
    // Безопасные для использования настройки параметров p и g
    inline static const std::unordered_set<int> valid_p_lengths = {1024, 2048, 3072, 4096, 8192};
    inline static const std::unordered_set<int> valid_g_values = {DH_GENERATOR_2, DH_GENERATOR_5};

public:
    // Пользуемся таблицами openssl для получения сруктуры DH с рандомными предпосчитанными параметрами (лучше не использовать при передаче сверхсекретной информации)
    static DH* generate_dh_fast(const int p_length = 2048);

    // Честное создание структуры DH с новыми параметрами (может много времени)
    static DH* generate_dh(const int p_lenght = 2048, const int g_value = DH_GENERATOR_2);

    // Cоздание структуры DH с уже известынми параметрами p и g
    static DH* generate_peer_dh(const std::vector<unsigned char>& p, const std::vector<unsigned char>& g);

    // Восстановление структуры DH по параметрами
    static DH* fill_dh(const DHParamsByte& dh_params);

    // Генерация открытого и закрытого ключа пользователя для алгоритма DH
    static DHParamsByte get_params_dh(DH* dh);

    // Функция вычисления общего секрета для двух собеседников
    static std::vector<unsigned char> get_shared_secret(DH* dh, const std::vector<unsigned char>& other_public_key);

    // Функция создания 256-битного ключа
    static std::vector<unsigned char> derive_256bit_key(const std::vector<unsigned char>& shared_secret);
};

class MultiDiffieHellman {
public:
    // Потсепенное получение общего ключа (other_public_key^my_private_key mod p)
    static std::vector<unsigned char> compute_shared_secret(DH* dh, const std::vector<unsigned char>& other_public_key);
};

} // namespace ext