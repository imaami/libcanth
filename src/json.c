/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file json.c
 *
 * @author Juuso Alasuutari
 */
#include <errno.h>
#include <string.h>

#include "dbg.h"
#include "json_internal.h"

/*
 * <json>       ::= <element>
 * <element>    ::= <ws> <value> <ws>
 * <value>      ::= <object> | <array> | <string> | <number> | "true" | "false" | "null"
 * <object>     ::= "{" <ws> "}" | "{" <members> "}"
 * <members>    ::= <member> | <member> "," <members>
 * <member>     ::= <ws> <string> <ws> ":" <element>
 * <array>      ::= "[" <ws> "]" | "[" <elements> "]"
 * <elements>   ::= <element> | <element> "," <elements>
 * <string>     ::= "\"" <characters> "\""
 * <characters> ::= "" | <character> <characters>
 * <character>  ::= <unescaped> | <escaped>
 * <unescaped>  ::= <any Unicode character except " or \ or control characters>
 * <escaped>    ::= "\\" <escape>
 * <escape>     ::= "\"" | "/" | "b" | "f" | "n" | "r" | "t" | "u" <hex> <hex> <hex> <hex>
 * <number>     ::= <int> <frac> <exp>
 * <int>        ::= <digit> | <onenine> <digits> | "-" <digit> | "-" <onenine> <digits>
 * <digits>     ::= <digit> | <digit> <digits>
 * <digit>      ::= "0" | <onenine>
 * <onenine>    ::= "1" | "2" | "3" | "4" | "5" | "6" | "7" | "8" | "9"
 * <frac>       ::= "" | "." <digits>
 * <exp>        ::= "" | "e" <sign> <digits> | "E" <sign> <digits>
 * <sign>       ::= "" | "+" | "-"
 * <ws>         ::= "" | " " <ws> | "\t" <ws> | "\n" <ws> | "\r" <ws>
 */

/**
 * @brief Parse the fractional and exponential parts of a number.
 *
 * Parse the fractional (dot and digits) and exponential (e/E with
 * optional sign and digits) parts of a number, if any.
 *
 * Corresponds to the regex `(\.[0-9]+)?([Ee][\+-]?[0-9]+)?`.
 *
 * @param arg JSON parser input. @ref json_arg::ptr must point to
 *            the first non-digit following `-?(0|[1-9][0-9]*)`.
 * @return JSON parsing result. @ref json_ret::code will be 0 on
 *         success, `ENODATA` on unexpected end of input buffer,
 *         and `EILSEQ` on JSON syntax error.
 */
static struct json_ret
json_parse_frc_exp (struct json_arg arg)
{
	int e = 0;
	uint8_t const *p = arg.ptr;

	if (p < arg.end) do {
		if (*p == '.') {
			if (++p >= arg.end) {
				// last character is '.'
				e = ENODATA;
				break;
			}

			if (!(json_lut[*p] & json_dig)) {
				// non-digit follows '.'
				e = EILSEQ;
				break;
			}

			p = json_skip_digits(&p[1], arg.end);
		}

		if (p < arg.end && (json_lut[*p] & json_exp)) {
			if (++p >= arg.end ||
			    ((json_lut[*p] & json_sig) && ++p >= arg.end)) {
				// last character is [Ee+-]
				e = ENODATA;
				break;
			}

			if (!(json_lut[*p] & json_dig)) {
				// non-digit follows [Ee][+-]?
				e = EILSEQ;
				break;
			}

			p = json_skip_digits(&p[1], arg.end);
		}
	} while (0);

	return (struct json_ret){
		.size = (uint64_t)(ptrdiff_t)(p - arg.ptr),
		.type = json_number,
		.code = (uint64_t)e
	};
}

static force_inline struct json_ret
json_parse_num (struct json_arg arg)
{
	uint8_t const *p = json_skip_digits(&arg.ptr[1], arg.end);
	struct json_ret ret = json_parse_frc_exp((struct json_arg){
		.ptr = p,
		.end = arg.end,
		.ctx = arg.ctx,
	});
	ret.size += (uint64_t)(ptrdiff_t)(p - arg.ptr);
	return ret;
}

static struct json_ret
json_parse_array (useless struct json_arg arg)
{
	return (struct json_ret){
		.size = 0,
		.type = json_array,
		.code = ENOSYS
	};
}

static struct json_ret
json_parse_object (useless struct json_arg arg)
{
	return (struct json_ret){
		.size = 0,
		.type = json_object,
		.code = ENOSYS
	};
}

static struct json_ret
json_parse_string (useless struct json_arg arg)
{
	return (struct json_ret){
		.size = 0,
		.type = json_string,
		.code = ENOSYS
	};
}

/**
 * @brief Parse a negative number.
 *
 * On entry, `arg.ptr` points to the '-' character.
 *
 * @param arg JSON argument.
 * @return JSON return value.
 */
