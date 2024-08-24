/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file json_internal.h
 *
 * @author Juuso Alasuutari
 */
#ifndef LIBCANTH_SRC_JSON_INTERNAL_H_
#define LIBCANTH_SRC_JSON_INTERNAL_H_

#include "json.h"

/**
 * @brief JSON parser internal function type.
 * @param arg JSON parsing input.
 * @return JSON parsing result.
 */
typedef struct json_ret json_fn (struct json_arg arg);

/**
 * @brief JSON parsing flags.
 */
fixed_enum(json_flag, uint8_t) {
	json_1_9 = 1U,
	json_dig = json_1_9 << 1U,
	json_esc = json_dig << 1U,
	json_exp = json_esc << 1U,
	json_frc = json_exp << 1U,
	json_hex = json_frc << 1U,
	json_sig = json_hex << 1U,
	json_ws  = json_sig << 1U,
};

/**
 * @brief Lookup table for parsing JSON.
 */
constexpr static const uint8_t json_lut[256] = {
	['\t'] = json_ws,
	['\n'] = json_ws,
	['\r'] = json_ws,
	[' ' ] = json_ws,

	['"' ] = json_esc,

	['+' ] = json_sig,
	['-' ] = json_sig,

	['.' ] = json_frc,

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

/**
 * @brief Skip digits.
 *
 * Skip decimal digit characters until a non-digit character is found
 * or the end of the buffer is reached. Return a pointer to the first
 * non-digit character.
 *
 * Digits are as defined by the JSON standard, i.e. the characters '0'
 * to '9'.
 *
 * @note No bounds checking or input validation is performed. `ptr` and
 *       `end` must be valid pointers within the same contiguous buffer.
 *
 * @param ptr Pointer to the current position. Must be within a valid
 *            buffer and not larger than `end`.
 * @param end Pointer to the end of the buffer. Must not be larger than
 *            a pointer to one past the last element of a valid buffer.
 * @return Pointer to the first encountered non-decimal digit character.
 */
nonnull_in() nonnull_out
static force_inline uint8_t const *
json_skip_digits (uint8_t const *      ptr,
                  uint8_t const *const end)
{
	for (; ptr < end && (json_lut[*ptr] & json_dig); ++ptr);
	return ptr;
}

/**
 * @brief Skip whitespace characters.
 *
 * Skip whitespace characters until a non-whitespace character is found
 * or the end of the buffer is reached, whichever comes first. Return a
 * pointer to the first non-whitespace character.
 *
 * Whitespace characters are as defined by the JSON standard, i.e. tab,
 * newline, carriage return, and space.
 *
 * @note No bounds checking or input validation is performed. `ptr` and
 *       `end` must be valid pointers within the same contiguous buffer.
 *
 * @param ptr Pointer to the current position. Must be within a valid
 *            buffer and not larger than `end`.
 * @param end Pointer to the end of the buffer. Must not be larger than
 *            a pointer to one past the last element of a valid buffer.
 * @return Pointer to the first encountered non-whitespace character.
 */
nonnull_in() nonnull_out
static force_inline uint8_t const *
json_skip_ws (uint8_t const *      ptr,
              uint8_t const *const end)
{
	for (; ptr < end && (json_lut[*ptr] & json_ws); ++ptr);
	return ptr;
}

#endif /* LIBCANTH_SRC_JSON_INTERNAL_H_ */
