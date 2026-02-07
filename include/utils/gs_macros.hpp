/**
 * @file gs_macros.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Platform-agnostic macros for C++17 embedded systems
 * @version 0.3
 * @date 2026-02-07
 * 
 * @copyright Copyright (c) 2026
 */

#pragma once

// ============================================================================
// PLATFORM DETECTION
// ============================================================================
#if defined(ARDUINO) || defined(__AVR__)
    #define PLATFORM_AVR 1
    #define PLATFORM_NATIVE 0
#else
    #define PLATFORM_AVR 0
    #define PLATFORM_NATIVE 1
#endif

// ============================================================================
// MOVE SEMANTICS (C++17 compatible)
// ============================================================================
#if PLATFORM_NATIVE
    #include <utility>
    #define ZMOVE(x) ::std::move(x)
#else
    // Manual move implementation for AVR
    namespace gridshield {
    namespace detail {
        template<typename T>
        struct remove_reference { using type = T; };
        
        template<typename T>
        struct remove_reference<T&> { using type = T; };
        
        template<typename T>
        struct remove_reference<T&&> { using type = T; };
        
        template<typename T>
        constexpr typename remove_reference<T>::type&& 
        move_impl(T&& arg) noexcept {
            return static_cast<typename remove_reference<T>::type&&>(arg);
        }
    } // namespace detail
    } // namespace gridshield
    
    #define ZMOVE(x) ::gridshield::detail::move_impl(x)
#endif

// ============================================================================
// COMPILER HINTS
// ============================================================================
#if defined(__GNUC__) || defined(__clang__)
    #define GS_LIKELY(x)   __builtin_expect(!!(x), 1)
    #define GS_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define GS_INLINE      inline __attribute__((always_inline))
    #define GS_NOINLINE    __attribute__((noinline))
#elif defined(_MSC_VER)
    #define GS_LIKELY(x)   (x)
    #define GS_UNLIKELY(x) (x)
    #define GS_INLINE      __forceinline
    #define GS_NOINLINE    __declspec(noinline)
#else
    #define GS_LIKELY(x)   (x)
    #define GS_UNLIKELY(x) (x)
    #define GS_INLINE      inline
    #define GS_NOINLINE
#endif

// Legacy aliases (deprecated, use GS_ prefix)
#define LIKELY(x)   GS_LIKELY(x)
#define UNLIKELY(x) GS_UNLIKELY(x)

// ============================================================================
// CONSTEXPR COMPATIBILITY
// ============================================================================
// C++17 has constexpr by default, no need for CONSTEXPR14 macro
#define CONSTEXPR14 constexpr

// ============================================================================
// NODISCARD
// ============================================================================
#if __cplusplus >= 201703L
    #define NODISCARD [[nodiscard]]
#else
    #define NODISCARD
#endif

// ============================================================================
// MEMORY ALIGNMENT
// ============================================================================
#if defined(__GNUC__) || defined(__clang__)
    #define GS_ALIGN(n) __attribute__((aligned(n)))
#elif defined(_MSC_VER)
    #define GS_ALIGN(n) __declspec(align(n))
#else
    #define GS_ALIGN(n)
#endif

// ============================================================================
// SECTION PLACEMENT (for critical code in flash/RAM)
// ============================================================================
#if PLATFORM_AVR
    #define GS_SECTION_FLASH   __attribute__((section(".progmem.data")))
    #define GS_SECTION_NOINIT  __attribute__((section(".noinit")))
#else
    #define GS_SECTION_FLASH
    #define GS_SECTION_NOINIT
#endif