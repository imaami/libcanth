/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file util.h
 *
 * @author Juuso Alasuutari
 */
#ifndef LIBCANTH_SRC_UTIL_H_
#define LIBCANTH_SRC_UTIL_H_

/** @brief Instruct the compiler to always inline a function.
 */
#define force_inline __attribute__((always_inline)) inline

/** @brief Instruct the compiler to always inline a function
 *         and hint that its return value is invariant.
 */
#define const_inline __attribute__((always_inline,const)) inline

/** @brief Function returns a specific baked-in data pointer.
 */
#define const_nonnull __attribute__((const,returns_nonnull))

/** @brief Suppress compiler warnings about an unused entity.
 */
#define useless __attribute__((unused))

/** @brief Calculate the element count of an array.
 */
#define array_size(x) (sizeof(x) / sizeof((x)[0]))

/** @brief Stringify arguments.
 */
#define stringify(...) stringify_(__VA_ARGS__)
#define stringify_(...) #__VA_ARGS__ ""

#ifndef __cplusplus

#define is_negative(x) (_Generic((x), \
        default:(x), unsigned char:1, \
        unsigned short:1, unsigned:1, \
        typeof(1UL):1,typeof(1ULL):1) < (typeof(x))0)

/** @brief Check if a value is a char array.
 */
#define is_char_array(x) _Generic((typeof(x) *){0}, \
        char (*)[sizeof(x)]: 1, char const (*)[sizeof(x)]: 1, default: 0)

/** @brief Check if a value is a string literal.
 */
#define is_string_literal(x) __builtin_choose_expr( \
        is_char_array(x), __builtin_constant_p(x), 0)

/** @brief Evaluates to the string literal passed as the first parameter if
 *         it would fit in the array specified by the second parameter, but
 *         otherwise evaluates to the empty string.
 *
 * This macro is useful for removing warnings resulting from non-taken code
 * paths being checked for correctness, a common occurrence with constructs
 * such as `_Generic()` and `__builtin_choose_expr()`.
 *
 * @param lit  The string literal which becomes the result of this macro if
 *             it would fit in the array specified by `what`.
 * @param what The array, or type of array, to use as type information when
 *             determining the maximum size of `lit`.
 */
#define safe_string_literal(lit, what)                                  \
        __builtin_choose_expr(                                          \
                is_string_literal(lit) && sizeof(lit) <= sizeof(what),  \
                lit, "")

#endif /* __cplusplus */

#endif /* LIBCANTH_SRC_UTIL_H_ */
