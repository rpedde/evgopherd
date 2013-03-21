/*
 * Copyright (C) 2013 Ron Pedde (ron@pedde.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdint.h>

#ifndef TRUE
# define TRUE  1
# define FALSE 0
#endif

typedef struct gopher_conf_t {
    char *config_file;
    uint16_t port;
    char *base_dir;
    int debug_level;
    int drop_core;
    int socket_backlog;
} gopher_conf_t;

extern struct lookup_config_t lookup_config;

#define UNUSED(a) { (void)(a); };
#define MAX(a,b) ((a) > (b)) ? (a) : (b)
#define MIN(a,b) ((a) < (b)) ? (a) : (b)

#endif /* _MAIN_H_ */
