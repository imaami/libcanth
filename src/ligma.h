/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file ligma.h
 *
 * @author Juuso Alasuutari
 */
#ifndef LIBCANTH_SRC_LIGMA_H_
#define LIBCANTH_SRC_LIGMA_H_

#define diag_apple_clang(...)   ligma_apple_clang(diagnostic __VA_ARGS__)
#define diag_clang(...)         ligma_clang(diagnostic __VA_ARGS__)
#define diag_gcc(...)           ligma_gcc(diagnostic __VA_ARGS__)
#define diag(...)               diag_clang(__VA_ARGS__) diag_gcc(__VA_ARGS__)

#ifdef __apple_build_version__
# define ligma_apple_clang      ligma_clang
#else
# define ligma_apple_clang(...)
#endif

#ifdef __clang__
# define ligma_clang(...)       ligma(clang __VA_ARGS__)
#else
# define ligma_clang(...)
#endif

#if !defined __clang__ && defined __GNUC__
# define ligma_gcc(...)         ligma(GCC __VA_ARGS__)
#else
# define ligma_gcc(...)
#endif

#define ligma_(...)             # __VA_ARGS__
#define ligma(...)              _Pragma(ligma_(__VA_ARGS__))

#endif /* LIBCANTH_SRC_LIGMA_H_ */
