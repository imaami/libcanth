/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file utf8.h
 *
 * @author Juuso Alasuutari
 */
#ifndef LIBCANTH_SRC_UTF8_H_
#define LIBCANTH_SRC_UTF8_H_

#include <stddef.h>
#include <stdint.h>

#include "compat.h"
#include "util.h"

/** @brief UTF-8 parser states.
 *
 * The parser is implemented as a state machine with 15 operational
 * states and an initial state. The states are encoded as bit flags
 * in the byte value lookup table to track multi-byte sequences. In
 * other situations, state enumeration values are used directly.
 *
 * The first 8 states (indices 0 through 7) are leading byte states,
 * indices 8 through 14 are continuation byte states, and finally at
 * index 15 is the initial state. States 0, 14, and 15 are special
 * in that each can be followed by any of the 8 leading byte states.
 * The remaining 13 states (1 through 13) can be followed by exactly
 * one state.
 *
 * Due to explicit restrictions in the UTF-8 encoding scheme, not all
 * leading byte values can be followed by any continuation byte value.
 * This is reflected in the presence of overlapping state transitions
 * in the table below.
 *
 * There are three distinct types of restrictions at play:
 *
 * 1. Overlong encodings are avoided by rejecting the leading bytes
 *    0xc0-0xc1 and 0xf5-0xff, and limiting the range of the first
 *    continuation byte following the leading bytes 0xe0 and 0xf0.
 *
 * 2. Surrogate code points are avoided by restricting the range of
 *    the first continuation byte following the leading byte 0xed.
 *
 * 3. Code points above the Unicode maximum of 0x10ffff are avoided
 *    by lowering the maximum value of the first continuation byte
 *    following the leading byte 0xf4.
 */
#define UTF8_PARSER_DESCRIPTOR(F)                                                                      \
        /* ╭───────────────────────────────index                                                    */ \
        /* │      ╭────────────────────────label                                                    */ \
        /* │      │    ╭───────────────────size                                                     */ \
        /* │      │    │     ╭─────────────start                                                    */ \
        /* │      │    │     │    ╭────────run                                                      */ \
        /* │      │    │     │    │  ╭─────skip                                                     */ \
        /* │      │    │     │    │  │  ╭──run                                                      */ \
        F( 0,  asc,    1, 0x01, 127, 0, 0) /* ASCII - never followed by continuation byte 0x80-0xbf */ \
        F( 1,  lb2,    2, 0xc2,  30, 0, 0) /* start of 2-byte sequence, any continuation may follow */ \
        F( 2,  lb3_e0, 3, 0xe0,   1, 0, 0) /* start of 3-byte sequence, next byte must be 0xa0-0xbf */ \
        F( 3,  lb3,    3, 0xe1,  12, 1, 2) /* start of 3-byte sequence, any continuation may follow */ \
        F( 4,  lb3_ed, 3, 0xed,   1, 0, 0) /* start of 3-byte sequence, next byte must be 0x80-0x9f */ \
        F( 5,  lb4_f0, 4, 0xf0,   1, 0, 0) /* start of 4-byte sequence, next byte must be 0x90-0xbf */ \
        F( 6,  lb4,    4, 0xf1,   3, 0, 0) /* start of 4-byte sequence, any continuation may follow */ \
        F( 7,  lb4_f4, 4, 0xf4,   1, 0, 0) /* start of 4-byte sequence, next byte must be 0x80-0x8f */ \
        F( 8,  cb3_f4, 3, 0x80,  16, 0, 0) /* 3rd-to-last continuation, follows 0xf4                */ \
        F( 9,  cb3,    3, 0x80,  64, 0, 0) /* 3rd-to-last continuation, follows 0xf1-0xf3           */ \
        F(10,  cb3_f0, 3, 0x90,  48, 0, 0) /* 3rd-to-last continuation, follows 0xf0                */ \
        F(11,  cb2_ed, 2, 0x80,  32, 0, 0) /* 2nd-to-last continuation, follows 0xed                */ \
        F(12,  cb2,    2, 0x80,  64, 0, 0) /* 2nd-to-last continuation, follows 0xe1-0xec,0xee-0xef */ \
        F(13,  cb2_e0, 2, 0xa0,  32, 0, 0) /* 2nd-to-last continuation, follows 0xe0                */ \
        F(14,  cb1,    1, 0x80,  64, 0, 0) /* last continuation, common to all multi-byte sequences */ \
        F(15,  ini,    0,    0,   0, 0 ,0) /* initial state, not set as a bit lookup table elements */

/**
 * @brief UTF-8 parser state enumeration.
 */
fixed_enum(utf8_st8, uint8_t) {
	#define F(n,m,...) utf8_##m = n,
	UTF8_PARSER_DESCRIPTOR(F)
	#undef F
};

/**
 * @brief UTF-8 parser state bit flags.
 * @param label State label (2nd column in the descriptor).
 * @return Bit flag value for the state.
 */
#define utf8_bit(label) (uint16_t)(1U << utf8_##label)

/**
 * @brief UTF-8 parser object.
 */
struct utf8 {
	uint16_t state;
	uint8_t  cache[5];
	uint8_t  error;
};

/**
 * @brief UTF-8 parser object RAII initializer.
 * @return A UTF-8 parser object by value.
 */
static const_inline
struct utf8 utf8 (void)
{
	return (struct utf8) {
		.state = utf8_bit(ini),
		.cache = {0,0,0,0,0},
		.error = 0,
	};
}

/**
 * @brief Clear a UTF-8 parser object.
 * @param u8p Pointer to parser object.
 */
nonnull_in()
static force_inline void
utf8_reset (struct utf8 *const u8p)
{
	*u8p = utf8();
}

/**
 * @brief Consume up to 4 bytes of a UTF-8 sequence and update the
 *        parser state accordingly.
 *
 * @param u8p Pointer to parser object.
 * @param ptr Pointer to the next byte to parse.
 * @return A pointer to the byte immediately following the consumed
 *         sequence, or a pointer to the first invalid byte if one
 *         is encountered, or the original pointer if the pointed-to
 *         byte is a null terminator.
 */
nonnull_out
extern uint8_t const *
utf8_next (struct utf8   *u8p,
           uint8_t const *ptr) nonnull_in();

nonnull_in() nonnull_out
static force_inline char const *
utf8_result (struct utf8 const *u8p)
{
	uint8_t i = u8p->cache[0];
	if (u8p->cache[i])
		i = 1;
	return (char const *)&u8p->cache[i];
}

nonnull_in()
static force_inline size_t
utf8_size (struct utf8 const *u8p)
{
	return u8p->cache[0];
}

nonnull_in()
static const_inline bool
utf8_expects_leading_byte (struct utf8 const *const u8p)
{
	return u8p->state & (utf8_bit(asc) | utf8_bit(cb1) | utf8_bit(ini));
}

#endif /* LIBCANTH_SRC_UTF8_H_ */
