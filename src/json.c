/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file json.c
 *
 * @author Juuso Alasuutari
 */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "dbg.h"
#include "util.h"

#ifdef __INTELLISENSE__
# define C23(...)
#else
# define C23(...) __VA_ARGS__
#endif

enum json_type C23(: uint8_t) {
	json_false,
	json_true,
	json_null,
	json_number,
	json_string,
	json_array,
	json_object
};

enum json_byte C23(: uint8_t) {
	json_1_9 = 1U,
	json_dig = json_1_9 << 1U,
	json_esc = json_dig << 1U,
	json_exp = json_esc << 1U,
	json_hex = json_exp << 1U,
	json_sig = json_hex << 1U,
	json_ws  = json_sig << 1U,
};

struct json_w {
	unsigned char const *buf;
	size_t               len;
};

struct json_arg {
	uint8_t const *ptr;
	uint8_t const *end;
	struct json_w *ctx;
};

struct json_ret {
	uint64_t size : 48;
	uint64_t type :  8;
	uint64_t code :  8;
};

static void
json_parse (char const *str);

int
main (int    c,
      char **v)
{
	for (int i = 0; ++i < c;) {
		json_parse(v[i]);
	}
}

constexpr static const uint8_t lut[256] = {
	['\t'] = json_ws,
	['\n'] = json_ws,
	['\r'] = json_ws,
	[' ' ] = json_ws,

	['"' ] = json_esc,

	['+' ] = json_sig,
	['-' ] = json_sig,

	['/' ] = json_esc,

	['0' ] = json_dig,
	['1' ] = json_dig| json_1_9,
	['2' ] = json_dig| json_1_9,
	['3' ] = json_dig| json_1_9,
	['4' ] = json_dig| json_1_9,
	['5' ] = json_dig| json_1_9,
	['6' ] = json_dig| json_1_9,
	['7' ] = json_dig| json_1_9,
	['8' ] = json_dig| json_1_9,
	['9' ] = json_dig| json_1_9,

	['A' ] = json_hex,
	['B' ] = json_hex,
	['C' ] = json_hex,
	['D' ] = json_hex,
	['E' ] = json_hex| json_exp,
	['F' ] = json_hex,

	['\\'] = json_esc,

	['a' ] = json_hex,
	['b' ] = json_hex| json_esc,
	['c' ] = json_hex,
	['d' ] = json_hex,
	['e' ] = json_hex| json_exp,
	['f' ] = json_hex| json_esc,

	['n' ] = json_esc,
	['r' ] = json_esc,
	['t' ] = json_esc,
	['u' ] = json_esc,
};

static force_inline uint8_t const *
skip_ws (struct json_arg arg)
{
	for (; arg.ptr < arg.end && (lut[*arg.ptr] & json_ws); ++arg.ptr);
	return arg.ptr;
}

static struct json_ret
parse_integer (struct json_arg arg)
{
	return (struct json_ret){0};
}

static struct json_ret
parse_number (struct json_arg arg)
{
	return (struct json_ret){0};
}

static struct json_ret
parse_array (struct json_arg arg)
{
	return (struct json_ret){0};
}

static struct json_ret
parse_object (struct json_arg arg)
{
	return (struct json_ret){0};
}

static struct json_ret
parse_string (struct json_arg arg)
{
	return (struct json_ret){0};
}

static force_inline struct json_ret
parse_digits (struct json_arg arg)
{
	return (struct json_ret){0};
}

static force_inline struct json_ret
parse_neg_int (struct json_arg arg)
{
	struct json_ret ret = {
		.size = 1,
		.type = json_number,
		.code = 0
	};

	if (++arg.ptr >= arg.end) {
		ret.code = ENODATA;
		return ret;
	}

	uint8_t flags = lut[*arg.ptr];

	if (!(flags & json_dig)) {
		ret.code = EINVAL;
		return ret;
	}

	if (flags & json_1_9)
		ret = parse_integer(arg);

	ret.size++;
	return ret;
}

static force_inline struct json_ret
parse_false (struct json_arg arg)
{
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

		ret.code = *p ? EINVAL : ENODATA;
	} while (0);

	ret.size = (uint64_t)(ptrdiff_t)(p - arg.ptr);
	return ret;
}

static force_inline struct json_ret
parse_null (struct json_arg arg)
{
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

		ret.code = *p ? EINVAL : ENODATA;
	} while (0);

	ret.size = (uint64_t)(ptrdiff_t)(p - arg.ptr);
	return ret;
}

static struct json_ret
parse_true (struct json_arg arg)
{
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

		ret.code = *p ? EINVAL : ENODATA;
	} while (0);

	ret.size = (uint64_t)(ptrdiff_t)(p - arg.ptr);
	return ret;
}

static struct json_ret
parse_value (struct json_arg arg)
{
	struct json_ret ret = {0};

	switch (*arg.ptr) {
	case '"':
		ret = parse_string(arg);
		break;
	case '-':
		ret = parse_neg_int(arg);
		break;
	case '0':
		ret.size = 1;
		ret.type = json_number;
		break;
	case '1': case '2': case '3':
	case '4': case '5': case '6':
	case '7': case '8': case '9':
		ret = parse_integer(arg);
		break;
	case '[':
		ret = parse_array(arg);
		break;
	case 'f':
		ret = parse_false(arg);
		break;
	case 'n':
		ret = parse_null(arg);
		break;
	case 't':
		ret = parse_true(arg);
		break;
	case '{':
		ret = parse_object(arg);
		break;
	}

	return ret;
}

static struct json_ret
parse_element (struct json_arg arg)
{
		arg.ptr = skip_ws(arg);
		struct json_ret ret = parse_value(arg);
		arg.ptr = skip_ws(arg);
}

static void
json_parse (char const *str)
{
	if (!str)
		return;

	struct json_w w = {
		.buf = (void const *)str,
		.len = strlen(str),
	};

	struct json_ret ret = parse_value((struct json_arg){
		.ptr = (uint8_t const *)w.buf,
		.end = &w.buf[w.len],
		.ctx = &w,
	});

	if (ret.code) {
		pr_errno(ret.code, "json_parse");
	}
}
