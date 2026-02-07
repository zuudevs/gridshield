/**
 * @file platform.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Virtual destructor implementations for platform interfaces
 * @version 0.2
 * @date 2026-02-08
 * 
 * @copyright Copyright (c) 2026
 */

#include "platform/platform.hpp"

namespace gridshield {
namespace platform {

// Virtual destructors must have implementation even if pure virtual
IPlatformTime::~IPlatformTime() = default;
IPlatformGPIO::~IPlatformGPIO() = default;
IPlatformInterrupt::~IPlatformInterrupt() = default;
IPlatformCrypto::~IPlatformCrypto() = default;
IPlatformStorage::~IPlatformStorage() = default;
IPlatformComm::~IPlatformComm() = default;

} // namespace platform
} // namespace gridshield