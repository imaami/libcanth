/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file dbg.c
 *
 * @author Juuso Alasuutari
 */
#ifndef NDEBUG
#include <errno.h>
#include <stdlib.h>

#include "dbg.h"

nonnull_in()
char *
dbg_mkdtemp_ (char *tmpl)
{
	char buf[1024];

	int len = snprintf(&buf[1], sizeof(buf) - 1U, "%s", tmpl);
	if (len >= 0) {
		if (len < (int)sizeof(buf) - 2)
			++len;
		else
			len = (int)sizeof(buf) - 2;

		buf[0]     = '"';
		buf[len++] = '"';
		buf[len]   = '\0';
	} else {
		buf[0] = '\0';
	}

	errno = 0;
	char *ret = mkdtemp(tmpl);
	int ec = errno;
	char const *const q = &"\""[!ret];

	pr_dbg_("mkdtemp(%s) -> %s%s%s", &buf[0], q, ret, q);

	errno = ec;
	return ret;
}
#endif /* NDEBUG */
