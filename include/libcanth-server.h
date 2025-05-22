/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file libcanth-server.h
 *
 * @author Juuso Alasuutari
 */
#if defined LIBCANTH_API && ! defined LIBCANTH_SERVER_H_
#define LIBCANTH_SERVER_H_

#include <libcanth-plugin.h>

#ifdef __cplusplus
extern "C" {
#endif

struct libcanth_server;

#ifndef __cplusplus
typedef struct libcanth_server libcanth_server;
#endif

/**
 * @brief Server (i.e. plugin instance) object
 */
struct libcanth_server {
	libcanth_list  hook;
	libcanth_host *host;
};

LIBCANTH_API extern int
libcanth_server_quit (libcanth_server *server);

LIBCANTH_API extern libcanth_server *
libcanth_server_next (libcanth_server *server);

LIBCANTH_API extern libcanth_server *
libcanth_server_prev (libcanth_server *server);

#ifdef __cplusplus
}
#endif

#endif /* LIBCANTH_API && ! LIBCANTH_SERVER_H_ */
