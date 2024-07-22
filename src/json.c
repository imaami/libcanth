#include <stdint.h>
#include <string.h>

#include "util.h"

enum json_type : uint8_t {
	json_false,
	json_true,
	json_null,
	json_number,
	json_string,
	json_array,
	json_object
};

enum json_byte : uint8_t {
	json_1_9 = 1U,
	json_dig = json_1_9 << 1U,
	json_esc = json_dig << 1U,
	json_exp = json_esc << 1U,
	json_hex = json_exp << 1U,
	json_sig = json_hex << 1U,
	json_ws  = json_sig << 1U,
};

struct json_w;

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

struct json_w {
	uint8_t const *buf;
	size_t         len;
};

static void json_parse (char const *str);

int
main (int    c,
      char **v)
{
	for (int i = 0; ++i < c;) {
		json_parse(v[i]);
	}
}

static uint8_t const *
skip_ws (uint8_t const       *p,
         uint8_t const *const e);

static struct json_ret
parse_value (struct json_arg w);

static void
json_parse (char const *str)
{
	if (!str)
		return;

	struct json_w w = {
		.buf = (void const *)str,
		.end = strlen(str),
	};

	ptr = parse_value(ptr, end);
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
skip_ws (uint8_t const       *p,
         uint8_t const *const e)
{
	for (; p < e && (lut[*p] & json_ws); ++p);
	return p;
}

static uint8_t const *
parse_integer (uint8_t const       *p,
               uint8_t const *const e)
{
	return p;
}

static uint8_t const *
parse_number (uint8_t const       *p,
              uint8_t const *const e)
{
	return p;
}

static uint8_t const *
parse_array (uint8_t const       *p,
             uint8_t const *const e)
{
	return p;
}

static uint8_t const *
parse_object (uint8_t const       *p,
              uint8_t const *const e)
{
	return p;
}

static uint8_t const *
parse_string (uint8_t const       *p,
              uint8_t const *const e)
{
	return p;
}

static force_inline uint8_t const *
parse_dash (uint8_t const       *p,
            uint8_t const *const e)
{
	return p;
}

static force_inline uint8_t const *
parse_false (uint8_t const       *p,
             uint8_t const *const e)
{
	if ((ptrdiff_t)(e - p) < 5) {
		if (++p < e && *p == 'a' &&
		    ++p < e && *p == 'l' &&
		    ++p < e && *p == 's' &&
		    ++p < e && *p == 'e') {
			++p;
		}
	} else if (*++p == 'a' &&
	           *++p == 'l' &&
	           *++p == 's' &&
	           *++p == 'e') {
		++p;
	}
	return p;
}

static force_inline uint8_t const *
parse_null (uint8_t const       *p,
            uint8_t const *const e)
{
	if ((ptrdiff_t)(e - p) < 4) {
		if (++p < e && *p == 'u' &&
		    ++p < e && *p == 'l' &&
		    ++p < e && *p == 'l') {
			++p;
		}
	} else if (*++p == 'u' &&
	           *++p == 'l' &&
	           *++p == 'l') {
		++p;
	}
	return p;
}

static force_inline struct json_ret
parse_true (uint8_t const       *p,
            uint8_t const *const e)
{
	bool done = (ptrdiff_t)(e - p) > 3;
	int error;

	if (!done) {
		error = ++p < e && (*p != 'r' ||
		      ( ++p < e && (*p != 'u' ||
		      ( ++p < e && (*p != 'e'))));
	} else {
		error = *++p != 'r' ||
		        *++p != 'u' ||
		        *++p != 'e';
		++p;
	}
	return p;
}

static uint8_t const *
parse_value (uint8_t const       *p,
             uint8_t const *const e)
{
	uint8_t const *q = p;

	switch (*p++) {
	case '"':
		p = parse_string(p, e);
		break;
	case '-':
		p = parse_dash(p, e);
		break;
	case '0':
		p++;
		break;
	case '1': case '2': case '3':
	case '4': case '5': case '6':
	case '7': case '8': case '9':
		q = parse_number(p, e);
		break;
	case '[':
		q = parse_array(p, e);
		break;
	case 'f':
		q = parse_false(p, e);
		break;
	case 'n':
		q = parse_null(p, e);
		break;
	case 't':
		q = parse_true(p, e);
		break;
	case '{':
		q = parse_object(p, e);
		break;
	}

	return p;
}