static force_inline struct json_ret
json_parse_neg (struct json_arg arg)
{
	struct json_ret ret = {
		.size = 1,
		.type = json_number,
		.code = 0
	};

	uint8_t const *p = &arg.ptr[1];
	if (p >= arg.end) {
		// last character is '-'
		ret.code = ENODATA;
		return ret;
	}

	uint8_t flags = json_lut[*p];

	if (!(flags & json_dig)) {
		// non-digit follows '-'
		ret.code = EILSEQ;
		return ret;
	}

	++p;

	if (flags & json_1_9)
		// -[1-9][0-9]* (instead of -0)
		p = json_skip_digits(p, arg.end);

	ret = json_parse_frc_exp((struct json_arg){
		.ptr = p,
		.end = arg.end,
		.ctx = arg.ctx,
	});
	if (!(flags & json_1_9)) {
		if (ret.size == 0 && p < arg.end
		    && (json_lut[*p] & json_dig))
			// digit follows "-0"
			ret.code = EILSEQ;
	}
	ret.size += (uint64_t)(ptrdiff_t)(p - arg.ptr);
	return ret;
}

#ifdef ALT_PARSING
/**
 * @brief Create a @ref json_ret with a @ref json_ret::size constrained to
 *        the minimum of the remaining buffer size and `ret.size`.
 *
 * @param arg @ref json_arg whose unread byte count to use as a constraint.
 * @param ret @ref json_ret from which to copy the @ref json_ret::type and
 *            @ref json_ret::code fields, and whose @ref json_ret::size to
 *            use as a constraint.
 *
 * @return A @ref json_ret which is otherwise identical to `ret`, but with
 *         @ref json_ret::size constrained to the minimum of the remaining
 *         buffer size and `ret.size`.
 */
static const_inline struct json_ret
json_ret_max (const struct json_arg arg,
              const struct json_ret ret)
{
	uint64_t size = (uint64_t)(ptrdiff_t)(arg.end - arg.ptr);
	return (struct json_ret){
		.size = ret.size < size ? ret.size : size,
		.type = ret.type,
		.code = ret.code
	};
}
#endif /* ALT_PARSING */

static struct json_ret
json_parse_false (struct json_arg arg)
{
	#ifdef ALT_PARSING
	struct json_ret ret = json_ret_max(arg, (struct json_ret){
		.size = sizeof "false" - 1U,
		.type = json_false,
		.code = 0
	});

	unsigned i = 0;
	char const *const str = (char const *)arg.ptr;
	while (i < ret.size && str[i] == "false"[i]) {
		++i;
	}

	if (i < ret.size) {
		ret.size = i;
		ret.code = EILSEQ;
	} else if (i < sizeof "false" - 1U)
		ret.code = ENODATA;

	#else /* ALT_PARSING */
	uint8_t const *p = arg.ptr;
	struct json_ret ret = {
		.size = 0,
		.type = json_false,
		.code = 0
	};

	do {
		if ((ptrdiff_t)(arg.end - arg.ptr) < 5) {
			if (!(++p < arg.end && (*p != 'a' ||
			     (++p < arg.end && (*p != 'l' ||
			     (++p < arg.end && (*p != 's' ||
			      ++p < arg.end))))))) {
				ret.code = ENODATA;
				break;
			}

		} else if (*++p == 'a' &&
		           *++p == 'l' &&
		           *++p == 's' &&
		           *++p == 'e') {
			++p;
			break;
		}

		ret.code = *p ? EILSEQ : ENODATA;
	} while (0);

	ret.size = (uint64_t)(ptrdiff_t)(p - arg.ptr);
	#endif /* ALT_PARSING */

	return ret;
}

static struct json_ret
json_parse_null (struct json_arg arg)
{
	#ifdef ALT_PARSING
	struct json_ret ret = json_ret_max(arg, (struct json_ret){
		.size = sizeof "null" - 1U,
		.type = json_null,
		.code = 0
	});

	unsigned i = 0;
	char const *const str = (char const *)arg.ptr;
	while (i < ret.size && str[i] == "null"[i]) {
		++i;
	}

	if (i < ret.size) {
		ret.size = i;
		ret.code = EILSEQ;
	} else if (i < sizeof "null" - 1U)
		ret.code = ENODATA;

	#else /* ALT_PARSING */
	uint8_t const *p = arg.ptr;
	struct json_ret ret = {
		.size = 0,
		.type = json_null,
		.code = 0
	};

	do {
		if ((ptrdiff_t)(arg.end - arg.ptr) < 4) {
			if (!(++p < arg.end && (*p != 'u' ||
			     (++p < arg.end && (*p != 'l' ||
			      ++p < arg.end))))) {
				ret.code = ENODATA;
				break;
			}

		} else if (*++p == 'u' &&
		           *++p == 'l' &&
		           *++p == 'l') {
			++p;
			break;
		}

		ret.code = *p ? EILSEQ : ENODATA;
	} while (0);

	ret.size = (uint64_t)(ptrdiff_t)(p - arg.ptr);
	#endif /* ALT_PARSING */

	return ret;
}

