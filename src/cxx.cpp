/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file cxx.cpp
 *
 * @author Juuso Alasuutari
 */
#include "version.h"

extern "C"
const_nonnull char const *
canth_cxx_version (void)
{
#ifdef __clang__
	return "clang++ " stringify(__clang_major__)
	              "." stringify(__clang_minor__)
	              "." stringify(__clang_patchlevel__);
#else
	return "g++ " stringify(__GNUC__)
	          "." stringify(__GNUC_MINOR__)
	          "." stringify(__GNUC_PATCHLEVEL__);
#endif
}
