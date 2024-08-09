/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file dstr.c
 *
 * @author Juuso Alasuutari
 */
#include <errno.h>

#include "dstr.h"

/**
 * @brief Validate input arguments of functions with assignment semantics.
 *
 * @param[in]     src The string pointer argument passed by the caller.
 * @param[in,out] len Pointer to the length argument passed by the caller.
 *
 * @return 0 on success, otherwise an error code.
 *
 * @retval EFAULT       The input string is NULL.
 * @retval EINVAL       The input string is empty.
 * @retval ENAMETOOLONG The size of the input string, including the null
 *                      terminator, cannot be expressed as `unsigned int`.
 */
static force_inline int
dstr_validate_input (char const *src,
                     size_t     *len)
{
	/* src was passed by the caller and therefore inherently sus. */
	if (!src)
		return EFAULT;

	/* len is known to be an internal pointer to the length variable. */
	size_t n = *len;

	/* If `*len` is nonzero, the responsibility of measuring the string
	 * is on the caller, in which case we just verify that `src` is not
	 * the empty string. If `*len` is zero, then the caller wants us to
	 * call `strlen()` on it, which doubles as the empty string check.
	 */
	if (n ? !*src : !(n = __builtin_strlen(src)))
		return EINVAL;

	/* Ensure that there's space for the terminator, and that on 64-bit
	 * systems the length is not too large to fit in an unsigned int.
	 */
	if (n > UINT_MAX - 1U)
		return ENAMETOOLONG;

	*len = n;
	return 0;
}

bool
dstr_set (dstr       *dest,
          char const *src,
          size_t      len,
          int        *err)
{
	int e = dstr_validate_input(src, &len);
	if (e) {
		*err = e;
		return false;
	}

	unsigned int size = dstr_is_pointer(dest) ? dest->size : 0U;
	unsigned int len_ = (unsigned int)len;
	char *ptr = dest->ptr;

	if (len < sizeof dest->arr) {
		if (size)
			free(ptr);

		ptr = &dest->arr[0];
		size = sizeof dest->arr - 1U;

	} else {
		if (len_ < size) {
			size = dest->len > len_ ? dest->len : len_;

		} else {
			char *p = size ? ptr : nullptr;
			size = len_ + 1U;
			ptr = realloc(p, size);
			if (!ptr) {
				*err = errno;
				return false;
			}
			dest->ptr = ptr;
			dest->size = size--;
		}
	}

	__builtin_strncpy(ptr, src, size);
	ptr[size] = '\0';
	dest->len = len_;
	return true;
}

void
dstr_move (dstr *dest,
           dstr *src)
{
	if (dest == src)
		return;

	if (dstr_owns_memory(dest))
		free(dest->ptr);

	if (dstr_is_array(src)) {
		__builtin_strncpy(dest->arr, src->arr, sizeof dest->arr - 1U);
		dest->arr[sizeof dest->arr - 1U] = '\0';
	} else {
		dest->ptr = src->ptr;
		dest->size = src->size;
	}

	dest->len = src->len;
	dstr_init(src);
}
