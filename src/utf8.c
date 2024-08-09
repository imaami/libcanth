/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file utf8.c
 *
 * @author Juuso Alasuutari
 */
#ifndef NDEBUG
# include <assert.h>
#endif /* !NDEBUG */
#include <errno.h>
#include <inttypes.h>

#include "dbg.h"
#include "utf8.h"

#ifndef NDEBUG
static const_inline char const *
utf8_st8_name (enum utf8_st8 st8)
{
	constexpr static char const name[][8] = {
		#define F(n,m,...) [n] = #m,
		UTF8_PARSER_DESCRIPTOR(F)
		#undef F
	};

	if (!is_negative(st8) && st8 < array_size(name))
		return name[st8];

	return nullptr;
}
#endif /* !NDEBUG */

#if 0
constexpr static const uint8_t utf8_range[][4] = {
	#define F(n,m,l,...) [n] = {__VA_ARGS__},
	UTF8_PARSER_DESCRIPTOR(F)
	#undef F
};
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
 * @brief Get the parser's current state as a state index.
 *
 * Note that this is not a pure getter, as @ref utf8::error is
 * clobbered on failure. Take precautions to save it if needed.
 *
 * @param u8p The UTF-8 parser. The error field is set on failure
 *            without regard to its current state.
 * @param st8 Where to store the current state index on success.
 *            Not modified on failure. Must not be `nullptr`.
 * @return `true` on success. On failure sets @ref ut8::error to
 *         `ENOTRECOVERABLE` and returns `false`.
 */
nonnull_in()
static force_inline bool
utf8_get_state (struct utf8 *const   u8p,
                enum utf8_st8 *const st8)
{
	int e = utf8_state_from_bit(u8p->state);
	if (e < 0) {
		u8p->error = ENOTRECOVERABLE;
		return false;
	}

	assume_value_bits(e, 0xf);

	*st8 = (enum utf8_st8)e;
	return true;
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

nonnull_in()
static const_inline bool
utf8_done (struct utf8 const *const u8p)
{
	return u8p->state & (utf8_bit(asc) | utf8_bit(cb1));
}

/**
 * @brief Check if a UTF-8 state index represents a UTF-8 leading byte
 *        or an ASCII byte.
 * @param st8 The state index to check.
 * @return `true` if the state is a leading byte or an ASCII byte, otherwise `false`.
 */
static const_inline bool
utf8_st8_is_leading_byte (enum utf8_st8 st8)
{
	return st8 < (enum utf8_st8)8;
}

/**
 * @brief Write a byte to a position in the UTF-8 cache determined
 *        by the cache's internal state and the given state index.
 *
 * This function updates the UTF-8 parser's byte cache with a new byte
 * in the context of the current parser state. If the state represents
 * a leading byte or an ASCII byte, the cache is reset before update.
 *
 * The cache is structured to hold bytes from a multi-byte sequence,
 * with the leading byte towards the start and continuation bytes
 * following.
 *
 * @param u8p The UTF-8 parser to update.
 * @param st8 The parser index representation of the UTF-8
 *            parser's state after reading the given byte.
 * @param byte The byte to push into the cache.
 */
nonnull_in(1)
static force_inline void
utf8_push_to_cache (struct utf8 *const u8p,
                    enum utf8_st8      st8,
                    uint8_t            byte)
{
	constexpr static const uint8_t utf8_len[16] = {
		#define F(n,m,l,...) [n] = l,
		UTF8_PARSER_DESCRIPTOR(F)
		#undef F
	};

	uint8_t len = utf8_len[st8] IF_DEBUG(, k = 0);

	if (utf8_st8_is_leading_byte(st8)) {
		// Leading byte or ASCII
		u8p->cache[0] = len;
		__builtin_memset(&u8p->cache[1], 0, sizeof u8p->cache - 1U);
#ifndef NDEBUG
		pr_(" [\033[1;3%" PRIu8 "m%02" PRIx8 "\033[m]", len, len);
		++k;
#endif /* !NDEBUG */
	}

	uint8_t pos = u8p->cache[0] + 1U - len;
	u8p->cache[pos] = byte;

#ifndef NDEBUG
	for (;k < pos; ++k) {
		pr_(" [%02" PRIx8 "]", u8p->cache[k]);
	}
	pr_(" [\033[1;3%" PRIu8 "m%02" PRIx8 "\033[m]", pos, u8p->cache[k]);
	for (; ++k < sizeof u8p->cache;) {
		pr_(" [%02" PRIx8 "]", u8p->cache[k]);
	}
#endif /* !NDEBUG */
}

#undef UTF8_PARSER_DESCRIPTOR

nonnull_in(1,2)
static bool
utf8_set_state (struct utf8 *const   u8p,
                enum utf8_st8 *const st8,
                uint8_t              byte)
{
	uint16_t next_bit = utf8_lut[byte] & utf8_allowed_states(*st8);

	int e = utf8_state_from_bit(next_bit);
	if (e < 0) {
		u8p->error = EILSEQ;
		return false;
	}

	assume_value_bits(e, 0xf);

	u8p->state = next_bit;
	utf8_push_to_cache(u8p, (enum utf8_st8)e, byte);

	pr_dbg_("  %6s -> %-6s", utf8_st8_name(*st8),
	                         utf8_st8_name((enum utf8_st8)e));

	*st8 = (enum utf8_st8)e;
	return true;
}

nonnull_in() nonnull_out
uint8_t const *
utf8_next (struct utf8 *const  u8p,
           uint8_t const      *ptr)
{
	enum utf8_st8 st8 = utf8_ini;

	if (utf8_get_state(u8p, &st8)) {
		while (utf8_set_state(u8p, &st8, *ptr)) {
			++ptr;
			if (utf8_done(u8p)) {
				u8p->error = 0;
				break;
			}
		}
	}

	return ptr;
}
