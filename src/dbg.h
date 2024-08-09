/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file dbg.h
 *
 * @author Juuso Alasuutari
 */
#ifndef LIBCANTH_SRC_DBG_H_
#define LIBCANTH_SRC_DBG_H_

#ifndef NDEBUG
# include <assert.h>
#endif /* !NDEBUG */
#include <stddef.h>
#include <stdio.h>
#ifdef NDEBUG
# include <stdlib.h>
#endif /* NDEBUG */
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ligma.h"
#include "util.h"

/**
 * @brief Print to stdout as-is.
 */
#define pr_out_(...)    pr_pfx_(xout_,       __VA_ARGS__)

/**
 * @brief Append a newline and print to stdout.
 */
#define pr_out(...)     pr_pfx_(xout,        __VA_ARGS__)

/**
 * @brief If `NDEBUG` was defined during compilation do nothing;
 *        otherwise append a newline and print to stderr.
 */
#define pr_dbg_(...)    pr_pfx_(xdbg_,       __VA_ARGS__)

/**
 * @brief If `NDEBUG` was defined during compilation do nothing;
 *        otherwise prepend the name of the source file, the current
 *        line number, and the name of the calling function, append
 *        a newline, and print to stderr.
 */
#define pr_dbg(...)     pr_pfx_(xdbg,        __VA_ARGS__)

/**
 * @brief Prepend "warning: ", append a newline, and print to stderr.
 */
#define pr_wrn_(...)    pr_pfx_(xwrn_,       __VA_ARGS__)

/**
 * @brief Prepend "warning: " followed by the name of the calling
 *        function, append a newline, and print to stderr.
 */
#define pr_wrn(...)     pr_pfx_(xwrn,        __VA_ARGS__)

/**
 * @brief Prepend "warning: ", call `strerror()` with the specified
 *        error number and append the result, append a newline, and
 *        print to stderr.
 */
#define pr_wrrno_(...)  pr_pfx_(xwrrno_,     __VA_ARGS__)

/**
 * @brief Prepend "warning: " followed by the name of the calling
 *        function, call `strerror()` with the specified error
 *        number and append the result, append a newline, and
 *        print to stderr.
 */
#define pr_wrrno(...)   pr_pfx_(xwrrno,      __VA_ARGS__)

/**
 * @brief Prepend "error: ", append a newline, and print to stderr.
 */
#define pr_err_(...)    pr_pfx_(xerr_,       __VA_ARGS__)

/**
 * @brief Prepend "error: " followed by the name of the calling
 *        function, append a newline, and print to stderr.
 */
#define pr_err(...)     pr_pfx_(xerr,        __VA_ARGS__)

/**
 * @brief Prepend "error: ", call `strerror()` with the specified
 *        error number and append the result, append a newline, and
 *        print to stderr.
 */
#define pr_errno_(...)  pr_pfx_(xerrno_,     __VA_ARGS__)

/**
 * @brief Prepend "error: " followed by the name of the calling
 *        function, call `strerror()` with the specified error
 *        number and append the result, append a newline, and
 *        print to stderr.
 */
#define pr_errno(...)   pr_pfx_(xerrno,      __VA_ARGS__)

#define pr_x_(f_p, ...) (void)fprintf(f_p,"" __VA_ARGS__)
#define pr_pfx_(x, ...) do {                            \
        diag_clang(push)                                \
        diag_clang(ignored                              \
                "-Wgnu-zero-variadic-macro-arguments")  \
        pr_##x(__VA_ARGS__);                            \
        diag_clang(pop)                                 \
} while (0)

#ifdef NDEBUG
# define pr_xdbg_(...)  do{}while(0)
# define pr_xdbg(...)   do{}while(0)
#endif /* NDEBUG */

#ifndef NO_VA_OPT

# define pr_(...)           pr_x_(stderr __VA_OPT__(,) __VA_ARGS__)
# define pr_xout_(...)      pr_x_(stdout __VA_OPT__(,) __VA_ARGS__)
# define pr_xwrn_(fmt, ...) pr_("warning: " fmt "\n" __VA_OPT__(,) __VA_ARGS__)
# define pr_xwrrno_(e, ...) pr_xstrerr_(xwrn_, (e) __VA_OPT__(,) __VA_ARGS__)
# define pr_xerr_(fmt, ...) pr_(  "error: " fmt "\n" __VA_OPT__(,) __VA_ARGS__)
# define pr_xerrno_(e, ...) pr_xstrerr_(xerr_, (e) __VA_OPT__(,) __VA_ARGS__)
# define pr_xout(fmt, ...)  pr_xout_(fmt "\n" __VA_OPT__(,) __VA_ARGS__)
# define pr_xwrn(fmt, ...)  pr_xwrn_("%s: " fmt, __func__ __VA_OPT__(,) __VA_ARGS__)
# define pr_xwrrno(e, ...)  pr_xstrerr_(xwrn, (e) __VA_OPT__(,) __VA_ARGS__)
# define pr_xerr(fmt, ...)  pr_xerr_("%s: " fmt, __func__ __VA_OPT__(,) __VA_ARGS__)
# define pr_xerrno(e, ...)  pr_xstrerr_(xerr, (e) __VA_OPT__(,) __VA_ARGS__)

