/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file utf8.c
 *
 * @author Juuso Alasuutari
 */
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
	size_t   n_u8;
	size_t   n_uc;
	uint16_t state;
	int16_t  error;
	uint8_t  cache[4];
};

static struct utf8
utf8_parser (void);

static int
utf8_check (struct utf8 *const u8p,
            char const *str);

#if 0
static void
utf8_flowchart (void);
#endif

int
main (int    c,
      char **v)
{
	//utf8_flowchart();
	struct utf8 u8p = utf8_parser();
	for (int i = 0; ++i < c;) {
		int e = utf8_check(&u8p, v[i]);
		if (e)
			pr_errno(e, "utf8_check");
	}
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
        F(15,  ini,       0,   0, 0 ,0) /* initial state, has no role as a lookup table bit flag */

#define utf8_enum(m)    utf8_##m
#define utf8_flag(m)    utf8_##m##_flag

#define make_enum(n,m,...)      utf8_enum(m) = n,
#define make_flag(n,m,...)      utf8_flag(m) = 1U << n,

fixed_enum(utf8_state, uint8_t) {UTF8_PARSER_DESCRIPTOR(make_enum )};
fixed_enum(utf8_flag, uint16_t) {UTF8_PARSER_DESCRIPTOR(make_flag )};

#undef make_enum
#undef make_flag

#if 0
#define make_range(n,m,...)     [n] = {__VA_ARGS__},
constexpr static
const uint8_t utf8_range[][4] = {UTF8_PARSER_DESCRIPTOR(make_range)};
#undef make_range
#endif

#define two(x)          x,x
#define three(x)        x,x,x
#define eight(x)        x,x,x,x,x,x,x,x
#define twelve(x)       eight(x),x,x,x,x
#define sixteen(x)      eight(x),eight(x)
#define thirty(x)       sixteen(x),twelve(x),x,x
#define thirty2(x)      sixteen(x),sixteen(x)
#define onehundred27(x) thirty2(x),thirty2(x),thirty2(x),thirty(x),x

constexpr static const uint16_t utf8_lut[256] = {
	/* 0x00 */
	0,

	/* 0x01-0x7f */
	onehundred27(utf8_asc_flag),

	sixteen(utf8_cb1_flag|utf8_cb2_flag|utf8_cb3_flag|utf8_cb2_ed_flag|utf8_cb3_f4_flag),
	sixteen(utf8_cb1_flag|utf8_cb2_flag|utf8_cb3_flag|utf8_cb2_ed_flag|utf8_cb3_f0_flag),
	thirty2(utf8_cb1_flag|utf8_cb2_flag|utf8_cb3_flag|utf8_cb2_e0_flag|utf8_cb3_f0_flag),

	/* 0xc0-0xc1 */
	0, 0,

	/* 0xc2-0xdf */
	thirty(utf8_lb2_flag),

	/* 0xe0 */
	utf8_lb3_e0_flag,
	/* 0xe1-0xec */
	twelve(utf8_lb3_flag),
	/* 0xed */
	utf8_lb3_ed_flag,
	/* 0xee-0xef */
	two(utf8_lb3_flag),

	/* 0xf0 */
	utf8_lb4_f0_flag,
	/* 0xf1-0xf3 */
	three(utf8_lb4_flag),
	/* 0xf4 */
	utf8_lb4_f4_flag
};

#undef onehundred27
#undef thirty
#undef thirty2
#undef sixteen
#undef twelve
#undef eight
#undef three
#undef two

constexpr static const uint16_t utf8_state_dst[16] = {
	[utf8_asc    ] = utf8_asc_flag
	               | utf8_lb2_flag
	               | utf8_lb3_e0_flag
	               | utf8_lb3_flag
	               | utf8_lb3_ed_flag
	               | utf8_lb4_f0_flag
	               | utf8_lb4_flag
	               | utf8_lb4_f4_flag,
	[utf8_lb2    ] = utf8_cb1_flag,
	[utf8_lb3_e0 ] = utf8_cb2_e0_flag,
	[utf8_lb3    ] = utf8_cb2_flag,
	[utf8_lb3_ed ] = utf8_cb2_ed_flag,
	[utf8_lb4_f0 ] = utf8_cb3_f0_flag,
	[utf8_lb4    ] = utf8_cb3_flag,
	[utf8_lb4_f4 ] = utf8_cb3_f4_flag,
	[utf8_cb3_f4 ] = utf8_cb2_flag,
	[utf8_cb3    ] = utf8_cb2_flag,
	[utf8_cb3_f0 ] = utf8_cb2_flag,
	[utf8_cb2_ed ] = utf8_cb1_flag,
	[utf8_cb2    ] = utf8_cb1_flag,
	[utf8_cb2_e0 ] = utf8_cb1_flag,
	[utf8_cb1    ] = utf8_asc_flag
	               | utf8_lb2_flag
	               | utf8_lb3_e0_flag
	               | utf8_lb3_flag
	               | utf8_lb3_ed_flag
	               | utf8_lb4_f0_flag
	               | utf8_lb4_flag
	               | utf8_lb4_f4_flag,
	[utf8_ini    ] = utf8_asc_flag
	               | utf8_lb2_flag
	               | utf8_lb3_e0_flag
	               | utf8_lb3_flag
	               | utf8_lb3_ed_flag
	               | utf8_lb4_f0_flag
	               | utf8_lb4_flag
	               | utf8_lb4_f4_flag,
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
	     src < sizeof utf8_state_dst /
	           sizeof utf8_state_dst[0]; ++src) {
		if (!(utf8_range[src][1] +
		      utf8_range[src][3]))
			continue;
		for (unsigned flags = utf8_state_dst[src],
		     dst = 0; flags; flags >>= 1U, ++dst) {
			if (!(flags & 1U))
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
		.n_u8  = 0,
		.n_uc  = 0,
		.state = utf8_ini_flag,
		.error = 0,
		.cache = {0}
	};
}

static int
utf8_check (struct utf8 *const u8p,
            char const *str)
{
	unsigned char const *p = (void const *)str;

	if (u8p->state) {
		for (; *p; ++p) {
			u8p->state = utf8_state_dst[
				__builtin_ctz(u8p->state)] & utf8_lut[*p];
			if (!u8p->state)
				return EILSEQ;
			putchar(*p);
		}
	}

	return 0;
}

#if clang_older_than_version(19) \
 && clang_newer_than_version(11)
diag_clang(pop)
#endif /* clang < 19 && clang > 11 */
