/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file rev.h
 * @brief XOR bit reverse
 * @author Juuso Alasuutari
 */
#ifndef LIBCANTH_SRC_REV_
#define LIBCANTH_SRC_REV_

#include "util.h"

#if defined _MSC_VER || defined __INTELLISENSE__
typedef struct uint2 { unsigned v : 2; } uint2;
typedef struct uint4 { unsigned v : 4; } uint4;
#define uint2(x) (uint2){.v=(x)}
#define uint4(x) (uint4){.v=(x)}
#define uint2_get(x) (x).v
#define uint4_get(x) (x).v
#else
typedef unsigned _BitInt(2) uint2;
typedef unsigned _BitInt(4) uint4;
#define uint2(x) (uint2)(x)
#define uint4(x) (uint4)(x)
#define uint2_get(x) x
#define uint4_get(x) x
#endif

static const_inline uint2
rev2 (uint2 x)
{
	x ^= (x & 0x2) >> 1U;
	x ^= (x & 0x1) << 1U;
	x ^= (x & 0x2) >> 1U;
	return x;
}

static const_inline uint4
rev4 (uint4 x)
{
	x ^= (x & 0xa) >> 1U;
	x ^= (x & 0x5) << 1U;
	x ^= (x & 0xc) >> 2U;
	x ^= (x & 0x3) << 2U;
	x ^= (x & 0xc) >> 2U;
	x ^= (x & 0xa) >> 1U;
	return x;
}

static const_inline uint8_t
rev8 (uint8_t x)
{
	x ^= (x & UINT8_C(0xaa)) >> 1U;
	x ^= (x & UINT8_C(0x55)) << 1U;
	x ^= (x & UINT8_C(0xcc)) >> 2U;
	x ^= (x & UINT8_C(0x33)) << 2U;
	x ^= (x & UINT8_C(0xf0)) >> 4U;
	x ^= (x & UINT8_C(0x0f)) << 4U;
	x ^= (x & UINT8_C(0xf0)) >> 4U;
	x ^= (x & UINT8_C(0xcc)) >> 2U;
	x ^= (x & UINT8_C(0xaa)) >> 1U;
	return x;
}

static const_inline uint16_t
rev16 (uint16_t x)
{
	x ^= (x & UINT16_C(0xaaaa)) >> 1U;
	x ^= (x & UINT16_C(0x5555)) << 1U;
	x ^= (x & UINT16_C(0xcccc)) >> 2U;
	x ^= (x & UINT16_C(0x3333)) << 2U;
	x ^= (x & UINT16_C(0xf0f0)) >> 4U;
	x ^= (x & UINT16_C(0x0f0f)) << 4U;
	x ^= (x & UINT16_C(0xff00)) >> 8U;
	x ^= (x & UINT16_C(0x00ff)) << 8U;
	x ^= (x & UINT16_C(0xff00)) >> 8U;
	x ^= (x & UINT16_C(0xf0f0)) >> 4U;
	x ^= (x & UINT16_C(0xcccc)) >> 2U;
	x ^= (x & UINT16_C(0xaaaa)) >> 1U;
	return x;
}

static const_inline uint32_t
rev32 (uint32_t x)
{
	x ^= (x & UINT32_C(0xaaaaaaaa)) >>  1U;
	x ^= (x & UINT32_C(0x55555555)) <<  1U;
	x ^= (x & UINT32_C(0xcccccccc)) >>  2U;
	x ^= (x & UINT32_C(0x33333333)) <<  2U;
	x ^= (x & UINT32_C(0xf0f0f0f0)) >>  4U;
	x ^= (x & UINT32_C(0x0f0f0f0f)) <<  4U;
	x ^= (x & UINT32_C(0xff00ff00)) >>  8U;
	x ^= (x & UINT32_C(0x00ff00ff)) <<  8U;
	x ^= (x & UINT32_C(0xffff0000)) >> 16U;
	x ^= (x & UINT32_C(0x0000ffff)) << 16U;
	x ^= (x & UINT32_C(0xffff0000)) >> 16U;
	x ^= (x & UINT32_C(0xff00ff00)) >>  8U;
	x ^= (x & UINT32_C(0xf0f0f0f0)) >>  4U;
	x ^= (x & UINT32_C(0xcccccccc)) >>  2U;
	x ^= (x & UINT32_C(0xaaaaaaaa)) >>  1U;
	return x;
}

static const_inline uint64_t
rev64 (uint64_t x)
{
	x ^= (x & UINT64_C(0xaaaaaaaaaaaaaaaa)) >>  1U;
	x ^= (x & UINT64_C(0x5555555555555555)) <<  1U;
	x ^= (x & UINT64_C(0xcccccccccccccccc)) >>  2U;
	x ^= (x & UINT64_C(0x3333333333333333)) <<  2U;
	x ^= (x & UINT64_C(0xf0f0f0f0f0f0f0f0)) >>  4U;
	x ^= (x & UINT64_C(0x0f0f0f0f0f0f0f0f)) <<  4U;
	x ^= (x & UINT64_C(0xff00ff00ff00ff00)) >>  8U;
	x ^= (x & UINT64_C(0x00ff00ff00ff00ff)) <<  8U;
	x ^= (x & UINT64_C(0xffff0000ffff0000)) >> 16U;
	x ^= (x & UINT64_C(0x0000ffff0000ffff)) << 16U;
	x ^= (x & UINT64_C(0xffffffff00000000)) >> 32U;
	x ^= (x & UINT64_C(0x00000000ffffffff)) << 32U;
	x ^= (x & UINT64_C(0xffffffff00000000)) >> 32U;
	x ^= (x & UINT64_C(0xffff0000ffff0000)) >> 16U;
	x ^= (x & UINT64_C(0xff00ff00ff00ff00)) >>  8U;
	x ^= (x & UINT64_C(0xf0f0f0f0f0f0f0f0)) >>  4U;
	x ^= (x & UINT64_C(0xcccccccccccccccc)) >>  2U;
	x ^= (x & UINT64_C(0xaaaaaaaaaaaaaaaa)) >>  1U;
	return x;
}

#endif /* LIBCANTH_SRC_REV_ */
