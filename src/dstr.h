/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file dstr.h
 *
 * @author Juuso Alasuutari
 */
#ifndef LIBCANTH_SRC_DSTR_H_
#define LIBCANTH_SRC_DSTR_H_

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "ligma.h"
#include "util.h"

#if clang_older_than_version(10)
 diag_clang(push)
 diag_clang(ignored "-Wdocumentation-unknown-command")
# if clang_older_than_version(9)
  diag_clang(ignored "-Wmissing-braces")
# endif /* clang_older_than_version(9) */
#endif /* clang_older_than_version(10) */

/**
 * @brief dstr stands for dumb string.
 */
typedef struct dstr
{
	_Alignas(char *) union
	{
		struct __attribute__((packed))
		{
			union {
				char       *ptr;  //!< String pointer.
				char const *view; //!< String view.
			}; //!< @anon

			/** @brief Total heap allocation size. */
			unsigned int size;
		}; //!< @anon

		/** @brief String array. */
		char arr[sizeof (char *) + sizeof (unsigned int)];
	}; //!< @anon

	/** @brief String length excluding the null terminator. */
	unsigned int len;
} dstr;

/**
 * @brief Compound literal @ref dstr view initialized with a string literal.
 */
#define make_dstr_view_from_literal(x)                                     \
        __builtin_choose_expr(                                             \
                sizeof (x) <= sizeof ((dstr *)0)->arr,                     \
                (dstr){.arr = {safe_string_literal(x, ((dstr *)0)->arr)},  \
                       .len = sizeof (x) - 1u},                            \
                (dstr){.view = x, .size = 0u, .len = sizeof (x) - 1u})

/**
 * @brief Compound literal @ref dstr view initialized with a `char` array.
 */
#define make_dstr_view_from_array(x)                                       \
        __builtin_choose_expr(                                             \
                sizeof (x) <= sizeof ((dstr *)0)->arr,                     \
                make_dstr_from_small_string(x, sizeof (x) - 1u),           \
                (dstr){.view = x, .size = 0u, .len = sizeof (x) - 1u})

/**
 * @brief Initialize a @ref dstr view.
 */
#define make_dstr_view(x)                                                  \
        __builtin_choose_expr(                                             \
                is_string_literal(x),                                      \
                make_dstr_view_from_literal(x),                            \
                __builtin_choose_expr(                                     \
                        is_char_array(x),                                  \
                        make_dstr_view_from_array(x),                      \
                        make_dstr_view_from_decay(x, __builtin_strlen(x))))

/**
 * @brief Initialize a @ref dstr from a `char` array.
 */
#define make_dstr_from_array(x)                                            \
        __builtin_choose_expr(                                             \
                sizeof (x) <= sizeof ((dstr *)0)->arr,                     \
                make_dstr_from_small_string(x, sizeof (x) - 1u),           \
                (dstr){.ptr = __builtin_strndup((x), sizeof (x) - 1u),     \
                       .size = sizeof (x),                                 \
                       .len = sizeof (x) - 1u})

/**
 * @brief Initialize a @ref dstr.
 */
#define make_dstr(x)                                                       \
        __builtin_choose_expr(                                             \
                is_string_literal(x),                                      \
                make_dstr_view_from_literal(x),                            \
                __builtin_choose_expr(                                     \
                        is_char_array(x),                                  \
                        make_dstr_from_array(x),                           \
                        make_dstr_from_decay(x, __builtin_strlen(x))))

/**
 * @brief Create a @ref dstr by value from a `char *` and string length.
 *
 * @note The string length must be less than `UINT_MAX`.
 *
 * @param[in] src String pointer.
 * @param[in] len String length.
 * @return    A @ref dstr by value.
 */
__attribute__((pure))
static force_inline dstr
make_dstr_from_small_string (char const *const src,
                             size_t            len)
{
	dstr d = {.arr = {0}, .len = (unsigned)len};
	__builtin_memcpy(&d.arr[0], src, len);
	return d;
}

/**
 * @brief Create a @ref dstr view by value from a `char *` and string length.
 */
__attribute__((pure))
static force_inline dstr
make_dstr_view_from_decay (char const *const src,
                           size_t            len)
{
	return !len || len > UINT_MAX - 1u
	       ? (dstr){0}
	       : len < sizeof ((dstr *)0)->arr
	         ? make_dstr_from_small_string(src, len)
	         : (dstr){.view = src, .size = 0u, .len = (unsigned)len};
}

/**
 * @brief Create a @ref dstr by value from a `char *` and string length.
 */
__attribute__((pure))
static force_inline dstr
make_dstr_from_decay (char const *const src,
                      size_t            len)
{
	return !len || len > UINT_MAX - 1u
	       ? (dstr){0}
	       : len < sizeof ((dstr *)0)->arr
	         ? make_dstr_from_small_string(src, len)
	         : (dstr){.ptr = __builtin_strndup(src, len),
	                  .size = (unsigned)len + 1u,
	                  .len = (unsigned)len};
}

/**
 * @brief Zero-initialize a @ref dstr.
 *
 * @param[out] s Object to initialize.
 */
static force_inline void
dstr_init (dstr *s)
{
	s->ptr = NULL;
	s->size = 0u;
	s->len = 0u;
}

