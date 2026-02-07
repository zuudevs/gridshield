/**
 * @file gridshield.ino
 * @brief Arduino sketch entry point
 * 
 * This file simply bridges Arduino IDE to C++ implementation.
 * All logic is in src/main.cpp
 */

#include <Arduino.h>

// Forward declarations from main.cpp
void gridshield_setup();
void gridshield_loop();

void setup() {
    gridshield_setup();
}

void loop() {
    gridshield_loop();
}