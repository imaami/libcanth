/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file compat_dbg.h
 *
 * @author Juuso Alasuutari
 */
#ifndef LIBCANTH_SRC_COMPAT_DBG_H_
#define LIBCANTH_SRC_COMPAT_DBG_H_

#include "ligma.h"

diag_clang(push)
diag_clang(ignored "-Wgnu-zero-variadic-macro-arguments")

#define pr_(...)            pr__(stderr, ## __VA_ARGS__)
#define pr_out_(...)        pr__(stdout, ## __VA_ARGS__)
#define pr__strerror(fn, e, fmt, ...) do {         \
        pr_##fn(fmt "%s%s", ## __VA_ARGS__,        \
                &"\0: "[!!(fmt)[0]], strerror(e)); \
} while (0)
#define pr_out(fmt, ...)    pr_out_(fmt "\n", ## __VA_ARGS__)
#define pr_xwrn_(fmt, ...)  pr_("warning: " fmt "\n", ## __VA_ARGS__)
#define pr_xerr_(fmt, ...)  pr_(  "error: " fmt "\n", ## __VA_ARGS__)
#define pr_xerrno_(e, ...)  pr__strerror(xerr_, (e), ## __VA_ARGS__)
#define pr_xwrrno_(e, ...)  pr__strerror(xwrn_, (e), ## __VA_ARGS__)
#define pr_xwrn(fmt, ...)   pr_xwrn_("%s: " fmt, __func__, ## __VA_ARGS__)
#define pr_xerr(fmt, ...)   pr_xerr_("%s: " fmt, __func__, ## __VA_ARGS__)
#define pr_xerrno(e, ...)   pr__strerror(xerr, (e), ## __VA_ARGS__)
#define pr_xwrrno(e, ...)   pr__strerror(xwrn, (e), ## __VA_ARGS__)

#ifndef NDEBUG
# define pr_xdbg_(fmt, ...) pr_(       fmt "\n", ## __VA_ARGS__)
# define pr_xdbg(fmt, ...)  pr_("%s:%d:%s: " fmt "\n", __FILE__, \
                                __LINE__, __func__, ## __VA_ARGS__)
#endif /* !NDEBUG */

diag_clang(pop)

#endif /* LIBCANTH_SRC_COMPAT_DBG_H_ */
