/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file b26.h
 * @brief 64-bit binary De Bruijn sequences
 * @author Juuso Alasuutari
 */
#ifndef LIBCANTH_SRC_B26_H_
#define LIBCANTH_SRC_B26_H_

#include "bits.h"

#if defined _MSC_VER || defined __INTELLISENSE__
typedef uint32_t u32bit;
typedef struct uint6 {
	unsigned value : 6;
} uint6;
#define uint6(x) (uint6){.value=(x)}
#define uint6_get(x) (x).value
#else
typedef unsigned _BitInt(32) u32bit;
typedef unsigned _BitInt(6) uint6;
#define uint6(x) (uint6)(x)
#define uint6_get(x) x
#endif

struct b26_id {
	u32bit rot :  6;
	u32bit idx : 26;
};
_Static_assert(sizeof(struct b26_id) == sizeof(uint32_t),
               "width of b26_id must be 32 bits");

static const_inline struct b26_id
b26_id (uint32_t id)
{
	return (struct b26_id){
		.rot = id & UINT32_C(0x03f),
		.idx = id >> 6U,
	};
}

static const_inline uint32_t
b26_id_to_u32 (struct b26_id const id)
{
	return (uint32_t)((uint32_t)id.idx << 6U | id.rot);
}

extern uint64_t *
b26_load (char const *path,
          bool        validate);

extern uint64_t
b26_identify (uint64_t        x,
              struct b26_id  *id,
              uint64_t const *arr);

extern uint64_t
is_b26 (uint64_t  const x,
        unsigned *const rot);

#endif /* LIBCANTH_SRC_B26_H_ */
