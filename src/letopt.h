/* SPDX-License-Identifier: LGPL-2.1-or-later */
#ifndef LETOPT_H_
#define LETOPT_H_

#if defined OPTIONS || defined INCLUDED_FROM_LETOPT_C_
# if __STDC_VERSION__ < 202000L || (__GNUC__ < 13 && \
     (!defined(__clang_major__) || __clang_major__ < 18))
#  include <stdbool.h>
# endif
# include <errno.h>
# include <limits.h>
# include <stddef.h>
# include <stdint.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>

#ifndef noreturn
# if __STDC_VERSION__ >= 202000L && (__GNUC__ >= 13 || \
     (defined(__clang_major__) && __clang_major__ >= 15))
#  define noreturn [[noreturn]]
# elif __STDC_VERSION__ >= 201112L
#  define noreturn _Noreturn
# else // __STDC_VERSION__ < 201112L
#  define noreturn __attribute__((noreturn))
# endif // __STDC_VERSION__
#endif // noreturn

struct letopt_state {
	char       **v;
	int          c;
	int          e;
	char const **q;
	int          i;
	int          n;
	char        *p;
};

/* Temporary internal convenience macros for sprinkling extern
 * declarations inside function bodies and letopt.c. Should be
 * undefined immediately after use.
 *
 * An ugly hack, sure, but it serves its intended dual purpose
 * of reducing internal functions' visibility without multiple
 * source locations to keep in sync.
 */
#define extern_letopt_state_init()                           \
        extern struct letopt_state                           \
        letopt_state_init (int    argc,                      \
                           char **argv)
#define extern_letopt_get_long_opt_arg()                     \
        extern int                                           \
        letopt_get_long_opt_arg (struct letopt_state *state, \
                                 size_t               len)
#define extern_letopt_get_number_arg()                       \
        extern bool                                          \
        letopt_get_number_arg (struct letopt_state *state,   \
                               int64_t             *dest,    \
                               int64_t              min,     \
                               int64_t              max)
#define extern_letopt_get_string_arg()                       \
        extern bool                                          \
        letopt_get_string_arg (struct letopt_state  *state,  \
                               char const          **dest)

#endif // OPTIONS || INCLUDED_FROM_LETOPT_C_

#ifdef OPTIONS
#include "ligma.h"
#include "util.h"

#define letopt_error(opt) ((opt)->p.e)
#define letopt_nargs(opt) ((opt)->p.n)
#define letopt_arg(opt,i) ((opt)->p.q[i])

#define call(f, ...)   f(__VA_ARGS__)
#define eval(...)      __VA_ARGS__
#define first(x, ...)  x

#define boolean_arg(c,s,d,...) c, s, "", d
#define filter_args(T,tag,...) (T##_arg(__VA_ARGS__))
#define generic_arg(c,s,d,...) c, s, "" first(__VA_ARGS__,), d
#define number_arg generic_arg
#define string_arg generic_arg
#define transformed_options(X) eval(OPTIONS(X filter_args))

diag_clang(push)
diag_clang(ignored "-Wc++-compat")
diag_clang(ignored "-Wc++98-compat")
diag_clang(ignored "-Wgnu-empty-initializer")
diag_clang(ignored "-Wgnu-zero-variadic-macro-arguments")
diag_clang(ignored "-Wpre-c2x-compat")
diag_clang(ignored "-Wunknown-attributes")

#define ws0x0()  " "
#define ws0x1()  "  "
#define ws0x2()  "    "
#define ws0x3()  "        "
#define ws0x4()  "                "
#define ws0x5()  "                                "
#define ws0x6()  "                                                                "
#define ws0x7() ws0x6() ws0x6()
#define ws0x8() ws0x7() ws0x7()
#define ws0x9() ws0x8() ws0x8()
#define ws0xa() ws0x9() ws0x9()
#define ws0xb() ws0xa() ws0xa()

#define bit_(b, n, v) _Generic(&(char[1U + ((n) & (1U << (size_t)b))]){0} \
	, char(*)[1U + (1U << (size_t)b)]: (const struct bit##b##_on){v}  \
	, char(*)[1U]: (const struct bit_off){})

