/**
 * @file v30_test_runner.cpp
 * @brief Standalone test runner for v3.0.0 test suites
 */

#include "unity.h"
#include <cstdio>

int unity_test_count = 0;
int unity_test_failures = 0;
int unity_current_failed = 0;

extern "C" void test_protocols_suite(void);
extern "C" void test_cloud_suite(void);
extern "C" void test_performance_suite(void);
extern "C" void test_analytics_suite(void);

int main()
{
    printf("\n");
    printf("==============================================\n");
    printf(" GridShield v3.0.0 Unit Tests (Native Build)\n");
    printf("==============================================\n\n");

    UNITY_BEGIN();

    test_protocols_suite();
    test_cloud_suite();
    test_performance_suite();
    test_analytics_suite();

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
