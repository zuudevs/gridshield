#pragma once

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define PROGMEM
#define PGM_P const char*
#define PSTR(x) (x)
#define F(x) (x)

#define pgm_read_byte(p) (*(const uint8_t*)(p))
#define pgm_read_word(p) (*(const uint16_t*)(p))
#define pgm_read_ptr(p) (*(const void* const*)(p))
#define memcpy_P memcpy
#define strcpy_P strcpy
#define strlen_P strlen
#define strncmp_P strncmp
#define vsnprintf_P vsnprintf

// Enable Crypto library features
#define CRYPTO_AES_DEFAULT 1
#define CRYPTO_SHA256_DEFAULT 1

// Mock Print class if needed
class Print {
public:
    virtual size_t write(uint8_t) { return 1; }
    virtual size_t write(const uint8_t *buffer, size_t size) { return size; }
};

// Mock Stream
class Stream : public Print {
public:
    virtual int available() { return 0; }
    virtual int read() { return -1; }
    virtual int peek() { return -1; }
    virtual void flush() {}
};

extern "C" {
    inline unsigned long millis() { return 0; }
    inline unsigned long micros() { return 0; }
    inline void delay(unsigned long) {}
    inline void yield() {}
}
