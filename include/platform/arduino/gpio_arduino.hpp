/**
 * @file gpio_arduino.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Arduino GPIO driver implementation
 * @version 0.1
 * @date 2026-02-07
 * 
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "platform/platform.hpp"

#if PLATFORM_AVR
#include <Arduino.h>

namespace gridshield::platform::arduino {

class ArduinoGPIO : public IPlatformGPIO {
public:
    ArduinoGPIO() = default;
    
    core::Result<void> configure(uint8_t pin, PinMode mode) noexcept override {
        switch (mode) {
            case PinMode::Input:
                pinMode(pin, INPUT);
                break;
            case PinMode::Output:
                pinMode(pin, OUTPUT);
                break;
            case PinMode::InputPullup:
                pinMode(pin, INPUT_PULLUP);
                break;
            case PinMode::InputPulldown:
                // AVR doesn't have internal pulldown - use external resistor
                pinMode(pin, INPUT);
                break;
        }
        return core::Result<void>();
    }
    
    core::Result<bool> read(uint8_t pin) noexcept override {
        return core::Result<bool>(digitalRead(pin) == HIGH);
    }
    
    core::Result<void> write(uint8_t pin, bool value) noexcept override {
        digitalWrite(pin, value ? HIGH : LOW);
        return core::Result<void>();
    }
};

} // namespace gridshield::platform::arduino

#endif // PLATFORM_AVR