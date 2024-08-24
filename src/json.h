/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file json.h
 *
 * @author Juuso Alasuutari
 */
#ifndef LIBCANTH_SRC_JSON_H_
#define LIBCANTH_SRC_JSON_H_

#include <stddef.h>
#include <stdint.h>

#include "util.h"

fixed_enum(json_type, uint8_t) {
	json_false,
	json_true,
	json_null,
	json_number,
	json_string,
	json_array,
	json_object
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

extern void
json_parse (char const *s);

#endif /* LIBCANTH_SRC_JSON_H_ */