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

#include "plugin.h"
#include "debug.h"

#define MODULE_NAME "dir"

static void handler(client_t *client, char *resource);

int init(void) {
    INFO("Registering module %s", MODULE_NAME);
    return register_module(MODULE_NAME, handler);
}

int deinit(void) {
    INFO("Deregistering module %s", MODULE_NAME);
    return TRUE;
}

static void handler(client_t *client, char *resource) {
    DEBUG("Handling resource %s with module %s", resource, MODULE_NAME);
    handle_error(client, TYPE_DIR, "Not implemented");
}
