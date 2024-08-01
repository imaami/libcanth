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
		uint8_t const *p = (uint8_t *)v[i];

		while (*p) {
			p = utf8_next(&u8p, p);
			if (u8p.error)
				break;

			++n_codepts;

			pr_out_("%s%s", &" "[!separate], utf8_result(&u8p));
			separate ^= separate;
		}

		separate = u8p.error != EAGAIN;
		if (separate) {
			if (u8p.error == EILSEQ)
				pr_out_("\033[38;5;196m?\033[m");
			utf8_reset(&u8p);
		}
	}

	pr_out("%s%zu unicode characters", &"\n"[!n_codepts], n_codepts);
}
