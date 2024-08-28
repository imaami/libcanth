/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file stochar.c
 *
 * @author Juuso Alasuutari
 */
#include <errno.h>
#include <inttypes.h>

#define PROGNAME "stochar"
#define SYNOPSIS "[OPTION]... [--] [STRING]..."
#define PURPOSE  "Converts stochastic unicode to/from UTF-8."

#define OPTIONS(X)                                    \
	X(boolean, help, 'h', "help",                 \
	  "print this help text and exit")            \
	                                              \
	X(boolean, replace, 'r', "replace",           \
	  "substitute U+FFFD for bad UTF-8")          \
	                                              \
	X(boolean, separate, 's', "separate",         \
	  "don't concatenate input strings")

#define DETAILS                                       \
 "Given no options, input strings are concatenated\n" \
 "and invalid UTF-8 sequences silently ignored.\n\n"  \
 "Encoded output should be considered binary data,\n" \
 "it is not printable in any existing encoding."

#include "letopt.h"

#undef DETAILS
#undef OPTIONS
#undef PURPOSE
#undef SYNOPSIS
#undef PROGNAME

#include "dbg.h"
#include "utf8.h"

static const_inline size_t
saturated_add_uz (size_t a,
                  size_t b)
{
	if ((a += b) < b)
		a = SIZE_MAX;
	return a;
}

static size_t
arg_sizes_max (struct letopt const *opt)
{
	size_t ret = 0;
	for (int i = 0, m = letopt_nargs(opt); i < m; ++i) {
		size_t n = __builtin_strlen(letopt_arg(opt, i));
		if (n > ret)
			ret = n;
	}
	return ret;
}

/**
 * @brief Calculate saturated sum of string argument lengths.
 *
 * The largest safe result is `SIZE_MAX - 1`. `SIZE_MAX` is always an
 * error condition and indicates that an overflow would be inevitable
 * if the result were to be used for string allocation.
 *
 * It is not possible to distinguish between an overflow and an actual
 * `SIZE_MAX` sum. Such information would be useless, however, because
 * either way there would be no space left for the null terminator.
 *
 * @param opt The option object whose string argument lengths to sum.
 *
 * @return The saturated sum of the argument string lengths.
 * @retval `0` if the argument list is empty or all arguments are empty.
 * @retval `SIZE_MAX` if the sum is greater than or equal to `SIZE_MAX`.
 */
static size_t
arg_sizes_sum (struct letopt const *opt)
{
	size_t ret = 0;
	for (int i = 0, m = letopt_nargs(opt); i < m; ++i) {
		ret = saturated_add_uz(ret,
		                       __builtin_strlen(letopt_arg(opt, i)));
		if (ret == SIZE_MAX)
			break;
	}
	return ret;
}

/**
 * @brief Unicode code point encoding.
 *
 * These serve a dual purpose: they act as identifiers for different
 * encodings, and their enumeration values themselves are defined as
 * something relevant to that specific encoding. `UCP_UTF8_*` values
 * are the number of bytes when encoding as UTF-8, `UCP_UTF32_*` the
 * maximum amount of data bits the corresponding code points use.
 */
fixed_enum(ucp_kind, uint32_t) {
	UCP_UTF8_1   =  1, //!< UTF-8 encoding, 1 byte
	UCP_UTF8_2   =  2, //!< UTF-8 encoding, 2 bytes
	UCP_UTF8_3   =  3, //!< UTF-8 encoding, 3 bytes
	UCP_UTF8_4   =  4, //!< UTF-8 encoding, 4 bytes
	UCP_UTF32_7  =  7, //!< 1-byte UTF-8 as UTF-32
	UCP_UTF32_11 = 11, //!< 2-byte UTF-8 as UTF-32
	UCP_UTF32_16 = 16, //!< 3-byte UTF-8 as UTF-32
	UCP_UTF32_21 = 21  //!< 4-byte UTF-8 as UTF-32
};

/** @brief UTF-32 conversion from 1-byte UTF-8. */
struct ucp_utf32_7 {
	uint32_t b0 :  7; //!< ASCII byte   [ 0: 6]
	uint32_t p0 : 25; //!<              [ 7:31]
};

/** @brief UTF-32 conversion from 2-byte UTF-8. */
struct ucp_utf32_11 {
	uint32_t b1 :  6; //!< Cont. byte 1 [ 0: 5]
	uint32_t b0 :  5; //!< Leading byte [ 6:11]
	uint32_t p0 : 21; //!<              [12:31]
};

