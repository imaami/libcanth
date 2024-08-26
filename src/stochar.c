/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file stochastic-unicode.c
 *
 * @author Juuso Alasuutari
 */
#include <errno.h>
#include <inttypes.h>

#define PROGNAME "stochar"
#define SYNOPSIS "[OPTION]... [--] [STRING]..."
#define PURPOSE  "Converts UTF-8 to stochastic unicode."

#define OPTIONS(X)                                    \
	X(boolean, help, 'h', "help",                 \
	  "print this help text and exit")            \
	                                              \
	X(boolean, replace, 'r', "replace",           \
	  "substitute U+FFFD for bad UTF-8")          \
	                                              \
	X(boolean, separate, 's', "separate",         \
	  "don't concatenate input strings")

#define DETAILS                                       \
 "Given no options, input strings are concatenated\n" \
 "and invalid UTF-8 sequences silently ignored.\n\n"  \
 "Encoder output should be considered binary data,\n" \
 "it is not printable in any existing encoding."

#include "letopt.h"

#undef DETAILS
#undef OPTIONS
#undef PURPOSE
#undef SYNOPSIS
#undef PROGNAME

#include "dbg.h"
#include "utf8.h"

static const_inline size_t
saturated_add_uz (size_t a,
                  size_t b)
{
	if ((a += b) < b)
		a = SIZE_MAX;
	return a;
}

static struct uz2 {
	size_t max;
	size_t sum;
} arg_sizes (struct letopt const *opt)
{
	struct uz2 ret = {0};
	for (int i = 0; i < letopt_nargs(opt); ++i) {
		size_t n = strlen(letopt_arg(opt, i));
		ret.sum = saturated_add_uz(ret.sum, n);
		if (n > ret.max)
			ret.max = n;
	}
	return ret;
}

union code_point {
	uint32_t u32;
	struct {
		uint32_t b0 :  7;
		uint32_t p0 : 25;
	} utf32_0;
	struct {
		uint32_t b1 :  6;
		uint32_t b0 :  5;
		uint32_t p0 : 21;
	} utf32_1;
	struct {
		uint32_t b2 :  6;
		uint32_t b1 :  6;
		uint32_t b0 :  4;
		uint32_t p0 : 16;
	} utf32_2;
	struct {
		uint32_t b3 :  6;
		uint32_t b2 :  6;
		uint32_t b1 :  6;
		uint32_t b0 :  3;
		uint32_t p0 : 11;
	} utf32_3;

	uint8_t u8[4];
	struct {
		uint32_t b0 :  7;
		uint32_t p1 : 25;
	} utf8_0;
	struct {
		uint32_t b0 :  5;
		uint32_t p0 :  3;
		uint32_t b1 :  6;
		uint32_t p1 : 18;
	} utf8_1;
	struct {
		uint32_t b0 :  4;
		uint32_t p0 :  4;
		uint32_t b1 :  6;
		uint32_t p1 :  2;
		uint32_t b2 :  6;
		uint32_t p2 : 10;
	} utf8_2;
	struct {
		uint32_t b0 :  3;
		uint32_t p0 :  5;
		uint32_t b1 :  6;
		uint32_t p1 :  2;
		uint32_t b2 :  6;
		uint32_t p2 :  2;
		uint32_t b3 :  6;
		uint32_t p3 :  2;
	} utf8_3;

	unsigned char data[4];
	char          str[4];
};

_Static_assert(sizeof(union code_point) * CHAR_BIT == 32U,"");

#define BAD_CODE_POINT (constexpr union code_point){.u32 = UINT32_MAX}

static const_inline bool
code_point_error (const union code_point c)
{
	return c.u32 == BAD_CODE_POINT.u32;
}

