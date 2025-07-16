#pragma once

#include <memory>
#include <string>
#include <vector>
#include <array>
#include <mutex>

#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>

namespace securechat::crypto {

// AES-256 key size
constexpr size_t AES_KEY_SIZE = 32;
constexpr size_t AES_IV_SIZE = 16;
constexpr size_t AES_BLOCK_SIZE = 16;

// RSA key size
constexpr int RSA_KEY_SIZE = 2048;

// HMAC key size
constexpr size_t HMAC_KEY_SIZE = 32;
constexpr size_t HMAC_DIGEST_SIZE = 32;

using AESKey = std::array<unsigned char, AES_KEY_SIZE>;
using AESIv = std::array<unsigned char, AES_IV_SIZE>;
using HMACKey = std::array<unsigned char, HMAC_KEY_SIZE>;

struct EncryptedMessage {
    std::vector<unsigned char> ciphertext;
    AESIv iv;
    std::vector<unsigned char> hmac;
    uint64_t timestamp;
    uint64_t sequence_number;
};

class EncryptionManager {
public:
    EncryptionManager();
    ~EncryptionManager();

    // Non-copyable, non-movable
    EncryptionManager(const EncryptionManager&) = delete;
    EncryptionManager& operator=(const EncryptionManager&) = delete;
    EncryptionManager(EncryptionManager&&) = delete;
    EncryptionManager& operator=(EncryptionManager&&) = delete;

    bool initialize();

    // Key management
    bool generateEphemeralKeys();
    bool exchangeKeys(const std::string& peer_public_key);
    std::string getPublicKey() const;

    // Encryption/Decryption
    std::unique_ptr<EncryptedMessage> encrypt(const std::string& plaintext);
    std::string decrypt(const EncryptedMessage& encrypted_msg);

    // HMAC operations
    std::vector<unsigned char> computeHMAC(const std::vector<unsigned char>& data) const;
    bool verifyHMAC(const std::vector<unsigned char>& data, 
                   const std::vector<unsigned char>& hmac) const;

    // Perfect Forward Secrecy
    void rotateKeys();
    bool deriveSessionKeys(const std::vector<unsigned char>& shared_secret);

    // Utility functions
    static std::vector<unsigned char> generateRandomBytes(size_t length);
    static std::string bytesToHex(const std::vector<unsigned char>& bytes);
    static std::vector<unsigned char> hexToBytes(const std::string& hex);

private:
    bool initializeRSA();
    bool initializeAES();
    bool initializeHMAC();
    
    std::vector<unsigned char> rsaEncrypt(const std::vector<unsigned char>& data) const;
    std::vector<unsigned char> rsaDecrypt(const std::vector<unsigned char>& data) const;
    
    std::vector<unsigned char> aesEncrypt(const std::vector<unsigned char>& plaintext, 
                                        const AESIv& iv) const;
    std::vector<unsigned char> aesDecrypt(const std::vector<unsigned char>& ciphertext, 
                                        const AESIv& iv) const;

    // OpenSSL contexts
    EVP_PKEY* rsa_keypair_;
    EVP_PKEY* peer_public_key_;
    
    // Session keys
    AESKey session_key_;
    HMACKey hmac_key_;
    
    // Sequence numbers for replay protection
    std::atomic<uint64_t> send_sequence_{0};
    std::atomic<uint64_t> expected_receive_sequence_{0};
    
    // Key rotation
    std::chrono::steady_clock::time_point last_key_rotation_;
    static constexpr std::chrono::minutes KEY_ROTATION_INTERVAL{30};
    
    // Thread safety
    mutable std::mutex crypto_mutex_;
    
    // Initialization state
    bool initialized_{false};
};

} // namespace securechat::crypto