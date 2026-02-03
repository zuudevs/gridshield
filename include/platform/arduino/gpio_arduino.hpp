/**
 * @file gpio_arduino.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once

#include <Arduino.h>
#include "platform/platform.hpp"

namespace gridshield::platform::arduino {

class ArduinoGPIO : public IPlatformGPIO {
public:
    ArduinoGPIO() = default;
    ~ArduinoGPIO() override = default;

    core::Result<void> configure(uint8_t pin, PinMode mode) noexcept override {
        uint8_t arduinoMode = INPUT;
        switch (mode) {
            case PinMode::Output: arduinoMode = OUTPUT; break;
            case PinMode::InputPullup: arduinoMode = INPUT_PULLUP; break;
            case PinMode::InputPulldown: arduinoMode = INPUT_PULLDOWN; break; // ESP32 supports this
            default: arduinoMode = INPUT; break;
        }
        
        pinMode(pin, arduinoMode);
        return core::Result<void>();
    }

    core::Result<bool> read(uint8_t pin) noexcept override {
        int val = digitalRead(pin);
        return core::Result<bool>(val == HIGH);
    }

    core::Result<void> write(uint8_t pin, bool value) noexcept override {
        digitalWrite(pin, value ? HIGH : LOW);
        return core::Result<void>();
    }
};

} // namespace gridshield::platform::arduino