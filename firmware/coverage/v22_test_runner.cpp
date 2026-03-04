/**
 * @file v22_test_runner.cpp
 * @brief Standalone test runner for v2.2.0 test suites only.
 *
 * Avoids mbedtls dependency from existing production sources.
 */

#include "unity.h"
#include <cstdio>

int unity_test_count = 0;
int unity_test_failures = 0;
int unity_current_failed = 0;

extern "C" void test_mqtt_suite(void);
extern "C" void test_sensors_suite(void);
extern "C" void test_ota_power_suite(void);

int main()
{
    printf("\n");
    printf("==============================================\n");
    printf(" GridShield v2.2.0 Unit Tests (Native Build)\n");
    printf("==============================================\n\n");

    UNITY_BEGIN();

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
