/**
 * @file gs_macros.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Platform-agnostic macros for C++17
 * @version 0.4
 * @date 2026-02-07
 * 
 * @copyright Copyright (c) 2026
 */

#pragma once

// ============================================================================
// PLATFORM DETECTION
// ============================================================================
#if defined(ARDUINO) || defined(__AVR__)
    #define GS_PLATFORM_ARDUINO 1
    #define GS_PLATFORM_NATIVE 0
#else
    #define GS_PLATFORM_ARDUINO 0
    #define GS_PLATFORM_NATIVE 1
#endif

// ============================================================================
// COMPILER DETECTION
// ============================================================================
#if defined(__clang__)
    #define GS_COMPILER_CLANG 1
    #define GS_COMPILER_GCC 0
    #define GS_COMPILER_MSVC 0
#elif defined(__GNUC__)
    #define GS_COMPILER_CLANG 0
    #define GS_COMPILER_GCC 1
    #define GS_COMPILER_MSVC 0
#elif defined(_MSC_VER)
    #define GS_COMPILER_CLANG 0
    #define GS_COMPILER_GCC 0
    #define GS_COMPILER_MSVC 1
#else
    #define GS_COMPILER_CLANG 0
    #define GS_COMPILER_GCC 0
    #define GS_COMPILER_MSVC 0
#endif

// ============================================================================
// MOVE SEMANTICS (C++17 compatible)
// ============================================================================
#if GS_PLATFORM_NATIVE
    #include <utility>
    #define GS_MOVE(x) ::std::move(x)
    #define GS_FORWARD(T, x) ::std::forward<T>(x)
#else
    // Manual move for AVR (C++17 compliant)
    namespace gridshield {
    namespace detail {
        template<typename T>
        struct remove_reference { using type = T; };
        
        template<typename T>
        struct remove_reference<T&> { using type = T; };
        
        template<typename T>
        struct remove_reference<T&&> { using type = T; };
        
        template<typename T>
        inline typename remove_reference<T>::type&& 
        move_impl(T&& arg) noexcept {
            return static_cast<typename remove_reference<T>::type&&>(arg);
        }
        
        template<typename T>
        inline T&& forward_impl(typename remove_reference<T>::type& arg) noexcept {
            return static_cast<T&&>(arg);
        }
        
        template<typename T>
        inline T&& forward_impl(typename remove_reference<T>::type&& arg) noexcept {
            return static_cast<T&&>(arg);
        }
    } // namespace detail
    } // namespace gridshield
    
    #define GS_MOVE(x) ::gridshield::detail::move_impl(x)
    #define GS_FORWARD(T, x) ::gridshield::detail::forward_impl<T>(x)
#endif

// ============================================================================
// COMPILER HINTS
// ============================================================================
#if GS_COMPILER_GCC || GS_COMPILER_CLANG
    #define GS_LIKELY(x)   __builtin_expect(!!(x), 1)
    #define GS_UNLIKELY(x) __builtin_expect(!!(x), 0)
    #define GS_INLINE      inline __attribute__((always_inline))
    #define GS_NOINLINE    __attribute__((noinline))
    #define GS_PACKED      __attribute__((packed))
    #define GS_ALIGN(n)    __attribute__((aligned(n)))
#elif GS_COMPILER_MSVC
    #define GS_LIKELY(x)   (x)
    #define GS_UNLIKELY(x) (x)
    #define GS_INLINE      __forceinline
    #define GS_NOINLINE    __declspec(noinline)
    #define GS_PACKED
    #define GS_ALIGN(n)    __declspec(align(n))
#else
    #define GS_LIKELY(x)   (x)
    #define GS_UNLIKELY(x) (x)
    #define GS_INLINE      inline
    #define GS_NOINLINE
    #define GS_PACKED
    #define GS_ALIGN(n)
#endif

// ============================================================================
// ATTRIBUTES (C++17)
// ============================================================================
#if __cplusplus >= 201703L
    #define GS_NODISCARD [[nodiscard]]
    #define GS_FALLTHROUGH [[fallthrough]]
    #define GS_MAYBE_UNUSED [[maybe_unused]]
#else
    #define GS_NODISCARD
    #define GS_FALLTHROUGH
    #define GS_MAYBE_UNUSED
#endif

// ============================================================================
// CONSTEXPR
// ============================================================================
#define GS_CONSTEXPR constexpr

// ============================================================================
// SECTION PLACEMENT (Arduino only)
// ============================================================================
#if GS_PLATFORM_ARDUINO
    #define GS_PROGMEM   __attribute__((section(".progmem.data")))
    #define GS_NOINIT    __attribute__((section(".noinit")))
#else
    #define GS_PROGMEM
    #define GS_NOINIT
#endif

// ============================================================================
// ASSERTIONS
// ============================================================================
#if GS_PLATFORM_NATIVE
    #include <cassert>
    #define GS_ASSERT(expr) assert(expr)
#else
    #define GS_ASSERT(expr) ((void)0)
#endif

// ============================================================================
// STATIC ASSERT
// ============================================================================
#define GS_STATIC_ASSERT(expr, msg) static_assert(expr, msg)