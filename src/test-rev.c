/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file test-rev.c
 *
 * @author Juuso Alasuutari
 */
#include <inttypes.h>
#include <stdio.h>

#include "parse.h"
#include "rev.h"

struct asc4 {
	char d[4];
};

__attribute__((const))
static struct asc4
bin_asc (uint4 x)
{

	union {
		char          c[35U];
		unsigned char u[35U];
	} const t = {
		"1111000010011010" "111"
		"\x04""\x05""\x06"
		"\x09""\x07""\x0d"
		"\x0a""\x0f""\x03"
		"\x08""\x0c""\x0e"
		"\x02""\x0b""\x01"
	};
	unsigned const i = t.u[
		19U + uint4_get(x)
	];
	return (struct asc4){ .d = {
		t.c[i+0U], t.c[i+1U],
		t.c[i+2U], t.c[i+3U]
	}};
}

int
main (int    c,
      char **v)
{
	for (int i = 0; ++i < c;) {
		struct parsed p = parse_u64(v[i]);
		if (!parsed_ok(p, v[i]))
			continue;

		(void)printf("%016" PRIx64 " "
		             "%016" PRIx64 "\n",
		             p.u64, rev64(p.u64));

		if (p.u64 > UINT32_MAX)
			continue;
		(void)printf("        %0"
		             "8" PRIx64 " "
		             "%08" PRIx32 "\n",
		             p.u64, rev32((uint32_t)p.u64));

		if (p.u64 > UINT16_MAX)
			continue;
		(void)printf("            %0"
		             "4" PRIx64 " "
		             "%04" PRIx16 "\n",
		             p.u64, rev16((uint16_t)p.u64));

		if (p.u64 > UINT8_MAX)
			continue;
		(void)printf("              %0"
		             "2" PRIx64 " "
		             "%02" PRIx8 "\n",
		             p.u64, rev8((uint8_t)p.u64));

		if (p.u64 > 15U)
			continue;
		(void)printf("               "
		             "%" PRIx64 " %x\n",
		             p.u64, (unsigned)uint4_get(rev4(uint4(p.u64))));

		if (p.u64 > 3U)
			continue;
		(void)printf("               "
		             "%" PRIx64 " %x\n",
		             p.u64, (unsigned)uint2_get(rev2(uint2(p.u64))));
	}
}
