/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file file.c
 *
 * @author Juuso Alasuutari
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "compat.h"
#include "file.h"

/**
 * @brief Read a file into a buffer.
 *
 * `FILE_IN_EXACT` adds constraints to the input file size, while
 * `FILE_IN_TEXT` modifies the handling of the output buffer.
 *
 * The following rules apply:
 *
 * - If `FILE_IN_EXACT` is set, `size` must be non-zero and exactly
 *   match the file size.
 *
 * - If `FILE_IN_EXACT` is set and `dest` is not `nullptr`, no null
 *   terminator is appended even if `FILE_IN_TEXT` is set.
 *
 * - If `FILE_IN_TEXT` is set and `dest` is `nullptr`, the buffer is
 *   always null-terminated.
 *
 * - If `FILE_IN_TEXT` is set, `FILE_IN_EXACT` is set, and `dest` is
 *   `nullptr`, the allocated buffer will contain a null terminator.
 *
 * - If `FILE_IN_TEXT` is set, `FILE_IN_EXACT` is not set, and `dest`
 *   is not `nullptr`, a null terminator is appended if there's space
 *   left after reading the file (i.e. file is smaller than `size`).
 *
 * @param path The path to the file.
 * @param dest The buffer to read the file into.
 * @param size The size of the buffer.
 * @param flags The flags to use.
 */
struct file_in
file_in (char const *path,
         void       *dest,
         size_t      size,
         unsigned    flags)
{
	struct file_in ret = {0};

	do {
		if (size < 1U && (dest || (flags & FILE_IN_EXACT))) {
			ret.error = EINVAL;
			break;
		}

		FILE *fp = fopen(path, "rbem");
		if (!fp) {
			ret.error = errno;
			break;
		}

		if (dest)
			flags &= (unsigned)~FILE_IN_ALLOC;
		else
			flags |= FILE_IN_ALLOC;

		do {
			struct stat s = {0};
			ret.error = fileno(fp);

			ret.error = ret.error < 0 || fstat(ret.error, &s);
			if (ret.error) {
				ret.error = errno;
				break;
			}

			if ((s.st_mode & S_IFMT) != S_IFREG) {
				ret.error = ENOENT;
				break;
			}

			if (s.st_size < 0) {
				ret.error = EIO;
				break;
			}

			size_t len = (size_t)s.st_size;

			if ((flags & FILE_IN_EXACT) && size != len) {
				ret.error = EINVAL;
				break;
			}

			if (flags & FILE_IN_ALLOC) {
				dest = malloc(len + !!(flags & FILE_IN_TEXT));
				if (!dest) {
					ret.error = errno;
					break;
				}
			} else if (len > size) {
				ret.error = ENOMEM;
				break;
			}

			if (fread(dest, 1, len, fp) != len) {
				ret.error = errno;
				if (flags & FILE_IN_ALLOC) {
					free(dest);
					dest = nullptr;
				}
				break;
			}

			ret.data = dest;
			ret.size = len;
			ret.flags = flags & (FILE_IN_TEXT
			                    |FILE_IN_EXACT
			                    |FILE_IN_ALLOC);

			switch (ret.flags & (FILE_IN_TEXT | FILE_IN_ALLOC)) {
			case FILE_IN_TEXT:
				if (size > len)
					ret.data[ret.size] = 0;
				break;
			case FILE_IN_TEXT | FILE_IN_ALLOC:
				ret.data[ret.size] = 0;
				break;
			default:
				break;
			}
		} while (0);

		(void)fclose(fp);
		fp = nullptr;
	} while (0);

	return ret;
}

void
file_in_fini (struct file_in *f)
{
	if (f) {
		if (f->flags & FILE_IN_ALLOC)
			free(f->data);
		f->data = nullptr;
		f->size = 0;
		f->flags = 0;
		f->error = 0;
	}
}
