/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file utf8.c
 *
 * @author Juuso Alasuutari
 */
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>

#include "compat.h"
#include "dbg.h"
#include "ligma.h"
#include "util.h"

#if clang_older_than_version(19) \
 && clang_newer_than_version(11)
diag_clang(push)
diag_clang(ignored "-Wgnu-zero-variadic-macro-arguments")
#endif /* clang < 19 && clang > 11 */

struct utf8 {
	size_t   n_uc;
	uint16_t state;
	uint8_t  bs[5];
	uint8_t  error;
};

static struct utf8
utf8_parser (void);

static uint8_t const *
utf8_next (struct utf8 *const    u8p,
           uint8_t const        *ptr,
           uint8_t const *const  end);

static force_inline char const *
utf8_result (struct utf8 const *u8p)
{
	return (char const *)&u8p->bs[u8p->bs[0]];
}

static void
utf8_reset (struct utf8 *const u8p);

#if 0
static void
utf8_flowchart (void);
#endif

int
main (int    c,
      char **v)
{
	//utf8_flowchart();
	bool separate = false;
	struct utf8 u8p = utf8_parser();
	for (int i = 0; ++i < c;) {
		uint8_t const *q = (uint8_t *)v[i];
		for (uint8_t const *p = q; *p;) {
			p = utf8_next(&u8p, p, nullptr);
			if (u8p.error)
				break;

			//fputs(utf8_result(&u8p), stdout);
		}

		if (u8p.error == EILSEQ)
			utf8_reset(&u8p);
		else {
			if (separate)
				putchar(' ');
			fputs(v[i], stdout);
		}

		separate = u8p.error != EAGAIN;
	}
	putchar('\n');
}

#define UTF8_PARSER_DESCRIPTOR(F) \
        F( 0,  asc,    0x01, 127, 0, 0) /* ASCII - never followed by continuation byte 0x80-0xbf */ \
        F( 1,  lb2,    0xc2,  30, 0, 0) /* start of 2-byte sequence, any continuation may follow */ \
        F( 2,  lb3_e0, 0xe0,   1, 0, 0) /* start of 3-byte sequence, next byte must be 0xa0-0xbf */ \
        F( 3,  lb3,    0xe1,  12, 1, 2) /* start of 3-byte sequence, any continuation may follow */ \
        F( 4,  lb3_ed, 0xed,   1, 0, 0) /* start of 3-byte sequence, next byte must be 0x80-0x9f */ \
        F( 5,  lb4_f0, 0xf0,   1, 0, 0) /* start of 4-byte sequence, next byte must be 0x90-0xbf */ \
        F( 6,  lb4,    0xf1,   3, 0, 0) /* start of 4-byte sequence, any continuation may follow */ \
        F( 7,  lb4_f4, 0xf4,   1, 0, 0) /* start of 4-byte sequence, next byte must be 0x80-0x8f */ \
        F( 8,  cb3_f4, 0x80,  16, 0, 0) /* 3rd-to-last continuation, follows 0xf4                */ \
        F( 9,  cb3,    0x80,  64, 0, 0) /* 3rd-to-last continuation, follows 0xf1-0xf3           */ \
        F(10,  cb3_f0, 0x90,  48, 0, 0) /* 3rd-to-last continuation, follows 0xf0                */ \
        F(11,  cb2_ed, 0x80,  32, 0, 0) /* 2nd-to-last continuation, follows 0xed                */ \
        F(12,  cb2,    0x80,  64, 0, 0) /* 2nd-to-last continuation, follows 0xe1-0xec,0xee-0xef */ \
        F(13,  cb2_e0, 0xa0,  32, 0, 0) /* 2nd-to-last continuation, follows 0xe0                */ \
        F(14,  cb1,    0x80,  64, 0, 0) /* last continuation, common to all multi-byte sequences */ \
        F(15,  ini,       0,   0, 0 ,0) /* initial state bit, never set in lookup table elements */

#define utf8_st8(m)     utf8_##m
#define utf8_bit(m)     (uint16_t)(1U << utf8_st8(m))

#define F(n,m,...)      utf8_st8(m) = n,
fixed_enum(utf8_st8, uint8_t)   {UTF8_PARSER_DESCRIPTOR(F)};
#undef F

