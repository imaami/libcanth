/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file test-utf8.c
 *
 * @author Juuso Alasuutari
 */
#include <errno.h>
#include <inttypes.h>

#define PROGNAME "test-utf8"
#define SYNOPSIS "[[MODE-OPTION] [OPTION]... [--] ARGS...|--help]"
#define PURPOSE  "Inspect UTF-8 encoded strings"

#define OPTIONS(X)                              \
	X(boolean, help, 'h', "help",           \
	  "print this help text and exit")      \
	                                        \
	X(boolean, bytes, 'b', "bytes",         \
	  "count bytes")                        \
	                                        \
	X(boolean, chars, 'c', "chars",         \
	  "count unicode characters")           \
	                                        \
	X(boolean, join, 'j', "join",           \
	  "treat arguments as one string")      \
	                                        \
	X(boolean, skip, 's', "skip", "skip "   \
	  "invalid sequences (no placeholder)") \
	                                        \
	X(boolean, quiet, 'q', "quiet",         \
	  "report invalid UTF-8 via exit code")

#define DETAILS  "Default mode is 'check'."

#include "letopt.h"

#include "dbg.h"
#include "utf8.h"

int
main (int    c,
      char **v)
{
	struct letopt opt = letopt_init(c, v);

	if (opt.m_help)
		letopt_helpful_exit(&opt);

	bool need_nl = false;
	size_t n_codepts = 0;
	struct utf8 u8p = utf8();

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
			char const *s = utf8_result(&u8p);
			need_nl = *s != '\n';
			fputs(s, stdout);
		}

		n_codepts += n;

		if (!opt.m_join && need_nl) {
			need_nl = false;
			putchar('\n');
		}
	}

	if (opt.m_join && need_nl)
		putchar('\n');

	return letopt_fini(&opt);
}
