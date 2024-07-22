/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file version.h
 *
 * @author Juuso Alasuutari
 */
#ifndef LIBCANTH_SRC_VERSION_H_
#define LIBCANTH_SRC_VERSION_H_

#include "util.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern const_nonnull char const *canth_c_version (void);
extern const_nonnull char const *canth_cxx_version (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LIBCANTH_SRC_VERSION_H_ */