static const_inline char const *
utf8_st8_name (IF_NDEBUG(useless) enum utf8_st8 st8)
{
#ifndef NDEBUG
	constexpr static char const name[][8] = {
		#define F(n,m,...) [utf8_st8(m)] = #m,
		UTF8_PARSER_DESCRIPTOR(F)
		#undef F
	};

	if (!is_negative(st8) && st8 < array_size(name))
		return name[st8];
#endif /* !NDEBUG */

	return nullptr;
}

#if 0
#define make_range(n,m,...)     [n] = {__VA_ARGS__},
constexpr static
const uint8_t utf8_range[][4] = {UTF8_PARSER_DESCRIPTOR(make_range)};
#undef make_range
#endif

#define X1(x)   x
#define X2(x)   x,x
#define X3(x)   x,x,x
#define X8(x)   x,x,x,x,x,x,x,x
#define X11(x)  X8(x),x,x,x
#define X12(x)  X11(x),x
#define X16(x)  X8(x),X8(x)
#define X30(x)  X16(x),X12(x),x,x
#define X32(x)  X16(x),X16(x)
#define X128(x) X32(x),X32(x),X32(x),X32(x)
#define X(n, x) X##n(x)

constexpr static const uint16_t utf8_lut[] = {
	X(128,utf8_bit(asc)  ), /* 0x00-0x7f */

	/* 0x80-0x8f */
	X(16,utf8_bit(cb3)   \
	    |utf8_bit(cb3_f4)\
	    |utf8_bit(cb2)   \
	    |utf8_bit(cb2_ed)\
	    |utf8_bit(cb1)   ),

	/* 0x90-0x9f */
	X(16,utf8_bit(cb3)   \
	    |utf8_bit(cb3_f0)\
	    |utf8_bit(cb2)   \
	    |utf8_bit(cb2_ed)\
	    |utf8_bit(cb1)   ),

	/* 0xa0-0xbf */
	X(32,utf8_bit(cb3)   \
	    |utf8_bit(cb3_f0)\
	    |utf8_bit(cb2)   \
	    |utf8_bit(cb2_e0)\
	    |utf8_bit(cb1)   ),

	X( 2,0               ), /* 0xc0-0xc1 */
	X(30,utf8_bit(lb2)   ), /* 0xc2-0xdf */
	X( 1,utf8_bit(lb3_e0)), /* 0xe0      */
	X(12,utf8_bit(lb3)   ), /* 0xe1-0xec */
	X( 1,utf8_bit(lb3_ed)), /* 0xed      */
	X( 2,utf8_bit(lb3)   ), /* 0xee-0xef */
	X( 1,utf8_bit(lb4_f0)), /* 0xf0      */
	X( 3,utf8_bit(lb4)   ), /* 0xf1-0xf3 */
	X( 1,utf8_bit(lb4_f4)), /* 0xf4      */
	X(11,0               ), /* 0xf5-0xff */
};

#undef X
#undef X128
#undef X32
#undef X30
#undef X16
#undef X12
#undef X11
#undef X8
#undef X3
#undef X2
#undef X1

constexpr static const uint16_t utf8_dst[16] = {
	[utf8_asc   ] = utf8_bit(asc)
	              | utf8_bit(lb2)
	              | utf8_bit(lb3_e0)
	              | utf8_bit(lb3)
	              | utf8_bit(lb3_ed)
	              | utf8_bit(lb4_f0)
	              | utf8_bit(lb4)
	              | utf8_bit(lb4_f4),
	[utf8_lb2   ] = utf8_bit(cb1),
	[utf8_lb3_e0] = utf8_bit(cb2_e0),
	[utf8_lb3   ] = utf8_bit(cb2),
	[utf8_lb3_ed] = utf8_bit(cb2_ed),
	[utf8_lb4_f0] = utf8_bit(cb3_f0),
	[utf8_lb4   ] = utf8_bit(cb3),
	[utf8_lb4_f4] = utf8_bit(cb3_f4),
	[utf8_cb3_f4] = utf8_bit(cb2),
	[utf8_cb3   ] = utf8_bit(cb2),
	[utf8_cb3_f0] = utf8_bit(cb2),
	[utf8_cb2_ed] = utf8_bit(cb1),
	[utf8_cb2   ] = utf8_bit(cb1),
	[utf8_cb2_e0] = utf8_bit(cb1),
	[utf8_cb1   ] = utf8_bit(asc)
	              | utf8_bit(lb2)
	              | utf8_bit(lb3_e0)
	              | utf8_bit(lb3)
	              | utf8_bit(lb3_ed)
	              | utf8_bit(lb4_f0)
	              | utf8_bit(lb4)
	              | utf8_bit(lb4_f4),
	[utf8_ini   ] = utf8_bit(asc)
	              | utf8_bit(lb2)
	              | utf8_bit(lb3_e0)
	              | utf8_bit(lb3)
	              | utf8_bit(lb3_ed)
	              | utf8_bit(lb4_f0)
	              | utf8_bit(lb4)
	              | utf8_bit(lb4_f4),
};

