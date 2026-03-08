/**
 * @file esp_log.h
 * @brief ESP-IDF esp_log.h shim for native builds
 *
 * Provides no-op or printf-based implementations of ESP_LOGx macros
 * so production code compiles natively for fuzzing and coverage.
 */

#pragma once

#include <cstdio>

// ESP-IDF log levels (simplified)
typedef enum
{
    ESP_LOG_NONE,
    ESP_LOG_ERROR,
    ESP_LOG_WARN,
    ESP_LOG_INFO,
    ESP_LOG_DEBUG,
    ESP_LOG_VERBOSE
} esp_log_level_t;

// Map ESP_LOGx to printf for native builds
#define ESP_LOGE(tag, fmt, ...) fprintf(stderr, "E (%s) " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGW(tag, fmt, ...) fprintf(stderr, "W (%s) " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGI(tag, fmt, ...) printf("I (%s) " fmt "\n", tag, ##__VA_ARGS__)
#define ESP_LOGD(tag, fmt, ...) (void)0
#define ESP_LOGV(tag, fmt, ...) (void)0
