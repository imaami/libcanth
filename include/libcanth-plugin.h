/* SPDX-License-Identifier: LGPL-3.0-or-later */
/** @file libcanth-plugin.h
 *
 * @author Juuso Alasuutari
 */
#if defined LIBCANTH_API && ! defined LIBCANTH_PLUGIN_H_
#define LIBCANTH_PLUGIN_H_

#include <libcanth-server.h>

#ifdef __cplusplus
extern "C" {
#endif

struct libcanth_plugin;
struct libcanth_plugin_host;

#ifndef __cplusplus
typedef struct libcanth_plugin      libcanth_plugin;
typedef struct libcanth_plugin_host libcanth_plugin_host;
#endif

typedef int libcanth_plugin_func(void);

/**
 * @brief Plugin base object.
 */
struct libcanth_plugin {
	libcanth_plugin_host *host;
	char const           *name;
	char const           *version;
	libcanth_plugin_func *on_load;
	libcanth_plugin_func *on_yeet;
};

LIBCANTH_API extern int
libcanth_plugin_initialize (libcanth_plugin *plugin,
                            int              error);

LIBCANTH_API extern int
libcanth_plugin_ask_unload (libcanth_plugin *plugin);

LIBCANTH_API extern int
libcanth_plugin_add_server (libcanth_plugin *plugin,
                            libcanth_server *server);

LIBCANTH_API extern int
libcanth_plugin_del_server (libcanth_plugin *plugin,
                            libcanth_server *server);

LIBCANTH_API extern int
libcanth_plugin_next_server (libcanth_plugin  *plugin,
                             libcanth_server **server);

#ifdef __cplusplus
}
#endif

#endif /* LIBCANTH_API && ! LIBCANTH_PLUGIN_H_ */
