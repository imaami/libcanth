/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file cc.c
 *
 * @author Juuso Alasuutari
 */
#include "version.h"

const_nonnull char const *
canth_c_version (void)
{
#ifdef __clang__
	return "clang " stringify(__clang_major__)
	            "." stringify(__clang_minor__)
	            "." stringify(__clang_patchlevel__);
#else
	return "gcc " stringify(__GNUC__)
	          "." stringify(__GNUC_MINOR__)
	          "." stringify(__GNUC_PATCHLEVEL__);
#endif
}
