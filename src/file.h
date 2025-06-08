/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file file.h
 *
 * @author Juuso Alasuutari
 */
#ifndef LIBCANTH_SRC_FILE_H_
#define LIBCANTH_SRC_FILE_H_

#include <stddef.h>

#include "util.h"

struct file_in {
	unsigned char *data;
	size_t         size;
	unsigned       flags;
	int            error;
};

enum file_in_flags {
	FILE_IN_TEXT  = 1U << 0U,
	FILE_IN_EXACT = 1U << 1U,
	FILE_IN_ALLOC = 1U << 2U,
};

extern struct file_in
file_in (char const *path,
         void       *dest,
         size_t      size,
         unsigned    flags);

extern void
file_in_fini (struct file_in *f);

static force_inline int
file_in_error (struct file_in const *f)
{
	return f && !f->data ? f->error : 0;
}

static force_inline char const *
file_in_text (struct file_in const *f)
{
	return f && f->data ? (char const *)f->data : "";
}

#endif /* LIBCANTH_SRC_FILE_H_ */
