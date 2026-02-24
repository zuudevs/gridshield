/**
 * @file unity.h
 * @brief Unity test framework shim for native coverage builds
 *
 * Provides a minimal Unity-compatible API for running tests natively
 * (outside ESP-IDF). This allows reusing the existing test_*.cpp files
 * without modifications.
 *
 * NOTE: Counters use `extern` linkage so all translation units share
 * the same counters. They are defined in coverage_main.cpp.
 */

#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstring>

// ============================================================================
// Shared counters (defined in coverage_main.cpp)
// ============================================================================
extern int unity_test_count;
extern int unity_test_failures;
extern int unity_current_failed;

// ============================================================================
// Unity-compatible functions
// ============================================================================
inline void UnityBegin(const char* /*file*/)
{
    unity_test_count = 0;
    unity_test_failures = 0;
}

inline int UnityEnd()
{
    printf("\n-----------------------\n");
    printf("%d Tests %d Failures\n", unity_test_count, unity_test_failures);
    if (unity_test_failures == 0) {
        printf("OK\n");
    } else {
        printf("FAIL\n");
    }
    return unity_test_failures;
}

#define UNITY_BEGIN() UnityBegin(__FILE__)
#define UNITY_END() UnityEnd()

#define RUN_TEST(func)                                                                             \
    do {                                                                                           \
        unity_current_failed = 0;                                                                  \
        unity_test_count++;                                                                        \
        printf("  TEST: %s ... ", #func);                                                          \
        func();                                                                                    \
        if (unity_current_failed == 0) {                                                           \
            printf("PASS\n");                                                                      \
        }                                                                                          \
    } while (0)

// ============================================================================
// Assertion macros
// ============================================================================

#define UNITY_FAIL(msg)                                                                            \
    do {                                                                                           \
        if (unity_current_failed == 0) {                                                           \
            unity_test_failures++;                                                                 \
            unity_current_failed = 1;                                                              \
        }                                                                                          \
        printf("FAIL\n    %s:%d: %s\n", __FILE__, __LINE__, msg);                                  \
    } while (0)

#define TEST_ASSERT(cond)                                                                          \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            UNITY_FAIL("TEST_ASSERT(" #cond ")");                                                  \
        }                                                                                          \
    } while (0)

#define TEST_ASSERT_TRUE(cond)                                                                     \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            UNITY_FAIL("Expected TRUE: " #cond);                                                   \
        }                                                                                          \
    } while (0)

#define TEST_ASSERT_FALSE(cond)                                                                    \
    do {                                                                                           \
        if ((cond)) {                                                                              \
            UNITY_FAIL("Expected FALSE: " #cond);                                                  \
        }                                                                                          \
    } while (0)

#define TEST_ASSERT_NULL(ptr)                                                                      \
    do {                                                                                           \
        if ((ptr) != nullptr) {                                                                    \
            UNITY_FAIL("Expected NULL: " #ptr);                                                    \
        }                                                                                          \
    } while (0)

#define TEST_ASSERT_NOT_NULL(ptr)                                                                  \
    do {                                                                                           \
        if ((ptr) == nullptr) {                                                                    \
            UNITY_FAIL("Expected NOT NULL: " #ptr);                                                \
        }                                                                                          \
    } while (0)

#define TEST_ASSERT_EQUAL(expected, actual)                                                        \
    do {                                                                                           \
        auto _e = (expected);                                                                      \
        auto _a = (actual);                                                                        \
        if (_e != _a) {                                                                            \
            char _buf[128];                                                                        \
            snprintf(_buf, sizeof(_buf), "Expected %lld, got %lld", (long long)_e, (long long)_a); \
            UNITY_FAIL(_buf);                                                                      \
        }                                                                                          \
    } while (0)

#define TEST_ASSERT_NOT_EQUAL(expected, actual)                                                    \
    do {                                                                                           \
        if ((expected) == (actual)) {                                                              \
            UNITY_FAIL("Expected NOT " #expected);                                                 \
        }                                                                                          \
    } while (0)

#define TEST_ASSERT_EQUAL_UINT8(expected, actual)                                                  \
    TEST_ASSERT_EQUAL((uint8_t)(expected), (uint8_t)(actual))

#define TEST_ASSERT_EQUAL_UINT16(expected, actual)                                                 \
    TEST_ASSERT_EQUAL((uint16_t)(expected), (uint16_t)(actual))

#define TEST_ASSERT_EQUAL_UINT32(expected, actual)                                                 \
    TEST_ASSERT_EQUAL((uint32_t)(expected), (uint32_t)(actual))

#define TEST_ASSERT_EQUAL_INT(expected, actual) TEST_ASSERT_EQUAL((int)(expected), (int)(actual))

#define TEST_ASSERT_GREATER_THAN(threshold, actual)                                                \
    do {                                                                                           \
        if (!((actual) > (threshold))) {                                                           \
            UNITY_FAIL("Expected > " #threshold);                                                  \
        }                                                                                          \
    } while (0)

#define TEST_ASSERT_LESS_THAN(threshold, actual)                                                   \
    do {                                                                                           \
        if (!((actual) < (threshold))) {                                                           \
            UNITY_FAIL("Expected < " #threshold);                                                  \
        }                                                                                          \
    } while (0)

#define TEST_ASSERT_GREATER_OR_EQUAL(threshold, actual)                                            \
    do {                                                                                           \
        if (!((actual) >= (threshold))) {                                                          \
            UNITY_FAIL("Expected >= " #threshold);                                                 \
        }                                                                                          \
    } while (0)

#define TEST_ASSERT_EQUAL_STRING(expected, actual)                                                 \
    do {                                                                                           \
        if (strcmp((expected), (actual)) != 0) {                                                   \
            char _buf[256];                                                                        \
            snprintf(_buf, sizeof(_buf), "Expected \"%s\", got \"%s\"", (expected), (actual));     \
            UNITY_FAIL(_buf);                                                                      \
        }                                                                                          \
    } while (0)

#define TEST_ASSERT_EQUAL_MEMORY(expected, actual, len)                                            \
    do {                                                                                           \
        if (memcmp((expected), (actual), (len)) != 0) {                                            \
            UNITY_FAIL("Memory blocks differ");                                                    \
        }                                                                                          \
    } while (0)

#define TEST_ASSERT_EQUAL_HEX8_ARRAY(expected, actual, len)                                        \
    TEST_ASSERT_EQUAL_MEMORY(expected, actual, len)

#define TEST_ASSERT_TRUE_MESSAGE(cond, msg)                                                        \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            UNITY_FAIL(msg);                                                                       \
        }                                                                                          \
    } while (0)

#define TEST_ASSERT_FALSE_MESSAGE(cond, msg)                                                       \
    do {                                                                                           \
        if ((cond)) {                                                                              \
            UNITY_FAIL(msg);                                                                       \
        }                                                                                          \
    } while (0)

#define TEST_FAIL_MESSAGE(msg) UNITY_FAIL(msg)
#define TEST_PASS()                                                                                \
    do {                                                                                           \
    } while (0)
#define TEST_IGNORE()                                                                              \
    do {                                                                                           \
        printf("IGNORED\n");                                                                       \
    } while (0)
#define TEST_IGNORE_MESSAGE(msg)                                                                   \
    do {                                                                                           \
        printf("IGNORED: %s\n", msg);                                                              \
    } while (0)
