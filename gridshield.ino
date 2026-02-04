#include <Arduino.h>
#include "core/system.hpp"

// forward declaration dari main.cpp
void gridshield_setup();
void gridshield_loop();

void setup() {
    gridshield_setup();
}

void loop() {
    gridshield_loop();
}