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

/**
 * @brief Copy a @ref json_ret while constraining its @ref json_ret::size.
 *
 * If the remaining buffer size calculated from `arg` is not less than the
 * @ref json_ret::size of `ret`, the return value is the unmodified `ret`.
 * If the remaining buffer size is less than that, the result is otherwise
 * identical except @ref json_ret::size contains the remaining buffer size.
 *
 * @param arg A @ref json_arg for calculating the remaining buffer size.
 * @param ret The @ref json_ret to copy and possibly constrain.
 *
 * @return A copy of `ret`, possibly with a smaller @ref json_ret::size.
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

/**
 * @brief Parse a JSON keyword.
 * @param arg JSON argument.
 * @param kwd JSON keyword.
 * @return JSON return value.
 */
static struct json_ret __attribute__((pure))
json_parse_keyword (const struct json_arg arg,
                    const struct json_kwd kwd)
{
	struct json_ret ret = json_ret_max(arg, (struct json_ret){
		.size = kwd.size,
		.type = kwd.type,
		.code = 0
	});

	unsigned i = 0;
	char const *const str = (char const *)arg.ptr;
	while (i < ret.size && str[i] == kwd.str[i]) {
		++i;
	}

	if (i < ret.size) {
		ret.size = i;
		ret.code = EILSEQ;

	} else if (i < kwd.size) {
		ret.code = ENODATA;
	}

	return ret;
}

/* Don't even try. Just accept it and move on. */
#define json_define_kw(x)      static force_inline struct json_ret \
json_parse_##x(struct json_arg arg){return json_parse_keyword(arg, \
(struct json_kwd){json_##x, sizeof #x - 1U, #x});} _Static_assert( \
sizeof #x > 1U, "keyword is empty"); _Static_assert(sizeof #x - 1U \
<= sizeof ((struct json_kwd *)0)->str, "keyword exceeds json_kwd")
json_define_kw(false); json_define_kw(null); json_define_kw(true);
#undef json_define_kw

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
