/**
 * @file gs_macros.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Portable macros for embedded systems
 * @version 0.1
 * @date 2026-02-07
 * 
 * @copyright Copyright (c) 2026
 */

#pragma once

// Platform detection
#if defined(ARDUINO) || defined(__AVR__)
    #define PLATFORM_AVR 1
    #define PLATFORM_NATIVE 0
#else
    #define PLATFORM_AVR 0
    #define PLATFORM_NATIVE 1
#endif

// Move semantics
#if PLATFORM_NATIVE
    #include <utility>
    #define ZMOVE(x) std::move(x)
#else
    // Manual move for AVR (no std::move available)
    template<typename T>
    struct remove_reference_gs { using type = T; };
    
    template<typename T>
    struct remove_reference_gs<T&> { using type = T; };
    
    template<typename T>
    struct remove_reference_gs<T&&> { using type = T; };
    
    template<typename T>
    typename remove_reference_gs<T>::type&& move_gs(T&& arg) {
        return static_cast<typename remove_reference_gs<T>::type&&>(arg);
    }
    
    #define ZMOVE(x) move_gs(x)
#endif

// Constexpr compatibility
#if __cplusplus >= 201402L
    #define CONSTEXPR14 constexpr
#else
    #define CONSTEXPR14 inline
#endif

// No-discard warnings
#if __cplusplus >= 201703L
    #define NODISCARD [[nodiscard]]
#else
    #define NODISCARD
#endif

// Likely/unlikely hints
#if defined(__GNUC__) || defined(__clang__)
    #define LIKELY(x) __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define LIKELY(x) (x)
    #define UNLIKELY(x) (x)
#endif