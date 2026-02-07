/**
 * @file gs_typetraits.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Brief description
 * @version 0.1
 * @date 2026-02-07
 * 
 * @copyright Copyright (c) 2026
 */

#pragma once

#if	defined(__AVR__) || defined(__ARDUINO_ARCH_AVR__)
	template <typename T>
	struct remove_reference {
		using type = T;
	};

	template <typename T>
	struct remove_reference<T&> {
		using type = T;
	};

	template <typename T>
	struct remove_reference<T&&> {
		using type = T;
	};
#endif 