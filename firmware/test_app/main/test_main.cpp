/**
 * @file test_main.cpp
 * @brief GridShield Unit Test Entry Point (ESP-IDF + Unity)
 *
 * Runs all test suites on QEMU via: idf.py qemu monitor
 */

#include <cstdio>
#include "unity.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Test suite declarations (defined in individual test_*.cpp files)
extern void test_result_suite(void);
extern void test_static_buffer_suite(void);
extern void test_byte_array_suite(void);
extern void test_anomaly_detector_suite(void);
extern void test_secure_packet_suite(void);

extern "C" void app_main(void) {
    // Small delay for QEMU UART stabilization
    vTaskDelay(pdMS_TO_TICKS(200));

    printf("\n");
    printf("==============================================\n");
    printf(" GridShield Unit Tests\n");
    printf(" Platform: ESP-IDF + QEMU\n");
    printf("==============================================\n\n");

    UNITY_BEGIN();

    // Run all test suites
    test_result_suite();
    test_static_buffer_suite();
    test_byte_array_suite();
    test_anomaly_detector_suite();
    test_secure_packet_suite();

    int failures = UNITY_END();

    printf("\n==============================================\n");
    if (failures == 0) {
        printf(" ALL TESTS PASSED\n");
    } else {
        printf(" %d TEST(S) FAILED\n", failures);
    }
    printf("==============================================\n\n");
}