#define bit(b, n)      bit_(b, n, {ws##b()})
#define bit_type(b, n) typeof(bit_(b, n, 0))

#define ws_type(n) struct {                             \
	bit_type(0x0,n) bit0x0; bit_type(0x1,n) bit0x1; \
	bit_type(0x2,n) bit0x2; bit_type(0x3,n) bit0x3; \
	bit_type(0x4,n) bit0x4; bit_type(0x5,n) bit0x5; \
	bit_type(0x6,n) bit0x6; bit_type(0x7,n) bit0x7; \
	bit_type(0x8,n) bit0x8; bit_type(0x9,n) bit0x9; \
	bit_type(0xa,n) bit0xa; bit_type(0xb,n) bit0xb; }

#define ws_init(n) {                                    \
	bit(0x0,n), bit(0x1,n), bit(0x2,n), bit(0x3,n), \
	bit(0x4,n), bit(0x5,n), bit(0x6,n), bit(0x7,n), \
	bit(0x8,n), bit(0x9,n), bit(0xa,n), bit(0xb,n), }

#define dog(x,y) x##y
#define cat(x,y) dog(x, y)
#define sym()    cat(m_, __COUNTER__)
#define c_str(x) const char sym()[sizeof x]

#ifdef NO_VA_OPT
#define arg0(def_arg, ...) arg0_if_no_args(def_arg,__VA_ARGS__) first(__VA_ARGS__,)
#else
#define arg0(def_arg, ...) first(__VA_OPT__(__VA_ARGS__,)def_arg,)
#endif
#define arg1_(def, x, ...) arg0(def,__VA_ARGS__)
#define arg1(def_arg, ...) arg1_(def_arg,__VA_ARGS__)
#define arg2_(def, x, ...) arg1_(def,__VA_ARGS__)
#define arg2(def_arg, ...) arg2_(def_arg,__VA_ARGS__)
#define arg3_(def, x, ...) arg2_(def,__VA_ARGS__)
#define arg3(def_arg, ...) arg3_(def_arg,__VA_ARGS__)

#define gen_arg_list(T,tag,c,s,d,...) \
	T,tag,c,s,d T##_(__VA_ARGS__)
#define boolean_(...)   ,""           \
	, arg0(    false,__VA_ARGS__),,
#define string_(...)                  \
	, arg0(    "ARG",__VA_ARGS__) \
	, arg1(       "",__VA_ARGS__),,
#define number_(...)                  \
	, arg0(    "NUM",__VA_ARGS__) \
	, arg1(        0,__VA_ARGS__) \
	, arg2(INT64_MIN,__VA_ARGS__) \
	, arg3(INT64_MAX,__VA_ARGS__)

#define gen_opt_var(T,...) call(opt_var \
	, gen_arg_list(T,__VA_ARGS__),)

diag_clang(push)
diag_clang(ignored "-Wpadded")

struct letopt {
	struct letopt_state p;

	#define opt_var(T, tag, c, s, d, a, ...) \
		T##_var(tag,__VA_ARGS__,)

	#define boolean_var(...)
	#define string_var(...)
	#define number_var(tag,...) int64_t m_##tag;
	OPTIONS(gen_opt_var)
	#undef number_var
	#undef string_var

	#define number_var(...)
	#define string_var(tag,...) char const *m_##tag;
	OPTIONS(gen_opt_var)
	#undef string_var
	#undef boolean_var

	#define string_var(...)
	#define boolean_var(tag,...) bool m_##tag;
	OPTIONS(gen_opt_var)
	#undef boolean_var
	#undef string_var
	#undef number_var
	#undef opt_var

	struct {
		#define opt_var(T, tag, ...) bool tag;
		OPTIONS(gen_opt_var)
		#undef opt_var
	} has;
};

diag_clang(pop)

