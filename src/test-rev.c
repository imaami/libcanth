/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file test-rev.c
 *
 * @author Juuso Alasuutari
 */
#include <inttypes.h>
#include <stdio.h>

#include "parse.h"
#include "rev.h"

int
main (int    c,
      char **v)
{
	for (int i = 0; ++i < c;) {
		struct parsed p = parse_u64(v[i]);
		if (parsed_ok(p, v[i]))
			(void)printf("%016" PRIx64 " "
			             "%016" PRIx64 "\n",
			             p.u64, rev64(p.u64));

	}
}
