/**
 * @file gs_utils.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Brief description
 * @version 0.1
 * @date 2026-02-07
 * 
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "gs_typetraits.hpp"

#if	defined(__AVR__) || defined(__ARDUINO_ARCH_AVR__)
	template <typename T>
	typename remove_reference<T>::type&& move(T&& arg) {
		return static_cast<typename remove_reference<T>::type&&>(arg);
	}

	#define MOVE(x) move(x)
#elif defined(__CLANG__) || defined(__GNUC__) || defined(__GNUG__)
	#include <utility>
	
	#define MOVE(x) std::move(x)
#endif 