constexpr static const uint8_t utf8_len[16] = {
	[utf8_asc   ] = 1,
	[utf8_lb2   ] = 2,
	[utf8_lb3_e0] = 3,
	[utf8_lb3   ] = 3,
	[utf8_lb3_ed] = 3,
	[utf8_lb4_f0] = 4,
	[utf8_lb4   ] = 4,
	[utf8_lb4_f4] = 4,
	[utf8_cb3_f4] = 3,
	[utf8_cb3   ] = 3,
	[utf8_cb3_f0] = 3,
	[utf8_cb2_ed] = 2,
	[utf8_cb2   ] = 2,
	[utf8_cb2_e0] = 2,
	[utf8_cb1   ] = 1,
	[utf8_ini   ] = 0,
};

#if 0
static void
utf8_flowchart (void)
{
	char buf[1024] = "digraph {\n";
	size_t w = sizeof "digraph {";

	for (size_t i = 0; i < sizeof utf8_range / sizeof utf8_range[0]; ++i) {
		unsigned a = utf8_range[i][0];
		unsigned b = utf8_range[i][1];
		unsigned c = utf8_range[i][2];
		unsigned d = utf8_range[i][3];
		if (!c) {
			b += d;
			d = 0;
		} else if (!d)
			c = 0;
		if (!(b + d))
			continue;

		int n = snprintf(&buf[w], sizeof buf - w,
		                 "\tx%02zx [label=\"[", i);
		if (n < 1)
			return;
		w += (size_t)n;
		if (b) {
			n = b == 1
			    ? snprintf(&buf[w], sizeof buf - w,
			               "\\\\x%02x", a)
			    : snprintf(&buf[w], sizeof buf - w,
			               "\\\\x%02x-\\\\x%02x", a, a + b - 1);
			if (n < 1)
				return;
			w += (size_t)n;
		}
		if (d) {
			n = d == 1
			    ? snprintf(&buf[w], sizeof buf - w,
			               "\\\\x%02x", a + b + c)
			    : snprintf(&buf[w], sizeof buf - w,
			               "\\\\x%02x-\\\\x%02x", a + b + c, a + b + c + d - 1);
			if (n < 1)
				return;
			w += (size_t)n;
		}
		buf[w++] = ']';
		buf[w++] = '\"';
		buf[w++] = ']';
		buf[w++] = ';';
		buf[w++] = '\n';
	}

	for (size_t src = 0;
	     src < sizeof utf8_dst /
	           sizeof utf8_dst[0]; ++src) {
		if (!(utf8_range[src][1] +
		      utf8_range[src][3]))
			continue;
		for (unsigned bits = utf8_dst[src],
		     dst = 0; bits; bits >>= 1U, ++dst) {
			if (!(bits & 1U))
				continue;
			int n = snprintf(&buf[w], sizeof buf - w,
			                 "\tx%02zx -> x%02x;\n",
			                 src, dst);
			if (n < 1)
				return;
			w += (size_t)n;
		}
	}

	buf[w++] = '}';
	buf[w++] = '\n';
	buf[w] = '\0';
	fputs(buf, stdout);
}
#endif

static struct utf8
utf8_parser (void)
{
	return (struct utf8) {
		.n_uc  = 0,
		.state = utf8_bit(ini),
		.bs    = {0},
		.error = 0,
	};
}

/**
 * @brief Convert a state bit to a state index.
 * @param bit The state bit to convert. Must be a non-zero power of 2.
 * @return The state index if `bit` is valid, otherwise -1.
 */
static const_inline int
utf8_state_from_bit (uint16_t bit)
{
	return bit && !(bit & (bit - 1U))
		? __builtin_ctz(bit) : -1;
}

/**
 * @brief Get a bitmask of allowed states following a given state.
 * @param st8 The current state index.
 * @return Bitmask of allowed states following `st8` if it is valid
 *         input, otherwise 0.
 */
