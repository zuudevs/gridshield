/**
 * @file platform.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"

namespace gridshield::platform {

class IPlatformTime {
public:
    virtual ~IPlatformTime() = default;
    virtual core::timestamp_t get_timestamp_ms() noexcept = 0;
    virtual void delay_ms(uint32_t ms) noexcept = 0;
};

class IPlatformGPIO {
public:
    virtual ~IPlatformGPIO() = default;
    
    enum class PinMode : uint8_t {
        Input = 0,
        Output = 1,
        InputPullup = 2,
        InputPulldown = 3
    };
    
    virtual core::Result<void> configure(uint8_t pin, PinMode mode) noexcept = 0;
    virtual core::Result<bool> read(uint8_t pin) noexcept = 0;
    virtual core::Result<void> write(uint8_t pin, bool value) noexcept = 0;
};

class IPlatformInterrupt {
public:
    virtual ~IPlatformInterrupt() = default;
    
    enum class TriggerMode : uint8_t {
        Rising = 0,
        Falling = 1,
        Change = 2,
        Low = 3,
        High = 4
    };
    
    using InterruptCallback = void (*)(void* context);
    
    virtual core::Result<void> attach(uint8_t pin, TriggerMode mode, 
                                     InterruptCallback callback, 
                                     void* context) noexcept = 0;
    virtual core::Result<void> detach(uint8_t pin) noexcept = 0;
    virtual core::Result<void> enable(uint8_t pin) noexcept = 0;
    virtual core::Result<void> disable(uint8_t pin) noexcept = 0;
};

class IPlatformCrypto {
public:
    virtual ~IPlatformCrypto() = default;
    
    virtual core::Result<void> random_bytes(uint8_t* buffer, size_t length) noexcept = 0;
    virtual core::Result<uint32_t> crc32(const uint8_t* data, size_t length) noexcept = 0;
    virtual core::Result<void> sha256(const uint8_t* data, size_t length, 
                                     uint8_t* hash_out) noexcept = 0;
};

class IPlatformStorage {
public:
    virtual ~IPlatformStorage() = default;
    
    virtual core::Result<size_t> read(uint32_t address, uint8_t* buffer, 
                                     size_t length) noexcept = 0;
    virtual core::Result<size_t> write(uint32_t address, const uint8_t* data, 
                                      size_t length) noexcept = 0;
    virtual core::Result<void> erase(uint32_t address, size_t length) noexcept = 0;
};

class IPlatformComm {
public:
    virtual ~IPlatformComm() = default;
    
    virtual core::Result<void> init() noexcept = 0;
    virtual core::Result<void> shutdown() noexcept = 0;
    virtual core::Result<size_t> send(const uint8_t* data, size_t length) noexcept = 0;
    virtual core::Result<size_t> receive(uint8_t* buffer, size_t max_length, 
                                        uint32_t timeout_ms) noexcept = 0;
    virtual bool is_connected() noexcept = 0;
};

struct PlatformServices {
    IPlatformTime* time;
    IPlatformGPIO* gpio;
    IPlatformInterrupt* interrupt;
    IPlatformCrypto* crypto;
    IPlatformStorage* storage;
    IPlatformComm* comm;
    
    constexpr PlatformServices() noexcept 
        : time(nullptr), gpio(nullptr), interrupt(nullptr),
          crypto(nullptr), storage(nullptr), comm(nullptr) {}
    
    constexpr bool is_valid() const noexcept {
        return time != nullptr && gpio != nullptr && 
               interrupt != nullptr && crypto != nullptr;
    }
};

} // namespace gridshield::platform