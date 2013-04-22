/*
 * Copyright (C) 2013 Ron Pedde <ron@pedde.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef enum internal_type_t {
    TYPE_UNKNOWN=0,
    TYPE_DIR,
    TYPE_FILE,
} internal_type_t;

typedef struct client_t {
    int fd;
    int state;
    internal_type_t request_type;
    char *request;
    char *full_path;
    struct bufferevent *buf_ev;
    void *opaque_client;
} client_t;

extern int register_module(char *name,
                           void (*dispatch_fn)(client_t *client,
                                               char *resource));

extern void handle_error(client_t *client, internal_type_t type, char *text);

#endif /* _PLUGIN_H_ */
