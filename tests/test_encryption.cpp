#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "crypto/encryption_manager.hpp"

using namespace securechat::crypto;

class EncryptionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        encryption_manager_ = std::make_unique<EncryptionManager>();
        ASSERT_TRUE(encryption_manager_->initialize());
    }

    void TearDown() override {
        encryption_manager_.reset();
    }

    std::unique_ptr<EncryptionManager> encryption_manager_;
};

TEST_F(EncryptionManagerTest, InitializationSuccess) {
    EXPECT_TRUE(encryption_manager_->initialize());
}

TEST_F(EncryptionManagerTest, KeyGeneration) {
    EXPECT_TRUE(encryption_manager_->generateEphemeralKeys());
    
    std::string public_key = encryption_manager_->getPublicKey();
    EXPECT_FALSE(public_key.empty());
    EXPECT_GT(public_key.length(), 100); // RSA-2048 public key should be substantial
}

TEST_F(EncryptionManagerTest, EncryptDecryptRoundTrip) {
    ASSERT_TRUE(encryption_manager_->generateEphemeralKeys());
    
    std::string plaintext = "Hello, SecureChat! This is a test message.";
    
    auto encrypted = encryption_manager_->encrypt(plaintext);
    ASSERT_NE(encrypted, nullptr);
    EXPECT_FALSE(encrypted->ciphertext.empty());
    EXPECT_GT(encrypted->timestamp, 0);
    
    std::string decrypted = encryption_manager_->decrypt(*encrypted);
    EXPECT_EQ(plaintext, decrypted);
}

TEST_F(EncryptionManagerTest, EncryptionProducesUniqueResults) {
    ASSERT_TRUE(encryption_manager_->generateEphemeralKeys());
    
    std::string plaintext = "Test message for uniqueness";
    
    auto encrypted1 = encryption_manager_->encrypt(plaintext);
    auto encrypted2 = encryption_manager_->encrypt(plaintext);
    
    ASSERT_NE(encrypted1, nullptr);
    ASSERT_NE(encrypted2, nullptr);
    
    // Same plaintext should produce different ciphertext due to random IV
    EXPECT_NE(encrypted1->ciphertext, encrypted2->ciphertext);
    EXPECT_NE(encrypted1->iv, encrypted2->iv);
}

TEST_F(EncryptionManagerTest, HMACValidation) {
    ASSERT_TRUE(encryption_manager_->generateEphemeralKeys());
    
    std::vector<unsigned char> data = {'t', 'e', 's', 't', ' ', 'd', 'a', 't', 'a'};
    
    auto hmac = encryption_manager_->computeHMAC(data);
    EXPECT_FALSE(hmac.empty());
    EXPECT_EQ(hmac.size(), HMAC_DIGEST_SIZE);
    
    EXPECT_TRUE(encryption_manager_->verifyHMAC(data, hmac));
    
    // Modify data and verify HMAC fails
    data[0] = 'T';
    EXPECT_FALSE(encryption_manager_->verifyHMAC(data, hmac));
}

TEST_F(EncryptionManagerTest, LargeMessageEncryption) {
    ASSERT_TRUE(encryption_manager_->generateEphemeralKeys());
    
    // Create a large message (1MB)
    std::string large_message(1024 * 1024, 'A');
    
    auto start = std::chrono::high_resolution_clock::now();
    auto encrypted = encryption_manager_->encrypt(large_message);
    auto encrypt_end = std::chrono::high_resolution_clock::now();
    
    ASSERT_NE(encrypted, nullptr);
    
    std::string decrypted = encryption_manager_->decrypt(*encrypted);
    auto decrypt_end = std::chrono::high_resolution_clock::now();
    
    EXPECT_EQ(large_message, decrypted);
    
    auto encrypt_time = std::chrono::duration_cast<std::chrono::milliseconds>(encrypt_end - start);
    auto decrypt_time = std::chrono::duration_cast<std::chrono::milliseconds>(decrypt_end - encrypt_end);
    
    // Performance expectations (adjust based on your requirements)
    EXPECT_LT(encrypt_time.count(), 100); // Less than 100ms for 1MB
    EXPECT_LT(decrypt_time.count(), 100);
    
    std::cout << "Encryption time for 1MB: " << encrypt_time.count() << "ms" << std::endl;
    std::cout << "Decryption time for 1MB: " << decrypt_time.count() << "ms" << std::endl;
}

