/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file compat.h
 *
 * @author Juuso Alasuutari
 */
#ifndef LIBCANTH_SRC_COMPAT_H_
#define LIBCANTH_SRC_COMPAT_H_

#include "compiler.h"

#ifndef __cplusplus

# if (__STDC_VERSION__ < 202000L) \
  || clang_older_than_version(16) \
  || gcc_older_than_version(13,1) \
  || defined(__INTELLISENSE__)
#  include <stddef.h>
#  define nullptr NULL
# endif /* __STDC_VERSION__ < 202000L || clang < 16 || gcc < 13.1 || __INTELLISENSE__ */

# if clang_older_than_version(8)  \
  || gcc_older_than_version(13,1) \
  || defined(__INTELLISENSE__)
#  define fixed_enum(name, T) enum name
# elif clang_older_than_version(18)
#  include "ligma.h"
#  define fixed_enum(name, T)   \
    diag_clang(push)            \
    diag_clang(ignored          \
      "-Wfixed-enum-extension") \
    enum name : T               \
    diag_clang(pop)
# else
#  define fixed_enum(name, T) enum name : T
# endif /* clang < 8 || gcc < 13.1 || __INTELLISENSE__ */

# if (__STDC_VERSION__ < 202000L) \
  || clang_older_than_version(19) \
  || gcc_older_than_version(13,1) \
  || defined(__INTELLISENSE__)
#  define constexpr
# endif /* __STDC_VERSION__ < 202000L || clang < 19 || gcc < 13.1 || __INTELLISENSE__ */
#endif /* !__cplusplus */

#ifndef __has_builtin
# define __has_builtin(x) 0
#endif /* !__has_builtin */

#endif /* LIBCANTH_SRC_COMPAT_H_ */
