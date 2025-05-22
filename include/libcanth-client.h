/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file libcanth-client.h
 *
 * @author Juuso Alasuutari
 */
#if defined LIBCANTH_API && ! defined LIBCANTH_CLIENT_H_
#define LIBCANTH_CLIENT_H_

#include <libcanth-plugin.h>

#ifdef __cplusplus
extern "C" {
#endif

struct libcanth_client;

#ifndef __cplusplus
typedef struct libcanth_client libcanth_client;
#endif

/**
 * @brief Client object
 */
struct libcanth_client {
	libcanth_list  hook; //!< Client list hook.
	libcanth_list *head; //!< Pointer to owner.
};

LIBCANTH_API extern int
libcanth_client_quit (libcanth_client *client);

LIBCANTH_API extern libcanth_client *
libcanth_client_next (libcanth_client *client);

LIBCANTH_API extern libcanth_client *
libcanth_client_prev (libcanth_client *client);

#ifdef __cplusplus
}
#endif

#endif /* LIBCANTH_API && ! LIBCANTH_CLIENT_H_ */
