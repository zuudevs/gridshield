/**
 * @file platform.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Virtual destructor implementations (C++17)
 * @version 0.3
 * @date 2026-02-09
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