# define pr_xstrerr_(x, e, fmt, ...) pr_##x(fmt "%s%s"  \
        __VA_OPT__(,) __VA_ARGS__, &"\0: "[!!(fmt)[0]], \
        strerror(e))

# ifndef NDEBUG
#  define pr_xdbg_(fmt,...) pr_(fmt "\n" __VA_OPT__(,) __VA_ARGS__)
#  define pr_xdbg(fmt,...)  pr_("%s:%d:%s: " fmt "\n", __FILE__, \
                                __LINE__, __func__ __VA_OPT__(,) __VA_ARGS__)
# endif /* !NDEBUG */

#else /* !NO_VA_OPT */

diag_clang(push)
diag_clang(ignored "-Wgnu-zero-variadic-macro-arguments")

# define pr_(...)           pr_x_(stderr, ## __VA_ARGS__)
# define pr_xout_(...)      pr_x_(stdout, ## __VA_ARGS__)
# define pr_xwrn_(fmt, ...) pr_("warning: " fmt "\n", ## __VA_ARGS__)
# define pr_xwrrno_(e, ...) pr_xstrerr_(xwrn_, (e), ## __VA_ARGS__)
# define pr_xerr_(fmt, ...) pr_(  "error: " fmt "\n", ## __VA_ARGS__)
# define pr_xerrno_(e, ...) pr_xstrerr_(xerr_, (e), ## __VA_ARGS__)
# define pr_xout(fmt, ...)  pr_xout_(fmt "\n", ## __VA_ARGS__)
# define pr_xwrn(fmt, ...)  pr_xwrn_("%s: " fmt, __func__, ## __VA_ARGS__)
# define pr_xwrrno(e, ...)  pr_xstrerr_(xwrn, (e), ## __VA_ARGS__)
# define pr_xerr(fmt, ...)  pr_xerr_("%s: " fmt, __func__, ## __VA_ARGS__)
# define pr_xerrno(e, ...)  pr_xstrerr_(xerr, (e), ## __VA_ARGS__)

# define pr_xstrerr_(x, e, fmt, ...) pr_##x(fmt "%s%s"  \
        , ## __VA_ARGS__, &"\0: "[!!(fmt)[0]], strerror(e))

# ifndef NDEBUG
#  define pr_xdbg_(fmt,...) pr_(fmt "\n", ## __VA_ARGS__)
#  define pr_xdbg(fmt,...)  pr_("%s:%d:%s: " fmt "\n", __FILE__, \
                                __LINE__, __func__, ## __VA_ARGS__)
# endif /* !NDEBUG */

diag_clang(pop)

#endif /* !NO_VA_OPT */

#ifdef NDEBUG

# define IF_DEBUG(...)
# define IF_NDEBUG(...)     __VA_ARGS__

# define dbg_close          close
# define dbg_fclose         fclose
# define dbg_fopen          fopen
# define dbg_fstat          fstat
# define dbg_mkdirat        mkdirat
# define dbg_mkdtemp        mkdtemp
# define dbg_mkfifoat       mkfifoat
# define dbg_open           open
# define dbg_openat         openat
# define dbg_renameat       renameat
# define dbg_rmdir          rmdir
# define dbg_stat           stat
# define dbg_unlink         unlink
# define dbg_unlinkat       unlinkat

#else /* NDEBUG */

# define IF_DEBUG(...)      __VA_ARGS__
# define IF_NDEBUG(...)

# define dbg_close(fd) ({                       \
  int fd__ = (fd);                              \
  int rv__ = close(fd__);                       \
  int ec__ = errno;                             \
  pr_dbg_("close(%d) -> %d",                    \
          fd__, rv__);                          \
  errno = ec__;                                 \
  rv__;                                         \
})

# define dbg_fclose(fp) ({                      \
  FILE *fp__ = (fp);                            \
  char fs__[24] = {'\0'};                       \
  snprintf(fs__, sizeof(fs__), "%p",            \
           (void *)fp__);                       \
  int rv__ = fclose(fp__);                      \
  int ec__ = errno;                             \
  fp__ = NULL;                                  \
  pr_dbg_("fclose(%s) -> %d", fs__, rv__);      \
  __builtin_memset(fs__, 0, sizeof(fs__));      \
  errno = ec__;                                 \
  rv__;                                         \
})

# define dbg_fopen(pt, md) ({                   \
  char const *pt__ = (pt), *md__ = (md);        \
  FILE *fp__ = fopen(pt__, md__);               \
  int ec__ = errno;                             \
  pr_dbg_("fopen(\"%s\", \"%s\") -> %p", pt__,  \
          md__, (void *)fp__);                  \
  errno = ec__;                                 \
  fp__;                                         \
})

