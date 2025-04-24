#include "test.h"


TEST(DiffieHellman, generate_dh_fast) {
    EXPECT_NO_THROW({
        DH* dh = ext::DiffieHellman::generate_dh_fast(1024);
        ASSERT_NE(dh, nullptr);
        DH_free(dh);
    });

    EXPECT_NO_THROW({
        DH* dh = ext::DiffieHellman::generate_dh_fast(2048);
        ASSERT_NE(dh, nullptr);
        DH_free(dh);
    });
}


TEST(DiffieHellman, generate_dh) {
    EXPECT_NO_THROW({
        DH* dh = ext::DiffieHellman::generate_dh();
        ASSERT_NE(dh, nullptr);
        DH_free(dh);
    });
}


TEST(DiffieHellman, generate_peer_dh) {
    EXPECT_NO_THROW({
        ext::DHParamsByte params_byte = keys_manager_test::get_DHParamsByte();

        DH* dh = ext::DiffieHellman::generate_peer_dh(params_byte.p, params_byte.g);
        ASSERT_NE(dh, nullptr);
        DH_free(dh);
    });
}

TEST(DiffieHellman, fill_dh) {
    EXPECT_NO_THROW({
        ext::DHParamsByte params_byte = keys_manager_test::get_DHParamsByte();

        DH* dh = ext::DiffieHellman::fill_dh(params_byte);
        ASSERT_NE(dh, nullptr);
        DH_free(dh);
    });
}

TEST(DiffieHellman, get_params_dh) {
    EXPECT_NO_THROW({
        ext::DHParamsByte params_byte = keys_manager_test::get_DHParamsByte();

        DH* dh = ext::DiffieHellman::fill_dh(params_byte);
        ASSERT_NE(dh, nullptr);

        ext::DHParamsByte new_params_byte = ext::DiffieHellman::get_params_dh(dh);

        ASSERT_EQ(params_byte.p, new_params_byte.p);
        ASSERT_EQ(params_byte.g, new_params_byte.g);
        ASSERT_EQ(params_byte.public_key, new_params_byte.public_key);
        ASSERT_EQ(params_byte.private_key, new_params_byte.private_key);

        DH_free(dh);
    });
}

TEST(DiffieHellman, get_shared_secret) {
    EXPECT_NO_THROW({
        ext::DHParamsByte params_byte = keys_manager_test::get_DHParamsByte();
        std::vector<unsigned char> other_public_key = keys_manager_test::get_random_bytes();

        DH* dh = ext::DiffieHellman::fill_dh(params_byte);
        ASSERT_NE(dh, nullptr);

        std::vector<unsigned char> shared_secret = ext::DiffieHellman::get_shared_secret(dh, other_public_key);

        ASSERT_NE(std::vector<unsigned char>(), shared_secret);

        DH_free(dh);
    });
}

TEST(DiffieHellman, derive_256bit_key) {
    EXPECT_NO_THROW({
        std::vector<unsigned char> shared_secret = keys_manager_test::get_random_bytes();

        std::vector<unsigned char> shared_secret_256 = ext::DiffieHellman::derive_256bit_key(shared_secret);
        ASSERT_EQ(256 / 8, shared_secret_256.size());
    });
}

TEST(MultiDiffieHellman, compute_shared_secret) {
    EXPECT_NO_THROW({
        ext::DHParamsByte params_byte = keys_manager_test::get_DHParamsByte();
        std::vector<unsigned char> other_public_key = keys_manager_test::get_random_bytes();

        DH* dh = ext::DiffieHellman::fill_dh(params_byte);
        ASSERT_NE(dh, nullptr);

        std::vector<unsigned char> shared_secret = ext::MultiDiffieHellman::compute_shared_secret(dh, other_public_key);

        ASSERT_NE(std::vector<unsigned char>(), shared_secret);

        DH_free(dh);
    });
}

TEST(Base64Format, encode_to_base64) {
    EXPECT_NO_THROW({
        ext::Base64Format format;
        std::vector<unsigned char> random_bytes = keys_manager_test::get_random_bytes();
        std::vector<unsigned char> random_bytes_base64 = keys_manager_test::get_random_bytes_base64();
        
        std::string encode_random_bytes_str = format.encode_to_base64(random_bytes);
        std::vector<unsigned char> encode_random_bytes(encode_random_bytes_str.begin(), encode_random_bytes_str.end());
 
        ASSERT_EQ(random_bytes_base64, encode_random_bytes);
    });
}

TEST(Base64Format, decode_from_base64) {
    EXPECT_NO_THROW({
        ext::Base64Format format;
        std::vector<unsigned char> random_bytes = keys_manager_test::get_random_bytes();

        std::vector<unsigned char> random_bytes_base64 = keys_manager_test::get_random_bytes_base64();
        std::string random_bytes_base64_str(random_bytes_base64.begin(), random_bytes_base64.end());

        std::vector<unsigned char> decode_random_bytes = format.decode_from_base64(random_bytes_base64_str);

        ASSERT_EQ(random_bytes, decode_random_bytes);
    });
}

TEST(RsaKeyManager, rsa_valid) {
    EXPECT_NO_THROW({
        ext::RsaKeyManager rsa_manager;
        std::vector<unsigned char> random_bytes = keys_manager_test::get_random_bytes();
        random_bytes = std::vector<unsigned char>(random_bytes.begin(), random_bytes.begin() + 40);

        std::string message(random_bytes.begin(), random_bytes.end());

        auto public_private = rsa_manager.create_key();
        std::string encrypted_message = rsa_manager.encrypt_message(message, public_private.first);
        std::string decrypted_message = rsa_manager.decrypt_message(encrypted_message, public_private.second);

        ASSERT_EQ(message, decrypted_message);
        ASSERT_NE(message, encrypted_message);
    });
}