/**
 * @brief Get the value held by a @ref dstr as a pointer to `const char`.
 *
 * If the @ref dstr contains a pointer – either a heap-allocated string or
 * a view – then the value returned is the pointer itself. If the @ref dstr
 * contains a small string, then the value returned is the address of the
 * first character of the string.
 *
 * If the @ref dstr is empty, then the value returned is a pointer to a
 * null-terminated empty string. (At the implementation level, this is
 * simply the same as the value returned for a small string, with the
 * exception that the first character is the null terminator.)
 *
 * @warning The returned value is only valid until the next call to a
 * function that mutates the @ref dstr.
 *
 * @param[in] x The @ref dstr.
 * @return The value held by `x` as `const char *`.
 */
#define dstr_get(x) (            \
        !dstr_is_pointer(x)      \
        ? (char const *)(x)->arr \
        : !(x)->size ? (x)->view \
                     : (char const *)(x)->ptr)

/**
 * @brief Check if a @ref dstr is empty.
 *
 * @param[in] s Object to check.
 *
 * @return Whether or not @p s is empty.
 * @retval true  @p s is empty.
 * @retval false @p s is not empty.
 */
__attribute__((pure))
static force_inline bool
dstr_is_empty (dstr const *s)
{
	return !s->len;
}

/**
 * @brief Check if a @ref dstr contains an array.
 *
 * @param[in] s Object to check.
 *
 * @return Whether or not @p s contains an array.
 * @retval true  @p s contains an array; implies
 *               `@ref !dstr_is_empty() && @ref !dstr_is_pointer()`.
 * @retval false @p s does not contain an array.
 */
__attribute__((pure))
static force_inline bool
dstr_is_array (dstr const *s)
{
	return s->len && s->len < sizeof s->arr;
}

/**
 * @brief Check if a @ref dstr contains a pointer.
 *
 * @note Does not imply the pointed-to memory is owned by this @ref dstr,
 *       if you need to know that call @ref dstr_owns_memory() instead.
 *
 * @param[in] s Object to check.
 *
 * @return Whether or not @p s contains a pointer.
 * @retval true  @p s contains a pointer; implies @ref !dstr_is_empty().
 * @retval false @p s does not contain a pointer.
 */
__attribute__((pure))
static force_inline bool
dstr_is_pointer (dstr const *s)
{
	return s->len >= sizeof s->arr;
}

/**
 * @brief Check if a @ref dstr has ownership of allocated heap memory.
 *
 * @param[in] s Object to check.
 *
 * @return Whether or not @p s owns allocated heap memory.
 * @retval true  @p s owns memory; implies @ref dstr_is_pointer().
 * @retval false @p s does not own memory.
 */
__attribute__((pure))
static force_inline bool
dstr_owns_memory (dstr const *s)
{
	return dstr_is_pointer(s) && s->size;
}

/**
 * @brief Uninitialize and zero-out a @ref dstr.
 *
 * @param[in] s Object to uninitialize.
 */
static force_inline void
dstr_fini (dstr *s)
{
	if (dstr_owns_memory(s))
		free(s->ptr);
	dstr_init(s);
}

/**
 * @brief Compare a @ref dstr with a `char const *`.
 *
 * @param[in] s   Object to compare.
 * @param[in] str String to compare with.
 * @param[in] len Length of @p str.
 *
 * @return    Whether or not the strings are equal.
 */
__attribute__((pure))
static force_inline bool
dstr_eq (dstr const *s,
         char const *str,
         size_t      len)
{
	return len == s->len &&
	       (!len || !__builtin_memcmp(dstr_get(s), str, len));
}

/**
 * @brief Set the value of a @ref dstr, discarding its old value (if any).
 *
 * @param[in,out] dest Object to modify.
 * @param[in]     src  New value.
 * @param[in]     len  Length of @p src.
 * @param[out]    err  Where to save an errno value on failure,
 *                     otherwise not accessed. Mandatory.
 *
 * @return Whether or not the operation succeeds.
 */
extern bool
dstr_set (dstr       *dest,
          char const *src,
          size_t      len,
          int        *err);

/**
 * @brief Transfer the raw contents of one @ref dstr to another.
 *
 * @ref dstr_move() does nothing if @p src and @p dest point to the same
 * address.
 *
 * If @p src owns heap memory, its ownership is transferred. Memory owned by
 * @p dest is freed before the move operation.
 *
 * If @p dest is not empty, but @p src is empty, @ref dstr_move() is the
 * equivalent of calling @ref dstr_fini() on @p dest.
 *
 * After @ref dstr_move() returns, calling @ref dstr_get() on @p src is
 * guaranteed to return an empty string unless @p src and @p dest point
 * to the same address.
 *
 * @ref dstr_move() is guaranteed to never allocate new memory, but it may
 * free an existing allocation. Thus any pointers returned by @ref dstr_get()
 * on @p src or @p dest become invalid when @ref dstr_move() is called.
 *
 * @param dest Where to move the string.
 * @param src  Source of the moved string.
 */
extern void
dstr_move (dstr *dest,
           dstr *src);

#if clang_older_than_version(10)
 diag_clang(pop)
#endif /* clang_older_than_version(10) */

#endif /* LIBCANTH_SRC_DSTR_H_ */
