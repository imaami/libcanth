/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file debruijn26.c
 * @brief 64-bit binary De Bruijn sequence test
 * @author Juuso Alasuutari
 */

#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "b25.h"
#include "b26.h"

#if defined _MSC_VER || defined __INTELLISENSE__
typedef uint8_t u8bit;
typedef int8_t s8bit;

typedef struct uint3 {
	unsigned value : 3;
} uint3;
#define uint3(x) (uint3){.value=(x)}
#define uint3_get(x) (x).value

typedef struct uint4 {
	unsigned value : 4;
} uint4;
#define uint4(x) (uint4){.value=(x)}
#define uint4_get(x) (x).value

typedef struct uint3x32 {
	uint8_t value[12];
} uint3x32;

static force_inline uint3x32 *
uint3x32_or (uint3x32 *const x,
             unsigned        i,
             uint3           v)
{
	i = (i & 31U) * 3U;
	uint16_t v_ = (uint16_t)uint3_get(v) << (i & 7U);
	i >>= 3U;
	x->value[i] |= (uint8_t)v_;
	v_ >>= 8U;
	if (v_)
		x->value[i + 1U] |= (uint8_t)v_;
	return x;
}

static const_inline unsigned
uint3x32_get (uint3x32 const *const x,
              unsigned const        i)
{
	return (((uint16_t)x->value[(i+1U & 31U) * 3U >> 3U] << 8U)
	        | x->value[(i & 31U) * 3U >> 3U]) >> (i & 7U) & 7U;
}

#else
typedef unsigned _BitInt(8) u8bit;
typedef signed _BitInt(8) s8bit;

typedef unsigned _BitInt(3) uint3;
#define uint3(x) (uint3)(x)
#define uint3_get(x) x

typedef unsigned _BitInt(4) uint4;
#define uint4(x) (uint4)(x)
#define uint4_get(x) x

typedef unsigned _BitInt(96) uint3x32;
#define uint3x32_nand(x,i,v)  \
 ((x) & ~((uint3x32)(v) << (3U * (i))))
#define uint3x32_or_(x,i,v)  \
 ((x) |  ((uint3x32)(v) << (3U * (i))))

static force_inline uint3x32 *
uint3x32_or (uint3x32 *const x,
             unsigned  const i,
             uint3     const v)
{
	*x |= (uint3x32)uint3_get(v) << (i * 3U);
	return x;
}

static const_inline unsigned
uint3x32_get (uint3x32 const *const x,
              unsigned        const i)
{
	return (*x >> (3U * i)) & 7U;
}
#endif

struct b26_bitcount {
	union {
		uint32_t bits;
		struct {
			u32bit z6 : 1; //  0
			u32bit z4 : 1; //  1
			u32bit z3 : 2; //  2
			u32bit z2 : 3; //  4
			u32bit z1 : 4; //  7
			u32bit na :10; // 11
			u32bit o1 : 4; // 21
			u32bit o2 : 3; // 25
			u32bit o3 : 2; // 28
			u32bit o4 : 1; // 30
			u32bit o6 : 1; // 31
		};
	};
};

// 0b0000       0       000000
// 0b0001       1       0000
// 0b0010       2       000
// 0b0011       3       00
// 0b0100       4       0
// 0b0101       5       1
// 0b0110       6       11
// 0b0111       7       111
// 0b1000       8       1111
// 0b1001       9       111111
struct b26_bits {
	uint3x32 data;
	u8bit    wrap : 6;
	u8bit    is_1 : 1;
	u8bit    pad_ : 1;
};

static uint64_t
parse_u64 (char *arg,
           int  *err);

static bool
b26_decompose (struct b26_bits *dest,
               uint64_t         seq);

static uint64_t
b26_seq_compose (struct b26_bits const *bits);

static struct b26_bits
b26_permute (struct b26_bits const *what,
             uint32_t               with);

static uint64_t
test_b25 (uint64_t x,
          uint16_t id)
{
	struct b26_bits bs = {0};
	if (!b26_decompose(&bs, x))
		return 0;
	unsigned r_ = 0;
	uint64_t x_ = is_b26(x, &r_);
	struct b25_id id_ = b25_id(id);
	uint32_t with = b25(id_);
	struct b26_bits bs1 = bs;
	for (unsigned n = 0;; ++n) {
		bs1 = b26_permute(&bs1, with);
		uint64_t seq = b26_seq_compose(&bs1);
		unsigned rot = 0;
		uint64_t lyn = is_b26(seq, &rot);
		if (lyn == x_)
			break;
		if (lyn)
			(void)printf("%016" PRIx64 " %4u %016" PRIx64
			             " %016" PRIx64 " %2u %08" PRIx32
			             " %4u %2u\n", x, n, seq, lyn, rot,
			             with, (unsigned)id_.idx,
			             (unsigned)id_.rot);
	}

	return 0;
}

