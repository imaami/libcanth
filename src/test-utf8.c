/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file test-utf8.c
 *
 * @author Juuso Alasuutari
 */
#include <errno.h>
#include <inttypes.h>

#define PROGNAME "test-utf8"
#define SYNOPSIS "[OPTION]... [--] [STRING]..."
#define PURPOSE  "Inspect UTF-8 encoded strings"

#define OPTIONS(X)                              \
	X(boolean, help, 'h', "help",           \
	  "print this help text and exit")      \
	                                        \
	X(boolean, bytes, 'b', "bytes",         \
	  "count bytes inside valid UTF-8")     \
	                                        \
	X(boolean, chars, 'c', "chars",         \
	  "count valid unicode characters")     \
	                                        \
	X(boolean, join, 'j', "join",           \
	  "treat arguments as one string")      \
	                                        \
	X(boolean, skip, 's', "skip",           \
	  "skip invalid UTF-8, don't replace")  \
	                                        \
	X(boolean, quiet, 'q', "quiet",         \
	  "report invalid UTF-8 via exit code")

#define DETAILS \
 "The default behaviour without option arguments is to\n" \
 "print input strings on different lines, substituting\n" \
 "the replacement symbol U+FFFD for invalid sequences."

#include "letopt.h"

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
	size_t buf_sz = opt.m_join ? sz.sum : sz.max;
	buf_sz = saturated_add_uz(
		saturated_add_uz(buf_sz, 1U),
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

	size_t n_codepts = 0;
	struct utf8 u8p = utf8();
	char *out = buf;

	for (int i = 0; i < letopt_nargs(&opt); ++i) {
		size_t n = 0;
		uint8_t const *p = (uint8_t const *)letopt_arg(&opt, i);
		for (uint8_t const *q = p; *q; q = p) {
			p = utf8_next(&u8p, q);
			if (u8p.error) {
				if (!*p)
					break;
				if (p == q && utf8_expects_leading_byte(&u8p))
					++p;
				utf8_reset(&u8p);
				continue;
			}

			++n;

			for (char const *s = utf8_result(&u8p); *s;) {
				*out++ = *s++;
			}
		}

		n_codepts += n;

		if (!opt.m_join) {
			*out = '\0';
			(void)fputs(buf, stdout);
			if (out == buf || *(out - 1) != '\n')
				(void)putchar('\n');
			out = buf;
		}
	}

	if (opt.m_join) {
		*out = '\0';
		(void)fputs(buf, stdout);
		if (out == buf || *(out - 1) != '\n')
			(void)putchar('\n');
	}

	free(buf);
	return letopt_fini(&opt);
}