static struct json_ret
json_parse_true (struct json_arg arg)
{
	#ifdef ALT_PARSING
	struct json_ret ret = json_ret_max(arg, (struct json_ret){
		.size = sizeof "true" - 1U,
		.type = json_true,
		.code = 0
	});

	unsigned i = 0;
	char const *const str = (char const *)arg.ptr;
	while (i < ret.size && str[i] == "true"[i]) {
		++i;
	}

	if (i < ret.size) {
		ret.size = i;
		ret.code = EILSEQ;
	} else if (i < sizeof "true" - 1U)
		ret.code = ENODATA;

	#else /* ALT_PARSING */
	uint8_t const *p = arg.ptr;
	struct json_ret ret = {
		.size = 0,
		.type = json_true,
		.code = 0
	};

	do {
		if ((ptrdiff_t)(arg.end - arg.ptr) < 4) {
			if (!(++p < arg.end && (*p != 'r' ||
			     (++p < arg.end && (*p != 'u' ||
			      ++p < arg.end))))) {
				ret.code = ENODATA;
				break;
			}

		} else if (*++p == 'r' &&
		           *++p == 'u' &&
		           *++p == 'e') {
			++p;
			break;
		}

		ret.code = *p ? EILSEQ : ENODATA;
	} while (0);

	ret.size = (uint64_t)(ptrdiff_t)(p - arg.ptr);
	#endif /* ALT_PARSING */

	return ret;
}

static struct json_ret
json_parse_value (struct json_arg arg)
{
	struct json_ret ret = {0};

	switch (*arg.ptr) {
	case '"':
		ret = json_parse_string(arg);
		break;
	case '-':
		// -(0|[1-9][0-9]*)(\.[0-9]+)?([Ee][\+-]?[0-9]+)?
		ret = json_parse_neg(arg);
		break;
	case '0':
		// 0(\.[0-9]+)?([Ee][\+-]?[0-9]+)?
		++arg.ptr;
		ret = json_parse_frc_exp(arg);
		if (ret.size == 0 && arg.ptr < arg.end
		    && (json_lut[*arg.ptr] & json_dig))
			// digit follows '0'
			ret.code = EILSEQ;
		++ret.size;
		break;
	case '1': case '2': case '3':
	case '4': case '5': case '6':
	case '7': case '8': case '9':
		// [1-9][0-9]*(\.[0-9]+)?([Ee][\+-]?[0-9]+)?
		ret = json_parse_num(arg);
		break;
	case '[':
		ret = json_parse_array(arg);
		break;
	case 'f':
		ret = json_parse_false(arg);
		break;
	case 'n':
		ret = json_parse_null(arg);
		break;
	case 't':
		ret = json_parse_true(arg);
		break;
	case '{':
		ret = json_parse_object(arg);
		break;
	default:
		ret.code = EILSEQ;
	}

	return ret;
}

static void
json_describe (struct json_ret  ret,
               uint8_t const   *ptr,
               size_t           off,
               char const      *func)
{
	if (ret.code) {
		pr_errno_(ret.code, "%s: at offset %zu",
		          func, off + (size_t)ret.size);
		return;
	}

	switch (ret.type) {
	case json_false:
	case json_true:
	case json_null:
	case json_number:
	case json_string:
	case json_array:
	case json_object:
		if (ret.size)
			pr_("%.*s\n", (int)ret.size, &ptr[off]);
		break;
	default:
		break;
	}
}

static struct json_ret
json_parse_element (struct json_arg arg)
{
	uint8_t const *ptr = json_skip_ws(arg.ptr, arg.end);
	struct json_ret ret = json_parse_value((struct json_arg){
		.ptr = ptr,
		.end = arg.end,
		.ctx = arg.ctx,
	});
	json_describe(ret, arg.ptr, (size_t)(ptr - arg.ptr), __func__);
	ptr = &ptr[ret.size];
	if (!ret.code)
		ptr = json_skip_ws(ptr, arg.end);
	ret.size = (uint64_t)(ptrdiff_t)(ptr - arg.ptr);
	return ret;
}

void
json_parse (char const *str)
{
	if (!str)
		return;

	struct json_w w = {
		.buf = (void const *)str,
		.len = strlen(str),
	};

	struct json_ret ret = json_parse_element((struct json_arg){
		.ptr = (uint8_t const *)w.buf,
		.end = &w.buf[w.len],
		.ctx = &w,
	});

	(void)ret;
}
