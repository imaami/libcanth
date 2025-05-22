/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file libcanth.h
 *
 * @author Juuso Alasuutari
 */
#ifndef LIBCANTH_H_
#define LIBCANTH_H_

#ifndef _WIN32
# define LIBCANTH_API __attribute__((visibility("default")))
#elif defined LIBCANTH_EXPORT_API
# define LIBCANTH_API __declspec(dllexport)
#else
# define LIBCANTH_API __declspec(dllimport)
#endif

#include <libcanth-plugin.h>

#ifdef LIBCANTH_EXPORT_API
# undef LIBCANTH_EXPORT_API
#else
# undef LIBCANTH_API
#endif

#endif /* LIBCANTH_H_ */