__attribute__((always_inline))
static inline bool
handle_long_opt (struct letopt *const opt)
{
	extern_letopt_get_long_opt_arg();
	extern_letopt_get_number_arg();

	#ifndef INCLUDED_FROM_LETOPT_C_
	# undef extern_letopt_get_long_opt_arg
	#endif // !INCLUDED_FROM_LETOPT_C_

	#define parse_str(T, tag, chr, str, ...)                           \
		if (!__builtin_strncmp(opt->p.p, str, sizeof str - 1U)) {  \
			call(T##_str_opt,                                  \
			     gen_arg_list(T, tag, chr, str, __VA_ARGS__),) \
		}

	#define boolean_str_opt(T, tag, chr, str, ...) \
		if (!opt->p.p[sizeof str - 1U]) {      \
			opt->m_##tag = true;           \
			opt->has.tag = true;           \
			return true;                   \
		}

	#define number_str_opt(T, tag, chr, str, doc, a, def, min, max, ...) \
		int e = letopt_get_long_opt_arg(&opt->p, sizeof str - 1U);   \
		if (!e) {                                                    \
			if (letopt_get_number_arg(&opt->p, &opt->m_##tag,    \
			                          min, max)) {               \
				opt->has.tag = true;                         \
				return true;                                 \
			}                                                    \
			break;                                               \
		}                                                            \
		if (e != EAGAIN)                                             \
			break;

	#define string_str_opt(T, tag, chr, str, ...)                        \
		int e = letopt_get_long_opt_arg(&opt->p, sizeof str - 1U);   \
		if (!e) {                                                    \
			opt->m_##tag = opt->p.p;                             \
			opt->has.tag = true;                                 \
			return true;                                         \
		}                                                            \
		if (e != EAGAIN)                                             \
			break;

	do {
		OPTIONS(parse_str)
		opt->p.e = EINVAL;
	} while (0);

	return false;

	#undef string_str_opt
	#undef number_str_opt
	#undef boolean_str_opt
	#undef parse_str
}