# define dbg_fstat(fd, sb) ({                   \
  int fd__ = (fd);                              \
  int rv__ = fstat(fd__, (sb));                 \
  int ec__ = errno;                             \
  pr_dbg_("fstat(%d, " #sb ") "                 \
          "-> %d", fd__, rv__);                 \
  errno = ec__;                                 \
  rv__;                                         \
})

# define dbg_mkdirat(fd, pt, md) ({             \
  int fd__ = (fd);                              \
  char const *pt__ = (pt);                      \
  mode_t md__ = (md);                           \
  int rv__ = mkdirat(fd__, pt__, md__);         \
  int ec__ = errno;                             \
  pr_dbg_("mkdirat(%d, \"%s\", %s%o) -> %d",    \
          fd__, pt__, md__?"0":"", md__, rv__); \
  errno = ec__;                                 \
  rv__;                                         \
})

extern char *
dbg_mkdtemp_ (char *tmpl) nonnull_in();

# define dbg_mkdtemp(x) _Generic(0, default:    \
  (assert((x) != nullptr), dbg_mkdtemp_))(x)

# define dbg_mkfifoat(fd, pt, md) ({            \
  int fd__ = (fd);                              \
  char const *pt__ = (pt);                      \
  mode_t md__ = (md);                           \
  int rv__ = mkfifoat(fd__, pt__, md__);        \
  int ec__ = errno;                             \
  pr_dbg_("mkfifoat(%d, \"%s\", %s%o) -> %d",   \
          fd__, pt__, md__?"0":"", md__, rv__); \
  errno = ec__;                                 \
  rv__;                                         \
})

# define dbg_open(pt, fl, md) ({                \
  char const *pt__ = (pt);                      \
  int fl__ = (fl);                              \
  mode_t md__ = (md);                           \
  int rv__ = open(pt__, fl__, md__);            \
  int ec__ = errno;                             \
  pr_dbg_("open(\"%s\", 0x%x, %s%o) -> %d",     \
          pt__, fl__, md__?"0":"", md__, rv__); \
  errno = ec__;                                 \
  rv__;                                         \
})

# define dbg_openat(fd, pt, fl, md) ({          \
  int fd__ = (fd);                              \
  char const *pt__ = (pt);                      \
  int fl__ = (fl);                              \
  mode_t md__ = (md);                           \
  int rv__ = openat(fd__, pt__, fl__, md__);    \
  int ec__ = errno;                             \
  pr_dbg_("openat(%d, \"%s\", 0x%x, %s%o)"      \
          " -> %d", fd__, pt__, fl__,           \
          md__ ? "0" : "", md__, rv__);         \
  errno = ec__;                                 \
  rv__;                                         \
})

# define dbg_renameat(fd1, pt1, fd2, pt2) ({    \
  int fd1_ = (fd1), fd2_ = (fd2);               \
  char const *pt1_ = (pt1), *pt2_ = (pt2);      \
  int rv__ = renameat(fd1_, pt1_, fd2_, pt2_);  \
  int ec__ = errno;                             \
  pr_dbg_("renameat(%d, \"%s\", %d, \"%s\") ->" \
          " %d", fd1_, pt1_, fd2_, pt2_, rv__); \
  errno = ec__;                                 \
  rv__;                                         \
})

# define dbg_rmdir(pt) ({                       \
  char const *pt__ = (pt);                      \
  int rv__ = rmdir(pt__);                       \
  int ec__ = errno;                             \
  pr_dbg_("rmdir(\"%s\") -> %d", pt__, rv__);   \
  errno = ec__;                                 \
  rv__;                                         \
})

# define dbg_stat(pt, sb) ({                    \
  char const *pt__ = (pt);                      \
  int rv__ = stat(pt__, (sb));                  \
  int ec__ = errno;                             \
  pr_dbg_("stat(\"%s\", " #sb ") -> %d", pt__,  \
          rv__);                                \
  errno = ec__;                                 \
  rv__;                                         \
})

# define dbg_unlink(pt) ({                      \
  char const *pt__ = (pt);                      \
  int rv__ = unlink(pt__);                      \
  int ec__ = errno;                             \
  pr_dbg_("unlink(\"%s\") -> %d", pt__, rv__);  \
  errno = ec__;                                 \
  rv__;                                         \
})

# define dbg_unlinkat(fd, pt, fl) ({            \
  int fd__ = (fd);                              \
  char const *pt__ = (pt);                      \
  int fl__ = (fl);                              \
  int rv__ = unlinkat(fd__, pt__, fl__);        \
  int ec__ = errno;                             \
  pr_dbg_("unlinkat(%d, \"%s\", 0x%x) -> %d",   \
          fd__, pt__, fl__, rv__);              \
  errno = ec__;                                 \
  rv__;                                         \
})

#endif /* NDEBUG */

#endif /* LIBCANTH_SRC_DBG_H_ */
