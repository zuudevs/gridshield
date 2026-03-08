/**
 * @file coverage_main.cpp
 * @brief Native test runner for code coverage collection
 *
 * Replaces the ESP-IDF test_main.cpp (which uses FreeRTOS).
 * Runs all test suites and returns exit code for CI.
 */

#include "unity.h"
#include <cstdio>

// Global Unity counters (declared extern in unity.h)
int unity_test_count = 0;
int unity_test_failures = 0;
int unity_current_failed = 0;

// Test suite declarations (same as test_app/main/test_main.cpp)
extern void test_result_suite(void);
extern void test_static_buffer_suite(void);
extern void test_byte_array_suite(void);
extern void test_anomaly_detector_suite(void);
extern void test_secure_packet_suite(void);
extern void test_hkdf_suite(void);
extern void test_tamper_detector_suite(void);
extern void test_key_storage_suite(void);
extern void test_crypto_engine_suite(void);
extern void test_key_rotation_suite(void);
extern void test_retry_suite(void);
extern void test_system_integration_suite(void);
extern void test_degradation_suite(void);
extern void test_telemetry_suite(void);
extern "C" void test_mqtt_suite(void);
extern "C" void test_sensors_suite(void);
extern "C" void test_ota_power_suite(void);

int main()
{
    printf("\n");
    printf("==============================================\n");
    printf(" GridShield Unit Tests (Native Coverage Build)\n");
    printf("==============================================\n\n");

    UNITY_BEGIN();

    test_result_suite();
    test_static_buffer_suite();
    test_byte_array_suite();
    test_anomaly_detector_suite();
    test_secure_packet_suite();
    test_hkdf_suite();
    test_tamper_detector_suite();
    test_key_storage_suite();
    test_crypto_engine_suite();
    test_key_rotation_suite();
    test_retry_suite();
    test_system_integration_suite();
    test_degradation_suite();
    test_telemetry_suite();
    test_mqtt_suite();
    test_sensors_suite();
    test_ota_power_suite();

    int failures = UNITY_END();

    printf("\n==============================================\n");
    if (failures == 0) {
        printf(" ALL TESTS PASSED\n");
    } else {
        printf(" %d TEST(S) FAILED\n", failures);
    }
    printf("==============================================\n\n");

    return failures;
}