diag(push)
diag(ignored "-Wunused-label")
__attribute__((always_inline))
static inline bool
handle_short_opt (struct letopt *const opt)
{
	extern_letopt_get_number_arg();
	extern_letopt_get_string_arg();

	#ifndef INCLUDED_FROM_LETOPT_C_
	# undef extern_letopt_get_number_arg
	# undef extern_letopt_get_string_arg
	#endif // !INCLUDED_FROM_LETOPT_C_

	#define parse_chr(T, tag, chr, ...)                           \
		case chr:                                             \
			++opt->p.p;                                   \
			call(T##_chr_opt,                             \
			     gen_arg_list(T, tag, chr, __VA_ARGS__),) \
			opt->has.tag = true;                          \
			return true;

	#define boolean_chr_opt(T, tag, ...)         \
		opt->m_##tag = true;                 \
		if (*opt->p.p) {                     \
			opt->has.tag = true;         \
			goto next;                   \
		}

	#define number_chr_opt(T,tag,chr,str,doc,arg,def,min,max,...) \
		if (!*opt->p.p) {                                     \
			if (++opt->p.i >= opt->p.c)                   \
				goto invargs;                         \
			opt->p.p = opt->p.v[opt->p.i];                \
		}                                                     \
		if (!letopt_get_number_arg(&opt->p, &opt->m_##tag,    \
		                           min, max))                 \
			break;

	#define string_chr_opt(T, tag, ...)                           \
		if (!*opt->p.p) {                                     \
			if (++opt->p.i >= opt->p.c)                   \
				goto invargs;                         \
			opt->p.p = opt->p.v[opt->p.i];                \
		}                                                     \
		if (!letopt_get_string_arg(&opt->p, &opt->m_##tag))   \
			break;

next:
	switch (*opt->p.p) {
		OPTIONS(parse_chr)
	default:
	invargs:
		opt->p.e = EINVAL;
	}

	return false;

	#undef string_chr_opt
	#undef number_chr_opt
	#undef boolean_chr_opt
	#undef parse_chr
}
diag(pop)

static int
letopt_fini (struct letopt *opt);

noreturn static void
letopt_helpful_exit (struct letopt *opt);

static struct letopt
letopt_init (int    argc,
             char **argv)
{
	extern_letopt_state_init();

	#ifndef INCLUDED_FROM_LETOPT_C_
	# undef extern_letopt_state_init
	#endif // !INCLUDED_FROM_LETOPT_C_

	struct letopt opt = {
		.p = letopt_state_init(argc, argv)

		#ifdef NO_VA_OPT
		#define deparen(...)    va_cat(vanq, uish __VA_ARGS__)
		#define va_cat(x,...)   va_cat_(x, __VA_ARGS__)
		#define va_cat_(x,...)  x ## __VA_ARGS__
		#define uish(...)       uish __VA_ARGS__
		#define vanquish
		#define expand(...)     __VA_ARGS__
		#define opt_var_impl_(...) expand(deparen \
			arg0_if_no_args(()naught,##__VA_ARGS__))
		#define opt_var_impl(...) opt_var_impl_(__VA_ARGS__)
		#define opt_var(T, tag, chr, str, doc, arg, ...) \
			opt_var_impl(first(__VA_ARGS__,))(, \
				.m_##tag = first(__VA_ARGS__,))

		#else // NO_VA_OPT
		#define opt_var(T, tag, chr, str, doc, arg, ...) \
			__VA_OPT__(, .m_##tag = first(__VA_ARGS__,))
		#endif // NO_VA_OPT

		OPTIONS(gen_opt_var)
		#undef opt_var

		#ifdef NO_VA_OPT
		#undef opt_var_impl
		#undef opt_var_impl_
		#undef expand
		#undef vanquish
		#undef uish
		#undef va_cat_
		#undef va_cat
		#undef deparen
		#endif // NO_VA_OPT
	};

	if (opt.p.e) {
#ifdef PROGNAME
		(void)fprintf(stderr, PROGNAME ": %s: %s\n",
		              __func__, strerror(opt.p.e));
#else // PROGNAME
		(void)fprintf(stderr, "%s: %s: %s\n", *argv,
		              __func__, strerror(opt.p.e));
#endif // PROGNAME
		exit(letopt_fini(&opt));
	}

	struct letopt_state *state = &opt.p;
	int options_end = state->c;

	while (!state->e && ++state->i < state->c) {
		state->p = state->v[state->i];

		if (state->i >= options_end || *state->p != '-') {
			state->q[state->n++] = state->p;
			continue;
		}

		++state->p;
		switch (*state->p) {
		case '-':
			++state->p;
			if (!*state->p) {
				options_end = state->i + 1;
				continue;
			}

			if (!handle_long_opt(&opt))
				break;

			continue;

		default:
			if (!handle_short_opt(&opt))
				break;

			continue;
		}

		if (!state->e)
			state->e = EINVAL;
	}

	if (state->e)
		letopt_helpful_exit(&opt);

	return opt;
}

#undef gen_opt_var
#undef number_
#undef string_
#undef boolean_
#undef gen_arg_list
#undef arg3
#undef arg3_
#undef arg2
#undef arg2_
#undef arg1
#undef arg1_
#undef arg0

static int
letopt_fini (struct letopt *opt)
{
	int e = opt->p.e;
	free(opt->p.q);
	__builtin_memset(opt, 0, sizeof *opt);
	return e;
}

#define space(x)      ws_type(sizeof(max_align) - sizeof x) sym()
#define space_init(x) ws_init(sizeof(max_align) - sizeof x)

#define line_vars(chr, str, arg, help) \
	c_str("  -");                  \
	c_str(", --" str " " arg);     \
	space(str " " arg);            \
	c_str(help);

#define line_init(chr, str, arg, help) \
	{ ' ', ' ', '-', chr },        \
	", --" str " " arg " ",        \
	space_init(str " " arg),       \
	help "\n",

#define help_text(x) ((const union {                 \
	const struct {                               \
		c_str(LETOPT_HELP_OVERVIEW);         \
		x(line_vars)                         \
		c_str(LETOPT_HELP_DETAILS);          \
	};                                           \
	const char str[sizeof (                      \
		struct {                             \
			c_str(LETOPT_HELP_OVERVIEW); \
			x(line_vars)                 \
			c_str(LETOPT_HELP_DETAILS);  \
		}                                    \
	)];                                          \
}){{                                                 \
	LETOPT_HELP_OVERVIEW "\n", \
	x(line_init)               \
	LETOPT_HELP_DETAILS        \
}}.str)

#ifdef SYNOPSIS
# define LETOPT_HELP_SYNOPSIS SYNOPSIS "\n"
#else // SYNOPSIS
# define LETOPT_HELP_SYNOPSIS "[OPTION]...\n"
#endif // SYNOPSIS

#ifdef PURPOSE
# define LETOPT_HELP_PURPOSE "\n" PURPOSE "\n"
#else // PURPOSE
# define LETOPT_HELP_PURPOSE "\nOptions:"
#endif // PURPOSE

#ifdef PROGNAME
# define LETOPT_HELP_OVERVIEW \
	"Usage: " PROGNAME " " LETOPT_HELP_SYNOPSIS LETOPT_HELP_PURPOSE
#else // PROGNAME
# define LETOPT_HELP_OVERVIEW LETOPT_HELP_SYNOPSIS LETOPT_HELP_PURPOSE
#endif // PROGNAME

#ifdef DETAILS
# define LETOPT_HELP_DETAILS "\n" DETAILS "\n"
#else // DETAILS
# define LETOPT_HELP_DETAILS ""
#endif // DETAILS

ligma_gcc(push_options)
ligma_gcc(optimize ("Os"))
static void
letopt_usage (struct letopt const *const opt)
{
	diag(push)

	diag_clang(ignored "-Wgnu-empty-struct")
	diag_gcc(ignored "-Wpedantic")
	struct bit_off   { };

	diag(pop)

	struct bit0x0_on { const char a[1U <<  0U]; };
	struct bit0x1_on { const char a[1U <<  1U]; };
	struct bit0x2_on { const char a[1U <<  2U]; };
	struct bit0x3_on { const char a[1U <<  3U]; };
	struct bit0x4_on { const char a[1U <<  4U]; };
	struct bit0x5_on { const char a[1U <<  5U]; };
	struct bit0x6_on { const char a[1U <<  6U]; };
	struct bit0x7_on { const char a[1U <<  7U]; };
	struct bit0x8_on { const char a[1U <<  8U]; };
	struct bit0x9_on { const char a[1U <<  9U]; };
	struct bit0xa_on { const char a[1U << 10U]; };
	struct bit0xb_on { const char a[1U << 11U]; };

	#define mk_arr(c, str, arg, ...) \
		char sym()[sizeof str " " arg + (sizeof arg > 1U)];
	typedef char max_align[sizeof(union {transformed_options(mk_arr)})];
	#undef mk_arr

	if (opt->p.e)
		(void)fprintf(stderr, "%s\n", strerror(opt->p.e));

diag_gcc(push)
diag_gcc(ignored "-Wpedantic")

#ifdef PROGNAME
	(void)fputs(help_text(transformed_options), stdout);
#else // PROGNAME
	(void)printf("Usage: %s %s", opt->p.v[0],
	             help_text(transformed_options));
#endif // PROGNAME

diag_gcc(pop)
}
ligma_gcc(pop_options)

noreturn static void
letopt_helpful_exit (struct letopt *opt)
{
	letopt_usage(opt);
	exit(letopt_fini(opt));
}

diag_clang(pop)

#undef LETOPT_HELP_DETAILS
#undef LETOPT_HELP_OVERVIEW
#undef LETOPT_HELP_PURPOSE
#undef LETOPT_HELP_SYNOPSIS

#undef help_text
#undef line_init
#undef line_vars
#undef space_init
#undef space
#undef c_str
#undef sym
#undef cat
#undef dog
#undef ws_init
#undef ws_type
#undef bit_type
#undef bit
#undef bit_
#undef ws0xb
#undef ws0xa
#undef ws0x9
#undef ws0x8
#undef ws0x7
#undef ws0x6
#undef ws0x5
#undef ws0x4
#undef ws0x3
#undef ws0x2
#undef ws0x1
#undef ws0x0
#undef transformed_options
#undef string_arg
#undef number_arg
#undef generic_arg
#undef filter_args
#undef boolean_arg
#undef first
#undef eval
#undef call

#endif // OPTIONS

#endif // LETOPT_H_
