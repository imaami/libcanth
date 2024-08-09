/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file test.c
 *
 * @author Juuso Alasuutari
 */
#include "ligma.h"

diag_apple_clang(push)
diag_apple_clang(ignored "-Wpadded")

#include <cjson/cJSON.h>

diag_apple_clang(pop)

#include "dbg.h"
#include "dstr.h"
#include "file.h"
#include "version.h"

int
main (int    argc,
      char **argv)
{
	int ret = EXIT_SUCCESS;
	pr_out("%s / %s", canth_c_version(), canth_cxx_version());

	for (int i = 0; ++i < argc;) {
		struct file_in f = file_read(argv[i]);
		int e = file_error(&f);
		if (e) {
			pr_errno(e, "file_read");
			ret = EXIT_FAILURE;
			continue;
		}
		char const *txt = file_text(&f);
		cJSON *json = cJSON_Parse(txt);
		if (!json) {
			pr_err_("parsing failed");
			ret = EXIT_FAILURE;
			char const *s = cJSON_GetErrorPtr();
			if (s) {
				char const *p = s;
				for (char const *q = p; q-- > txt &&
				     *q != '\n' && *q != '\r'; p = q);
				size_t b = (size_t)(ptrdiff_t)(s - p);
				size_t n = b + strcspn(s, "\n\r");
				if (n) {
					pr_("%.*s\n", (int)n, p);
					if (b) {
						for (; --b; ++p) {
							(void)fputc(*p == '\t'
							            ? '\t'
							            : ' ',
							            stderr);
						}
					}
					(void)fputs("^\n", stderr);
				}
			}
		} else {
			cJSON_Delete(json);
		}

		file_in_fini(&f);
	}

	return ret;
}