int
main (int    c,
      char **v)
{
	char *b26_bin_file = nullptr;
	uint64_t *b26_data = nullptr;
	unsigned num_ids = 0;

second_round:
	for (int i = 0; ++i < c;) {
		if (v[i][0] == '-' &&
		    v[i][1] == '-' &&
		    v[i][2] == 'b' &&
		    v[i][3] == '2' &&
		    v[i][4] == '6') {
			if (!b26_data) {
				char *p = &v[i][5];
				if (*p == '=')
					++p;
				else if (!*p && ++i < c)
					p = v[i];
				else
					return 1;
				if (b26_bin_file)
					return 1;
				b26_bin_file = p;
			}
			continue;
		}

		int e = 0;
		uint64_t u = parse_u64(v[i], &e);
		if (!e && u < 65536U) {
			if (!b26_data) {
				num_ids++;
				continue;
			}

			for (size_t k = 0; k < 67108864U; ++k) {
				(void)test_b25(b26_data[k], (uint16_t)u);
			}
/*
			struct b26_bits bs = {0};
			if (b26_decompose(&bs, u)) {
				union {
					unsigned char ch[12];
					uint32_t      u32[3];
				} tmp = {0};
				memcpy(&tmp.ch[0], &bs.data, sizeof tmp.ch);
				printf("%08" PRIx32 "%08" PRIx32 "%08" PRIx32 "\n",
				       tmp.u32[2], tmp.u32[1], tmp.u32[0]);
				uint64_t seq = b26_seq_compose(&bs);
				(void)printf("0x%016" PRIx64 "\n", seq);
			}
*/
		}
	}

	if (!b26_bin_file)
		return 0;

	if (!b26_data) {
		b26_data = b26_load(b26_bin_file, false);
		if (b26_data) {
			if (num_ids)
				goto second_round;
			for (size_t i = 0; i < 67108864U; ++i) {
				(void)test_b25(b26_data[i], 0);
			}
		}
	}

	free(b26_data);
}

struct i_m {
	int8_t index;
	uint8_t mask;
	uint8_t want;
};

static const_inline struct i_m
b26_bitcount_offset (unsigned const size,
                     bool     const is_1)
{
	return (struct i_m[]){
		{-1, 0, 0}, { 7,15, 8},
		{ 4, 7, 4}, { 2, 3, 2},
		{ 1, 1, 1}, {-1, 0, 0},
		{ 0, 1, 1}, {-1, 0, 0},
		{21,15, 8}, {25, 7, 4},
		{28, 3, 2}, {30, 1, 1},
		{-1, 0, 0}, {31, 1, 1},
	}[(size < 7U) * (size + is_1 * 7U)];
}

static bool
b26_bitcount_add (struct b26_bitcount *const dest,
                  unsigned             const size,
                  bool                 const is_1)
{
	struct i_m i = b26_bitcount_offset(size, is_1);
	if (i.index == -1)
		return false;
	unsigned off = (unsigned)i.index;
	uint32_t cur = (dest->bits >> off) & i.mask;
	if (cur >= i.want)
		return false;
	dest->bits += UINT32_C(1) << off;
	return true;
}

static uint64_t
b26_seq_compose (struct b26_bits const *const bits)
{
	uint64_t seq = 0U;
	unsigned i = 32U;
	unsigned is_1 = bits->is_1;
	do {
		unsigned n = uint3x32_get(&bits->data, --i);
		seq += is_1;
		seq <<= n;
		seq -= is_1;
		//printf("\033[%c;3%cm\xe2\x96%c", (char)('0'+is_1),
		//       (char)('0'+n+(n<6U)), (char)(0x80+n));
		is_1 ^= 1U;
	} while (i);
	//puts("\033[m");
	//i = 32U;
	//is_1 = bits->is_1;
	//do {
	//	unsigned n = uint3x32_get(&bits->data, --i);
	//	putchar('0' + (int)n);
	//	is_1 ^= 1U;
	//} while (i);
	//putchar('\n');
	return rol(seq, bits->wrap);
}

static bool
b26_decompose (struct b26_bits *const dest,
               uint64_t               seq)
{
	struct b26_bitcount bc = {0};
	unsigned nbit = count_msb_0(seq);
	unsigned is_1 = !nbit;

	if (is_1)
		nbit = count_msb_1(seq);

	unsigned wrap = is_1 == (seq & 1U) && nbit < 64U;
	if (wrap)
		wrap = is_1 ? count_lsb_1(seq)
		            : count_lsb_0(seq);
	unsigned n = nbit + wrap;
	unsigned i = 31U;

	if (!b26_bitcount_add(&bc, n, is_1))
		return false;

	struct b26_bits bs = {
		.wrap = (u8bit)wrap,
		.is_1 = (u8bit)is_1,
	};
	uint3x32_or(&bs.data, 31U, uint3(n));

	for (unsigned left = 64U - n; left; left -= nbit) {
		seq <<= nbit;
		is_1 ^= 1U;
		nbit = is_1 ? count_msb_1(seq)
		            : count_msb_0(seq);
		if (nbit > left)
			nbit = left;
		if (!b26_bitcount_add(&bc, nbit, is_1))
			return false;
		uint3x32_or(&bs.data, --i, uint3(nbit));
	}

	*dest = bs;
	return true;
}

static struct b26_bits
b26_permute (struct b26_bits const *const what,
             uint32_t               const with)
{
	struct b26_bits ret = {
		.wrap = what->wrap,
		.is_1 = what->is_1
	};
	uint64_t b = ((uint64_t)with << 32U) | with;

	for (unsigned i = 0; i < 32U; ++i) {
		unsigned n = uint3x32_get(&what->data, i);
		uint3x32_or(&ret.data, (b >> i) & 31U, uint3(n));
	}

	return ret;
}

static uint64_t
parse_u64 (char *const arg,
           int  *const err)
{
	int e = !arg ? EFAULT : !*arg ? ENODATA : 0;
	uint64_t u64 = 0;

	if (!e) {
		errno = 0;
		char *p = arg;
		int64_t i64 = _Generic(
			i64, long: strtol,
			long long: strtoll
		)(arg, &p, 0);
		e = errno;

		if (!e) {
			u64 = (uint64_t)i64;

		} else if (e == ERANGE &&
		           i64 == _Generic(i64, long: LONG_MAX,
		                           long long: LLONG_MAX)) {
			errno = 0;
			p = arg;
			u64 = _Generic(
				u64, unsigned long: strtoul,
				unsigned long long: strtoull
			)(arg, &p, 0);
			e = errno;
		}

		if (!e && *p)
			e = EINVAL;
	}

	*err = e;
	return u64;
}