/** @brief UTF-32 conversion from 3-byte UTF-8. */
struct ucp_utf32_16 {
	uint32_t b2 :  6; //!< Cont. byte 2 [ 0: 5]
	uint32_t b1 :  6; //!< Cont. byte 1 [ 6:11]
	uint32_t b0 :  4; //!< Leading byte [12:15]
	uint32_t p0 : 16; //!<              [16:31]
};

/** @brief UTF-32 conversion from 4-byte UTF-8. */
struct ucp_utf32_21 {
	uint32_t b3 :  6; //!< Cont. byte 3 [ 0: 5]
	uint32_t b2 :  6; //!< Cont. byte 2 [ 6:11]
	uint32_t b1 :  6; //!< Cont. byte 1 [12:17]
	uint32_t b0 :  3; //!< Leading byte [18:20]
	uint32_t p0 : 11; //!<              [21:31]
};

/** @brief 1-byte UTF-8 encoding. */
struct ucp_utf8_1 {
	uint32_t b0 :  7; //!< ASCII byte   [ 0: 6]
	uint32_t p1 : 25; //!<              [ 7:31]
};

/** @brief 2-byte UTF-8 encoding. */
struct ucp_utf8_2 {
	uint32_t b0 :  5; //!< Leading byte [ 0: 4]
	uint32_t p0 :  3; //!<              [ 5: 7]
	uint32_t b1 :  6; //!< Cont. byte 1 [ 8:13]
	uint32_t p1 : 18; //!<              [14:31]
};

/** @brief 3-byte UTF-8 encoding. */
struct ucp_utf8_3 {
	uint32_t b0 :  4; //!< Leading byte [ 0: 3]
	uint32_t p0 :  4; //!<              [ 4: 7]
	uint32_t b1 :  6; //!< Cont. byte 1 [ 8:13]
	uint32_t p1 :  2; //!<              [14:15]
	uint32_t b2 :  6; //!< Cont. byte 2 [16:21]
	uint32_t p2 : 10; //!<              [22:31]
};

/** @brief 4-byte UTF-8 encoding. */
struct ucp_utf8_4 {
	uint32_t b0 :  3; //!< Leading byte [ 0: 2]
	uint32_t p0 :  5; //!<              [ 3: 7]
	uint32_t b1 :  6; //!< Cont. byte 1 [ 8:13]
	uint32_t p1 :  2; //!<              [14:15]
	uint32_t b2 :  6; //!< Cont. byte 2 [16:21]
	uint32_t p2 :  2; //!<              [22:23]
	uint32_t b3 :  6; //!< Cont. byte 3 [24:29]
	uint32_t p3 :  2; //!<              [30:31]
};

/**
 * @brief Unicode code point structure.
 *
 * Union-based type for representing Unicode code points in
 * various encodings, moving data bits around between them,
 * and for use as by-value function arguments.
 */
union ucp {
	uint32_t            u32;     //!< Raw 32-bit data view.

	struct ucp_utf32_7  utf32_0; //!< 7b UTF-32/1B UTF-8 view.
	struct ucp_utf32_11 utf32_1; //!< 11b UTF-32/2B UTF-8 view.
	struct ucp_utf32_16 utf32_2; //!< 16b UTF-32/3B UTF-8 view.
	struct ucp_utf32_21 utf32_3; //!< 21b UTF-32/4B UTF-8 view.

	uint8_t             u8[4];   //!< Raw octets view.

	struct ucp_utf8_1   utf8_0;  //!< 1B UTF-8 view.
	struct ucp_utf8_2   utf8_1;  //!< 2B UTF-8 view.
	struct ucp_utf8_3   utf8_2;  //!< 3B UTF-8 view.
	struct ucp_utf8_4   utf8_3;  //!< 4B UTF-8 view.

	unsigned char       data[4]; //!< Raw bytes view.
	char                str[4];  //!< Raw characters view.
};

_Static_assert(sizeof(union ucp) * CHAR_BIT == 32U,"");

#if clang_at_least_version(19)
# define BAD_CODE_POINT (const union ucp){.u32 = UINT32_MAX}
#else
# define BAD_CODE_POINT (constexpr const union ucp){.u32 = UINT32_MAX}
#endif

static const_inline bool
code_point_error (const union ucp c)
{
	return c.u32 == BAD_CODE_POINT.u32;
}

