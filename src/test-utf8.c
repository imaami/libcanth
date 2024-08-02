/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file test-utf8.c
 *
 * @author Juuso Alasuutari
 */
#include <errno.h>
#include <inttypes.h>

#include "dbg.h"
#include "utf8.h"

int
main (int    c,
      char **v)
{
	size_t n_codepts = 0;
	bool separate = false;
	struct utf8 u8p = utf8();

	for (int i = 0; ++i < c;) {
		size_t n = 0;
		uint8_t const *q = (uint8_t *)v[i];
		for (uint8_t const *p; *q; q = p) {
			p = utf8_next(&u8p, q);
			if (u8p.error) {
				if (!*p)
					break;
				if (p == q && utf8_expects_leading_byte(&u8p))
					++p;
				utf8_reset(&u8p);
				separate = n_codepts > 0;
				continue;
			}
			pr_out_("%s%s", &" "[!(separate && !n)], utf8_result(&u8p));
			n++;
			separate = false;
		}

		n_codepts += n;
		if (n && !u8p.error)
			separate = true;
	}

	pr_out("%s%zu unicode characters", &"\n"[!n_codepts], n_codepts);
}
