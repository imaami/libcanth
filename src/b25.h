/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file b25.h
 * @brief 32-bit binary De Bruijn sequences
 * @author Juuso Alasuutari
 */
#ifndef LIBCANTH_SRC_B25_H_
#define LIBCANTH_SRC_B25_H_

#include <inttypes.h>

#include "bits.h"

#if defined _MSC_VER || defined __INTELLISENSE__
typedef uint16_t u16bit;
typedef struct uint5 {
	unsigned value : 5;
} uint5;
#define uint5(x) (uint5){.value=(x)}
#define uint5_get(x) (x).value
#else
typedef unsigned _BitInt(16) u16bit;
typedef unsigned _BitInt(5) uint5;
#define uint5(x) (uint5)(x)
#define uint5_get(x) x
#endif

extern const uint32_t B25[2048];

struct b25_id {
	u16bit rot :  5;
	u16bit idx : 11;
};
_Static_assert(sizeof(struct b25_id) == sizeof(uint16_t),
               "width of b25_id must be 16 bits");

extern const struct b25_id four8[   1];
extern const struct b25_id two16[ 119];
extern const struct b25_id one32[1936];

static const_inline uint32_t
u10x3_count (uint32_t const x)
{
	return x & UINT32_C(3);
}

typedef struct b25_loc {
	u16bit offset : 5;
	u16bit period : 6;
	u16bit unused : 5;
} b25_loc;

_Static_assert(sizeof(b25_loc) == sizeof(uint16_t),
               "width of b25_loc must be 16 bits");

struct b25_info {
	uint64_t div;
	uint32_t loc[3];
	uint32_t mask[9];
};

static const_inline unsigned
b25_div_offset (uint5 const i)
{
	unsigned const k = uint5_get(i) >> 3U;
	return (uint5_get(i) & 7U)
	     * (uint8_t[]){3,2,1,1}[k]
	     + (uint8_t[]){8,32,48,56}[k];
}

static const_inline uint64_t
b25_div_mask (uint5 const i)
{
	return (uint8_t[]){7,3,1,1}[uint5_get(i) >> 3U];
}

static force_inline void
u10x3_append (uint32_t *const dst,
              uint5     const lo5,
              uint5     const hi5)
{
	uint32_t n = u10x3_count(*dst);
	if (n < 3U) {
		n = n * 10U + 2U;
		*dst += ((uint32_t)uint5_get(hi5) << 5U | uint5_get(lo5)) << n
		      | UINT32_C(1);
	}
}

static force_inline unsigned
b25_div_get (uint64_t const div,
             uint5    const i)
{
	return (unsigned)(div >> b25_div_offset(i) & b25_div_mask(i));
}

static const_inline unsigned
b25_div_get_total (uint64_t const div)
{
	return div & UINT64_C(15);
}

static const_inline unsigned
b25_div_get_unique (uint64_t const div)
{
	return (div >> 4U) & UINT64_C(15);
}

static force_inline unsigned
b25_info_get_num_cycles (struct b25_info const *const src)
{
	return b25_div_get_total(src->div) - b25_div_get(src->div, uint5(0));
}

extern b25_loc
b25_info_get_cycle (struct b25_info const *src,
                    unsigned               i);

static force_inline unsigned
b25_info_get_num_period_cycles (struct b25_info const *const src,
                                unsigned const               prd)
{
	if (!prd || prd > 32U)
		return 0;
	return b25_div_get(src->div, uint5(prd - 1U));
}

static const_inline struct b25_id
b25_id (uint16_t id)
{
	return (struct b25_id){
		.rot = id & UINT16_C(0x01f),
		.idx = id >> 5U,
	};
}

static const_inline uint32_t
b25 (struct b25_id const id)
{
	return ror32(B25[id.idx], id.rot);
}

static const_inline uint16_t
b25_id_to_u16 (struct b25_id const id)
{
	return (uint16_t)((uint16_t)id.idx << 5U | id.rot);
}

extern struct b25_info
b25_info (uint32_t seq);

static force_inline struct b25_info
b25_info_by_id (struct b25_id const id)
{
	return b25_info(ror32(B25[id.idx], id.rot));
}

extern void
b25_info_print (struct b25_info const *info,
                struct b25_id          id);

#endif /* LIBCANTH_SRC_B25_H_ */
