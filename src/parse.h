/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file parse.h
 * @brief String parsing
 * @author Juuso Alasuutari
 */
#ifndef LIBCANTH_SRC_PARSE_H_
#define LIBCANTH_SRC_PARSE_H_

#include <stdint.h>

#include "compat.h"

struct parsed {
	union {
		uint64_t u64;
		int64_t  i64;
	};
	int64_t err;
};

extern struct parsed
parse_u64 (char *str);

extern bool
parsed_ok (struct parsed  par,
           char const    *msg);

#endif /* LIBCANTH_SRC_PARSE_H_ */
