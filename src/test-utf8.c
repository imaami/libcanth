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
	X(boolean, join, 'j', "join",           \
	  "treat arguments as one string")      \
	                                        \
	X(string, mode, 'm', "mode", "one "     \
	  "of: check, length, quiet, strlen",   \
	  "MODE", "check")                      \
	                                        \
	X(boolean, check, 'c', "check",         \
	  "synonym for --mode=check")           \
	                                        \
	X(boolean, length, 'l', "length",       \
	  "synonym for --mode=length")          \
	                                        \
	X(boolean, quiet, 'q', "quiet",         \
	  "synonym for --mode=quiet")           \
	                                        \
	X(boolean, strlen, 's', "strlen",       \
	  "synonym for --mode=strlen")

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

	size_t n_codepts = 0;
	bool wordsep = false;
	struct utf8 u8p = utf8();

	for (int i = 0; ++i < c;) {
		size_t n = 0;
		uint8_t const *p = (uint8_t *)v[i];
		for (uint8_t const *q = p; *q; q = p) {
			p = utf8_next(&u8p, q);
			if (u8p.error) {
				if (!*p)
					break;
				if (p == q && utf8_expects_leading_byte(&u8p))
					++p;
				utf8_reset(&u8p);
				wordsep = n_codepts > 0;
				continue;
			}
			pr_out_("%s%s", &" "[!(wordsep && !n)], utf8_result(&u8p));
			n++;
			wordsep = false;
		}

		n_codepts += n;
		if (n && !u8p.error)
			wordsep = true;
	}

	pr_out("%s%zu unicode characters", &"\n"[!n_codepts], n_codepts);
	return letopt_fini(&opt);
}
