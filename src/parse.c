/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file parse.c
 * @brief String parsing
 * @author Juuso Alasuutari
 */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parse.h"

struct parsed
parse_u64 (char *const str)
{
	struct parsed p = {
		.u64 = 0U,
		.err = !str ? EFAULT
		            : !*str ? ENODATA : 0
	};

	if (!p.err) {
		errno = 0;
		char *s = str;
		p.i64 = _Generic(
			p.i64, long: strtol
			, long long: strtoll
		)(str, &s, 0);
		p.err = errno;

		if (p.err == ERANGE &&
		    p.i64 == _Generic(
			p.i64, long: LONG_MAX
			, long long: LLONG_MAX)
		) {
			errno = 0;
			s = str;
			p.u64 = _Generic(
				p.u64, unsigned long: strtoul
				, unsigned long long: strtoull
			)(str, &s, 0);
			p.err = errno;
		}

		if (!p.err && *s)
			p.err = EINVAL;
	}

	return p;
}

bool
parsed_ok (struct parsed const par,
           char const   *const msg)
{
	bool const ret = !par.err;
	if (!ret) {
		char const sep[] = ": ";
		char const *const m = msg ?: "";
		char const *const e = strerror(par.err) ?: "";
		(void)fprintf(stderr, "%s%s%s\n", m,
		              &sep[(unsigned)(!*m | !*e) << 1U], e);
	}
	return ret;
}