TEST(RsaKeyManager, rsa_invalid_too_large) {
    EXPECT_THROW({
        ext::RsaKeyManager rsa_manager;
        std::vector<unsigned char> random_bytes = keys_manager_test::get_random_bytes();

        std::string message(random_bytes.begin(), random_bytes.end());

        auto public_private = rsa_manager.create_key();
        std::string encrypted_message = rsa_manager.encrypt_message(message, public_private.first);

    }, std::runtime_error);
}

TEST(AesKeyManager, aes_solo) {
    EXPECT_NO_THROW({
        ext::AesKeyManager aes_manager;
        std::vector<unsigned char> random_bytes = keys_manager_test::get_random_bytes();

        std::string message(random_bytes.begin(), random_bytes.end());

        std::string key = aes_manager.create_key_solo();
        std::string encrypted_message = aes_manager.encrypt_message(message, key);
        std::string decrypted_message = aes_manager.decrypt_message(encrypted_message, key);

        ASSERT_EQ(message, decrypted_message);
        ASSERT_NE(message, encrypted_message);
    });
}

TEST(AesKeyManager, aes_duo) {
    EXPECT_NO_THROW({
        ext::AesKeyManager aes_manager;
        std::vector<unsigned char> random_bytes = keys_manager_test::get_random_bytes();

        std::string message(random_bytes.begin(), random_bytes.end());

        ext::DHParamsStr dh1 = aes_manager.get_dh_params();
        ext::DHParamsStr dh2 = aes_manager.get_dh_params_secondly(dh1.p, dh1.g);

        ASSERT_EQ(dh1.p, dh2.p);
        ASSERT_EQ(dh1.g, dh2.g);
        ASSERT_NE(dh1.public_key, dh2.public_key);
        ASSERT_NE(dh1.private_key, dh2.private_key);

        std::string key1 = aes_manager.create_key_duo(dh1, dh2.public_key);
        std::string key2 = aes_manager.create_key_duo(dh2, dh1.public_key);

        ASSERT_EQ(key1, key2);

        std::string encrypted_message1 = aes_manager.encrypt_message(message, key1);
        std::string decrypted_message1 = aes_manager.decrypt_message(encrypted_message1, key1);

        ASSERT_EQ(message, decrypted_message1);
        ASSERT_NE(message, encrypted_message1);

        std::string encrypted_message2 = aes_manager.encrypt_message(message, key2);
        std::string decrypted_message2 = aes_manager.decrypt_message(encrypted_message2, key2);

        ASSERT_EQ(message, decrypted_message2);
        ASSERT_NE(message, encrypted_message2);
    });
}

TEST(AesKeyManager, aes_trio) {
    EXPECT_NO_THROW({
        ext::AesKeyManager aes_manager;
        std::vector<unsigned char> random_bytes = keys_manager_test::get_random_bytes();

        std::string message(random_bytes.begin(), random_bytes.end());

        ext::DHParamsStr dh1 = aes_manager.get_dh_params();
        ext::DHParamsStr dh2 = aes_manager.get_dh_params_secondly(dh1.p, dh1.g);
        ext::DHParamsStr dh3 = aes_manager.get_dh_params_secondly(dh1.p, dh1.g);

        ASSERT_EQ(dh1.p, dh2.p);
        ASSERT_EQ(dh2.p, dh3.p);
        ASSERT_EQ(dh1.g, dh2.g);
        ASSERT_EQ(dh2.g, dh3.g);
        ASSERT_NE(dh1.public_key, dh2.public_key);
        ASSERT_NE(dh2.public_key, dh3.public_key);
        ASSERT_NE(dh1.public_key, dh3.public_key);
        ASSERT_NE(dh1.private_key, dh2.private_key);
        ASSERT_NE(dh2.private_key, dh3.private_key);
        ASSERT_NE(dh1.private_key, dh3.private_key);

        std::string key12 = aes_manager.сreate_key_multi(dh1, dh2.public_key);
        std::string key23 = aes_manager.сreate_key_multi(dh2, dh3.public_key);
        std::string key13 = aes_manager.сreate_key_multi(dh1, dh3.public_key);

        std::string key1 = aes_manager.сreate_key_multi(dh1, key23, true);
        std::string key2 = aes_manager.сreate_key_multi(dh2, key13, true);
        std::string key3 = aes_manager.сreate_key_multi(dh3, key12, true);

        ASSERT_EQ(key1, key2);
        ASSERT_EQ(key2, key3);
        ASSERT_NE(key12, key1);
        ASSERT_NE(key23, key1);
        ASSERT_NE(key13, key1);
        ASSERT_NE(key12, key23);
        ASSERT_NE(key23, key13);
        ASSERT_NE(key12, key13);

        std::string encrypted_message1 = aes_manager.encrypt_message(message, key1);
        std::string decrypted_message1 = aes_manager.decrypt_message(encrypted_message1, key1);

        ASSERT_EQ(message, decrypted_message1);
        ASSERT_NE(message, encrypted_message1);

        std::string encrypted_message2 = aes_manager.encrypt_message(message, key2);
        std::string decrypted_message2 = aes_manager.decrypt_message(encrypted_message2, key2);

        ASSERT_EQ(message, decrypted_message2);
        ASSERT_NE(message, encrypted_message2);

        std::string encrypted_message3 = aes_manager.encrypt_message(message, key3);
        std::string decrypted_message3 = aes_manager.decrypt_message(encrypted_message3, key3);

        ASSERT_EQ(message, decrypted_message3);
        ASSERT_NE(message, encrypted_message3);
    });
}