static const_inline union ucp
utf8_to_utf32 (const union ucp c,
               const size_t    n)
{
	switch (n) {
	case 1:
		return (union ucp){
			.utf32_0.b0 = c.utf8_0.b0,
			.utf32_0.p0 = 0
		};
	case 2:
		return (union ucp){
			.utf32_1.b1 = c.utf8_1.b1,
			.utf32_1.b0 = c.utf8_1.b0,
			.utf32_1.p0 = 0
		};
	case 3:
		return (union ucp){
			.utf32_2.b2 = c.utf8_2.b2,
			.utf32_2.b1 = c.utf8_2.b1,
			.utf32_2.b0 = c.utf8_2.b0,
			.utf32_2.p0 = 0
		};
	case 4:
		return (union ucp){
			.utf32_3.b3 = c.utf8_3.b3,
			.utf32_3.b2 = c.utf8_3.b2,
			.utf32_3.b1 = c.utf8_3.b1,
			.utf32_3.b0 = c.utf8_3.b0,
			.utf32_3.p0 = 0
		};
	default:
		break;
	}

	return BAD_CODE_POINT;
}

static union ucp
utf8_code_point (struct utf8 *const u8p)
{
	uint8_t const *const d = (uint8_t const *)utf8_result(u8p);
	return utf8_to_utf32(
		(union ucp){.u8 = {d[0], d[1], d[2], d[3]}},
		utf8_size(u8p)
	);
}

int
main (int    c,
      char **v)
{
	struct letopt opt = letopt_init(c, v);

	if (letopt_nargs(&opt) < 1 || opt.m_help)
		letopt_helpful_exit(&opt);

	size_t buf_sz = opt.m_separate ? arg_sizes_max(&opt)
	                               : arg_sizes_sum(&opt);
	buf_sz = saturated_add_uz(
		saturated_add_uz(buf_sz, 2U),
		saturated_add_uz(buf_sz, buf_sz));

	if (buf_sz == SIZE_MAX) {
		pr_errno_(EOVERFLOW, "refusing to allocate buffer");
		(void)letopt_fini(&opt);
		return EXIT_FAILURE;
	}

	char *buf = malloc(buf_sz);
	if (!buf) {
		pr_errno_(errno, "malloc(%zu)", buf_sz);
		(void)letopt_fini(&opt);
		return EXIT_FAILURE;
	}

	struct utf8 u8p = utf8();
	uint64_t n[] = {0,0,0,0};
	//char *out = buf;

	for (int i = 0; i < letopt_nargs(&opt); ++i) {
		uint8_t const *p = (uint8_t const *)letopt_arg(&opt, i);
		for (uint8_t const *q = p; *q; q = p) {
			p = utf8_parse_next_code_point(&u8p, q);
			if (u8p.error) {
				if (opt.m_replace && (*p || opt.m_separate)) {
					const union ucp uc = utf8_to_utf32(
						(union ucp){
							.u8 = {0xefU, 0xbfU,
							       0xbdU, 0x00U}
						},
						3U
					);
					printf("U+%04" PRIx32 "\n", uc.u32);
/*
					*out++ = (char)(unsigned char)0xefU;
					*out++ = (char)(unsigned char)0xbfU;
					*out++ = (char)(unsigned char)0xbdU;
*/
				}
				if (!*p)
					break;
				if (p == q && utf8_expects_leading_byte(&u8p))
					++p;
				utf8_reset(&u8p);
				continue;
			}

			const union ucp uc = utf8_code_point(&u8p);
			if (!code_point_error(uc))
				printf("U+%04" PRIx32 "\n", uc.u32);
		}

		if (opt.m_separate) {
/*
			if (out == buf || *(out - 1) != '\n')
				*out++ = '\n';
			*out = '\0';
			out = buf;
			(void)printf("%" PRIu64 "\t%" PRIu64 "\t%" PRIu64
			             "\t%" PRIu64 "\t%" PRIu64 "\t%s",
			             n[0], n[1], n[2], n[3], n[0] +
			             2U*n[1] + 3U*n[2] + 4U*n[3], buf);
*/
			__builtin_memset(&n[0], 0, sizeof n);
			utf8_reset(&u8p);
		}
	}
/*
	if (!opt.m_separate) {
		if (out == buf || *(out - 1) != '\n')
			*out++ = '\n';
		*out = '\0';
		(void)printf("%" PRIu64 "\t%" PRIu64 "\t%" PRIu64
		             "\t%" PRIu64 "\t%" PRIu64 "\t%s",
		             n[0], n[1], n[2], n[3], n[0] +
		             2U*n[1] + 3U*n[2] + 4U*n[3], buf);
	}
*/
	free(buf);
	return letopt_fini(&opt);
}