TEST_F(EncryptionManagerTest, KeyRotation) {
    ASSERT_TRUE(encryption_manager_->generateEphemeralKeys());
    
    std::string public_key_before = encryption_manager_->getPublicKey();
    
    encryption_manager_->rotateKeys();
    
    std::string public_key_after = encryption_manager_->getPublicKey();
    
    // Keys should be different after rotation
    EXPECT_NE(public_key_before, public_key_after);
}

TEST_F(EncryptionManagerTest, SequenceNumberIncrement) {
    ASSERT_TRUE(encryption_manager_->generateEphemeralKeys());
    
    std::string message = "Test message";
    
    auto encrypted1 = encryption_manager_->encrypt(message);
    auto encrypted2 = encryption_manager_->encrypt(message);
    
    ASSERT_NE(encrypted1, nullptr);
    ASSERT_NE(encrypted2, nullptr);
    
    // Sequence numbers should increment
    EXPECT_EQ(encrypted2->sequence_number, encrypted1->sequence_number + 1);
}

TEST_F(EncryptionManagerTest, UtilityFunctions) {
    // Test random byte generation
    auto random_bytes = EncryptionManager::generateRandomBytes(32);
    EXPECT_EQ(random_bytes.size(), 32);
    
    auto random_bytes2 = EncryptionManager::generateRandomBytes(32);
    EXPECT_NE(random_bytes, random_bytes2); // Should be different
    
    // Test hex conversion
    std::vector<unsigned char> test_data = {0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF};
    std::string hex = EncryptionManager::bytesToHex(test_data);
    EXPECT_EQ(hex, "0123456789ABCDEF");
    
    auto converted_back = EncryptionManager::hexToBytes(hex);
    EXPECT_EQ(test_data, converted_back);
}

// Performance benchmark test
TEST_F(EncryptionManagerTest, PerformanceBenchmark) {
    ASSERT_TRUE(encryption_manager_->generateEphemeralKeys());
    
    const int num_operations = 1000;
    const std::string test_message = "This is a test message for performance benchmarking.";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_operations; ++i) {
        auto encrypted = encryption_manager_->encrypt(test_message);
        ASSERT_NE(encrypted, nullptr);
        
        std::string decrypted = encryption_manager_->decrypt(*encrypted);
        EXPECT_EQ(test_message, decrypted);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    double ops_per_second = (num_operations * 2.0 * 1000000.0) / duration.count(); // *2 for encrypt+decrypt
    
    std::cout << "Performance: " << ops_per_second << " operations/second" << std::endl;
    std::cout << "Average time per encrypt+decrypt: " << duration.count() / (num_operations * 2.0) << " microseconds" << std::endl;
    
    // Performance expectation: should handle at least 1000 encrypt+decrypt operations per second
    EXPECT_GT(ops_per_second, 1000.0);
}

// Thread safety test
TEST_F(EncryptionManagerTest, ThreadSafety) {
    ASSERT_TRUE(encryption_manager_->generateEphemeralKeys());
    
    const int num_threads = 10;
    const int operations_per_thread = 100;
    std::vector<std::thread> threads;
    std::atomic<int> successful_operations{0};
    
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([this, &successful_operations, operations_per_thread]() {
            for (int i = 0; i < operations_per_thread; ++i) {
                std::string message = "Thread test message " + std::to_string(i);
                
                auto encrypted = encryption_manager_->encrypt(message);
                if (encrypted) {
                    std::string decrypted = encryption_manager_->decrypt(*encrypted);
                    if (decrypted == message) {
                        successful_operations.fetch_add(1);
                    }
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    EXPECT_EQ(successful_operations.load(), num_threads * operations_per_thread);
}