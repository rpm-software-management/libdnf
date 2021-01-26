/*
 * Copyright (C) 2021 Red Hat, Inc.
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */


#ifndef LIBDNF_PLUGIN_SNAPPER_UTILS_H
#define LIBDNF_PLUGIN_SNAPPER_UTILS_H

#include <dbus/dbus.h>
#include <inttypes.h>

typedef struct {
    char msg[250];
} ErrMsgBuf;

DBusConnection * snapper_dbus_conn(ErrMsgBuf * err_msg);
int snapper_dbus_create_pre_snap_call(DBusConnection * conn, const char * snapper_conf, const char * desc, uint32_t * out_snap_id, ErrMsgBuf * err_msg);
int snapper_dbus_create_post_snap_call(DBusConnection * conn, const char * snapper_conf, uint32_t pre_snap_id, const char * desc, ErrMsgBuf * err_msg);

#endif
