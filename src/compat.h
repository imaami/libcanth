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
  || clang_older_than_version(15) \
  || gcc_older_than_version(13,1)
#  include <stdbool.h>
# endif /* __STDC_VERSION__ < 202000L || clang < 15 || gcc < 13.1 */

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

# if clang_older_than_version(16)
#  define typeof __typeof__
# endif /* clang < 16 */

# ifndef NO_VA_OPT
#  if clang_older_than_version(12) \
   || gcc_older_than_version(8)
#   define NO_VA_OPT 1
#  endif /* clang < 12 || gcc < 8 */
# endif /* !NO_VA_OPT */

# endif /* !__cplusplus */

#ifndef __has_builtin
# define __has_builtin(x) 0
#endif /* !__has_builtin */

#endif /* LIBCANTH_SRC_COMPAT_H_ */