static const_inline uint16_t
utf8_allowed_states (unsigned st8)
{
#ifdef NDEBUG
	if (st8 >= array_size(utf8_dst))
		return 0;
#else
	assert(st8 < array_size(utf8_dst));
#endif
	return utf8_dst[st8];
}

/**
 * @brief Get the bitmask of allowed states following a given state bit.
 * @param bit The current state bit. Must be a non-zero power of 2.
 * @return Bitmask of allowed states following `bit` if it is valid
 *         input, otherwise 0.
 */
static const_inline uint16_t
utf8_allowed_states_from_bit (uint16_t bit)
{
	int s = utf8_state_from_bit(bit);
#ifdef NDEBUG
	if (s < 0)
		return 0;
#else
	assert(s >= 0);
#endif
	return utf8_dst[s];
}

static const_inline bool
utf8_complete (uint16_t bit)
{
	assert(bit && !(bit & (bit - 1U)));
	return bit & (utf8_bit(asc) | utf8_bit(cb1));
}

static force_inline void
utf8_push (struct utf8 *const u8p,
           uint16_t           bit,
           uint8_t            byte)
{
	int st8 = utf8_state_from_bit(bit);
	if (st8 > -1 && st8 < (int)utf8_ini) {
		uint8_t pos = (uint8_t)sizeof u8p->bs - utf8_len[st8];
		//uint8_t k = 0;
		if (st8 < 8) {
			// Leading byte or ASCII
			u8p->bs[0] = pos;
			__builtin_memset(&u8p->bs[1], 0, sizeof u8p->bs - 1U);
			//printf(" [\e[1;35m%02" PRIx8 "\e[m]", u8p->bs[0]);
			//++k;
		}
		u8p->bs[pos] = byte;
		//for (;k < pos; ++k) {
		//	printf(" [%02" PRIx8 "]", u8p->bs[k]);
		//}
		//printf(" [\e[1;35m%02" PRIx8 "\e[m]", u8p->bs[k]);
		//for (; ++k < sizeof u8p->bs;) {
		//	printf(" [%02" PRIx8 "]", u8p->bs[k]);
		//}
		//putchar('\n');
	}
}

static void
utf8_reset (struct utf8 *const u8p)
{
	u8p->state = utf8_bit(ini);
	u8p->bs[0] = 0;
	u8p->error = 0;
}

static uint8_t const *
utf8_next (struct utf8 *const    u8p,
           uint8_t const        *ptr,
           uint8_t const *const  end)
{
	int sb = utf8_state_from_bit(u8p->state);
	if (sb < 0) {
		u8p->error = ENOTRECOVERABLE;
		return nullptr;
	}
	enum utf8_st8 st8 = (enum utf8_st8)sb;

	if (end) {
		if (ptr < end) for (;;) {
			uint16_t bit = utf8_lut[*ptr] & utf8_allowed_states(st8);
			sb = utf8_state_from_bit(bit);
			if (sb < 0) {
				u8p->error = EILSEQ;
				return nullptr;
			}
			pr_dbg("%6s -> %-6s", utf8_st8_name(st8),
			       utf8_st8_name((enum utf8_st8)sb));

			utf8_push(u8p, bit, *ptr);
			u8p->state = bit;
			++ptr;

			if (utf8_complete(bit)) {
				++u8p->n_uc;
				u8p->error = 0;
				break;
			}

			if (ptr == end) {
				u8p->error = EAGAIN;
				return nullptr;
			}

			st8 = (enum utf8_st8)sb;
		}
	} else {
		if (*ptr) for (;;) {
			uint16_t bit = utf8_lut[*ptr] & utf8_allowed_states(st8);
			sb = utf8_state_from_bit(bit);
			if (sb < 0) {
				u8p->error = EILSEQ;
				return nullptr;
			}
			pr_dbg("%6s -> %-6s", utf8_st8_name(st8),
			       utf8_st8_name((enum utf8_st8)sb));

			utf8_push(u8p, bit, *ptr);
			u8p->state = bit;
			++ptr;

			if (utf8_complete(bit)) {
				++u8p->n_uc;
				u8p->error = 0;
				break;
			}

			if (!*ptr) {
				u8p->error = EAGAIN;
				return nullptr;
			}

			st8 = (enum utf8_st8)sb;
		}
	}

	return ptr;
}

#if clang_older_than_version(19) \
 && clang_newer_than_version(11)
diag_clang(pop)
#endif /* clang < 19 && clang > 11 */
