/**
 * @file endian.h
 * @brief Stub for machine/endian.h (missing from clang-tidy environment)
 *
 * ESP-IDF's newlib <endian.h> includes <machine/endian.h>, which exists
 * in the Xtensa toolchain's sysroot but not in clang's standard headers.
 * This stub satisfies the #include for clang-tidy analysis.
 */

#pragma once

#ifndef _MACHINE_ENDIAN_H
#define _MACHINE_ENDIAN_H

#ifndef __BYTE_ORDER__
#define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
#endif

#ifndef BYTE_ORDER
#define LITTLE_ENDIAN 1234
#define BIG_ENDIAN 4321
#define BYTE_ORDER LITTLE_ENDIAN
#endif

#endif /* _MACHINE_ENDIAN_H */
