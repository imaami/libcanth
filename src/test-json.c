/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file test-json.c
 *
 * @author Juuso Alasuutari
 */
#include "json.h"

int
main (int    c,
      char **v)
{
	for (int i = 0; ++i < c;) {
		json_parse(v[i]);
	}
}
