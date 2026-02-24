/**
 * @file fuzz_packet_parse.cpp
 * @brief LibFuzzer harness for SecurePacket::parse
 *
 * Feeds random bytes into the packet parser to find crashes,
 * buffer overflows, and undefined behavior in malformed packet handling.
 *
 * Build:  cmake -B build -S . && cmake --build build
 * Run:    ./build/fuzz_packet_parse -max_len=2048 -runs=1000000
 */

#include "network/packet.hpp"
#include "platform/mock_platform.hpp"
#include "security/crypto.hpp"

#include <cstdint>
#include <cstdlib>

using namespace gridshield;

// Persistent state across fuzzer iterations (initialized once)
static platform::mock::MockCrypto g_mock_crypto;
static security::CryptoEngine* g_crypto = nullptr;
static security::ECCKeyPair g_keypair;

// One-time initialization
static void fuzz_init()
{
    static bool initialized = false;
    if (initialized)
        return;

    g_crypto = new security::CryptoEngine(g_mock_crypto);
    auto result = g_crypto->generate_keypair(g_keypair);
    if (result.is_error()) {
        std::abort(); // Cannot proceed without a valid keypair
    }

    initialized = true;
}

// LibFuzzer entry point
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    fuzz_init();

    // Feed arbitrary bytes into the packet parser
    // This exercises all validation paths: magic checks, length checks,
    // checksum verification, and signature verification.
    network::SecurePacket packet;
    auto result = packet.parse(data, size, *g_crypto, g_keypair);

    // We don't care about the result — we only care that it doesn't crash
    (void)result;

    return 0;
}