static const_inline union code_point
utf8_to_utf32 (const union code_point c,
               const size_t           n)
{
	switch (n) {
	case 1:
		return (union code_point){
			.utf32_0.b0 = c.utf8_0.b0,
			.utf32_0.p0 = 0
		};
	case 2:
		return (union code_point){
			.utf32_1.b1 = c.utf8_1.b1,
			.utf32_1.b0 = c.utf8_1.b0,
			.utf32_1.p0 = 0
		};
	case 3:
		return (union code_point){
			.utf32_2.b2 = c.utf8_2.b2,
			.utf32_2.b1 = c.utf8_2.b1,
			.utf32_2.b0 = c.utf8_2.b0,
			.utf32_2.p0 = 0
		};
	case 4:
		return (union code_point){
			.utf32_3.b3 = c.utf8_3.b3,
			.utf32_3.b2 = c.utf8_3.b2,
			.utf32_3.b1 = c.utf8_3.b1,
			.utf32_3.b0 = c.utf8_3.b0,
			.utf32_3.p0 = 0
		};
	default:
		break;
	}

	return BAD_CODE_POINT;
}

static union code_point
utf8_code_point (struct utf8 *const u8p)
{
	char const *const s = utf8_result(u8p);
	return utf8_to_utf32(
		(union code_point){
			.str = {s[0], s[1], s[2], s[3]}
		}, utf8_size(u8p)
	);
}

int
main (int    c,
      char **v)
{
	struct letopt opt = letopt_init(c, v);

	if (letopt_nargs(&opt) < 1 || opt.m_help)
		letopt_helpful_exit(&opt);

	struct uz2 sz = arg_sizes(&opt);
	size_t buf_sz = opt.m_separate ? sz.max : sz.sum;
	buf_sz = saturated_add_uz(
		saturated_add_uz(buf_sz, 2U),
		saturated_add_uz(buf_sz, buf_sz));

	if (buf_sz == SIZE_MAX) {
		pr_err_("%s", strerror(EOVERFLOW));
		(void)letopt_fini(&opt);
		return EXIT_FAILURE;
	}

	char *buf = malloc(buf_sz);
	if (!buf) {
		pr_errno_(errno, "malloc(%zu)", buf_sz);
		(void)letopt_fini(&opt);
		return EXIT_FAILURE;
	}

	struct utf8 u8p = utf8();
	uint64_t n[] = {0,0,0,0};
	char *out = buf;

	for (int i = 0; i < letopt_nargs(&opt); ++i) {
		uint8_t const *p = (uint8_t const *)letopt_arg(&opt, i);
		for (uint8_t const *q = p; *q; q = p) {
			p = utf8_parse_next_code_point(&u8p, q);
			if (u8p.error) {
				if (opt.m_replace && (*p || opt.m_separate)) {
					*out++ = (char)(unsigned char)0xefU;
					*out++ = (char)(unsigned char)0xbfU;
					*out++ = (char)(unsigned char)0xbdU;
				}
				if (!*p)
					break;
				if (p == q && utf8_expects_leading_byte(&u8p))
					++p;
				utf8_reset(&u8p);
				continue;
			}

			union code_point c = utf8_code_point(&u8p);
		}

		if (opt.m_separate) {
			if (out == buf || *(out - 1) != '\n')
				*out++ = '\n';
			*out = '\0';
			out = buf;
			(void)printf("%" PRIu64 "\t%" PRIu64 "\t%" PRIu64
			             "\t%" PRIu64 "\t%" PRIu64 "\t%s",
			             n[0], n[1], n[2], n[3], n[0] +
			             2U*n[1] + 3U*n[2] + 4U*n[3], buf);

			__builtin_memset(&n[0], 0, sizeof n);
			utf8_reset(&u8p);
		}
	}

	if (!opt.m_separate) {
		if (out == buf || *(out - 1) != '\n')
			*out++ = '\n';
		*out = '\0';
		(void)printf("%" PRIu64 "\t%" PRIu64 "\t%" PRIu64
		             "\t%" PRIu64 "\t%" PRIu64 "\t%s",
		             n[0], n[1], n[2], n[3], n[0] +
		             2U*n[1] + 3U*n[2] + 4U*n[3], buf);
	}

	free(buf);
	return letopt_fini(&opt);
}
