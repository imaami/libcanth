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

	size_t nr[4] = {0};
	size_t n_bytes = 0;
	struct utf8 u8p = utf8();
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

			nr[utf8_size(&u8p) - 1U]++;
			n_bytes += utf8_size(&u8p);

			for (char const *s = utf8_result(&u8p); *s;) {
				*out++ = *s++;
			}
		}

		if (opt.m_separate) {
			if (out == buf || *(out - 1) != '\n')
				*out++ = '\n';
			*out = '\0';
			out = buf;
			(void)printf("%zu\t%zu\t%zu\t%zu\t%zu\t%s", nr[0],
			             nr[1], nr[2], nr[3], n_bytes, buf);

			nr[0] = nr[1] = nr[2] = nr[3] = 0;
			n_bytes = 0;
			utf8_reset(&u8p);
		}
	}

	if (!opt.m_separate) {
		if (out == buf || *(out - 1) != '\n')
			*out++ = '\n';
		*out = '\0';
		(void)printf("%zu\t%zu\t%zu\t%zu\t%zu\t%s", nr[0],
		             nr[1], nr[2], nr[3], n_bytes, buf);
	}

	free(buf);
	return letopt_fini(&opt);
}
