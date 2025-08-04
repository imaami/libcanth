/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file b26.c
 * @brief 64-bit binary De Bruijn sequences
 * @author Juuso Alasuutari
 */

#include <stdlib.h>

#include "b26.h"
#include "file.h"

static int
b26_cmp (void const *pkey,
         void const *pseq)
{
#if UINTPTR_MAX >= UINT64_MAX
	uint64_t key = (uint64_t)pkey;
#else
	uint64_t key = *(uint64_t const *)pkey;
#endif
	uint64_t seq = *(uint64_t const *)pseq;
	return (key > seq) - (key < seq);
}

static int64_t
b26_find (uint64_t const        seq,
          uint64_t const *const arr)
{
	if (!arr)
		return -1;

#if UINTPTR_MAX >= UINT64_MAX
	void const *k = (void const *)seq;
#else
	void const *k = (void const *)&seq;
#endif
	uint64_t const *ret = bsearch(k, arr, (size_t)67108864U,
	                              sizeof *arr, b26_cmp);
	if (!ret)
		return -1;

	return (int64_t)(ptrdiff_t)(ret - arr);
}

uint64_t
is_b26 (uint64_t  const x,
        unsigned *const rot)
{
	uint64_t y = x << 6U | x >> (64U - 6U);
	uint64_t m = 0U;
	unsigned r = 0U;
	unsigned n = 0U;

	do {
		y >>= 1U;
		uint64_t b = UINT64_C(1) << (y & 63U);
		uint64_t m_ = m;
		m |= b;
		if (m == m_)
			return 0U;
		++n;
		if (b == 1U)
			r = n & 63U;
	} while (n < 6U);

	y = x;
	do {
		y >>= 1U;
		uint64_t b = UINT64_C(1) << (y & 63U);
		uint64_t m_ = m;
		m |= b;
		if (m == m_)
			return 0U;
		++n;
		if (b == 1U)
			r = n & 63U;
	} while (m != UINT64_MAX);

	*rot = r;
	return ror64(x, r);
}

uint64_t
b26_identify (uint64_t              x,
              struct b26_id *const  id,
              uint64_t const *const arr)
{
	unsigned rot = 0;
	x = is_b26(x, &rot);
	if (x) {
		int64_t i = b26_find(x, arr);
		if (i < 0)
			return 0U;
		*id = (struct b26_id){
			.rot = rot,
			.idx = (uint32_t)i,
		};
	}
	return x;
}

static bool
b26_array_is_valid (uint64_t const *const arr)
{
	uint64_t prev = 0U;
	uint64_t i = 0U;
	do {
		uint64_t seq = arr[i];
		unsigned rot;
		if (seq <= prev || seq != is_b26(seq, &rot))
			return false;
	} while (++i < 67108864U);
	return true;
}

uint64_t *
b26_load (char const *const path,
          bool        const validate)
{
	struct file_in f = file_in(path, nullptr,
	                           67108864U * 8U,
	                           FILE_IN_EXACT);
	uint64_t *a = (uint64_t *)(void *)f.data;

	if (!file_in_error(&f) &&
	    (!validate || b26_array_is_valid(a)))
		return a;

	file_in_fini(&f);
	return nullptr;
}
