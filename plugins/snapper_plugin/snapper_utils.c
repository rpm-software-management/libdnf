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

#include "snapper_utils.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

static const char * const SNAPPER_CLEANUP_ALGORITHM = "number";
static const char * const SNAPPER_DBUS_SIG_CREATE_SNAP_RSP  = "u";
static const char * const SNAPPER_DBUS_SIG_STRING_DICT = "{ss}";

DBusConnection * snapper_dbus_conn(ErrMsgBuf * err_msg)
{
    DBusError err;
    dbus_error_init(&err);

    DBusConnection * conn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
    if (dbus_error_is_set(&err)) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "connection error: %s", err.message);
        dbus_error_free(&err);
    }

    return conn;
}

static int snapper_dbus_msg_send(DBusConnection * conn, DBusMessage * msg, DBusPendingCall ** pending_out, ErrMsgBuf * err_msg)
{
    DBusPendingCall * pending;

    // send message and get a handle for a reply
    if (!dbus_connection_send_with_reply(conn, msg, &pending, -1)) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "Out Of Memory!");
        return -ENOMEM;
    }
    if (!pending) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "error: Pending Call Null");
        return -EINVAL;
    }

    dbus_connection_flush(conn);
    *pending_out = pending;

    return 0;
}

static int snapper_dbus_msg_recv(DBusPendingCall * pending, DBusMessage ** msg_out, ErrMsgBuf * err_msg)
{
    DBusMessage * msg;

    // block until we receive a reply
    dbus_pending_call_block(pending);

    // get the reply message
    msg = dbus_pending_call_steal_reply(pending);
    if (!msg) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "error: Reply Null");
        return -ENOMEM;
    }

    // free the pending message
    dbus_pending_call_unref(pending);

    *msg_out = msg;

    return 0;
}

static int snapper_dbus_type_check_get(DBusMessageIter * iter, int expected_type, void * val, ErrMsgBuf * err_msg)
{
    int type = dbus_message_iter_get_arg_type(iter);
    if (type != expected_type) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "got type %d, expecting %d", type, expected_type);
        return -EINVAL;
    }

    dbus_message_iter_get_basic(iter, val);

    return 0;
}

static int snapper_dbus_create_pre_snap_pack(const char * snapper_conf, const char * desc, DBusMessage ** req_msg_out, ErrMsgBuf * err_msg)
{
    DBusMessage * msg;
    DBusMessageIter args;
    DBusMessageIter array_iter;
    bool ret;

    msg = dbus_message_new_method_call("org.opensuse.Snapper",  // target for the method call
                                       "/org/opensuse/Snapper", // object to call on
                                       "org.opensuse.Snapper",  // interface to call on
                                       "CreatePreSnapshot");    // method name
    if (!msg) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "failed to create req msg");
        return -ENOMEM;
    }

    // append arguments
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &snapper_conf)) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "Out Of Memory!");
        return -ENOMEM;
    }

    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &desc)) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "Out Of Memory!");
        return -ENOMEM;
    }

    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &SNAPPER_CLEANUP_ALGORITHM)) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "Out Of Memory!");
        return -ENOMEM;
    }

    ret = dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, SNAPPER_DBUS_SIG_STRING_DICT, &array_iter);
    if (!ret) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "failed to open array container");
        return -ENOMEM;
    }

    dbus_message_iter_close_container(&args, &array_iter);

    *req_msg_out = msg;

    return 0;
}

static int snapper_dbus_create_snap_unpack(DBusMessage * rsp_msg, uint32_t * snap_id_out, ErrMsgBuf * err_msg)
{
    DBusMessageIter iter;
    int msg_type;
    const char * sig;

    msg_type = dbus_message_get_type(rsp_msg);
    if (msg_type == DBUS_MESSAGE_TYPE_ERROR) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "create snap error response: %s",
            dbus_message_get_error_name(rsp_msg));
        return -EINVAL;
    }

    if (msg_type != DBUS_MESSAGE_TYPE_METHOD_RETURN) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "unexpected create snap ret type: %d", msg_type);
        return -EINVAL;
    }

    sig = dbus_message_get_signature(rsp_msg);
    if (!sig || (strcmp(sig, SNAPPER_DBUS_SIG_CREATE_SNAP_RSP) != 0)) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "bad create snap response sig: %s, expected: %s",
            (sig ? sig : "NULL"), SNAPPER_DBUS_SIG_CREATE_SNAP_RSP);
        return -EINVAL;
    }

    // read arguments
    if (!dbus_message_iter_init(rsp_msg, &iter)) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "snap response: Message has no arguments!");
        return -EINVAL;
    }

    if (snapper_dbus_type_check_get(&iter, DBUS_TYPE_UINT32, snap_id_out, err_msg)) {
        return -EINVAL;
    }

    return 0;
}

int snapper_dbus_create_pre_snap_call(DBusConnection * conn, const char * snapper_conf, const char * desc, uint32_t * out_snap_id, ErrMsgBuf * err_msg)
{
    int ret;
    DBusMessage * req_msg;
    DBusMessage * rsp_msg;
    DBusPendingCall *pending;

    ret = snapper_dbus_create_pre_snap_pack(snapper_conf, desc, &req_msg, err_msg);
    if (ret < 0) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "failed to pack create snap request");
        return ret;
    }

    ret = snapper_dbus_msg_send(conn, req_msg, &pending, err_msg);
    if (ret < 0) {
        dbus_message_unref(req_msg);
        return ret;
    }

    ret = snapper_dbus_msg_recv(pending, &rsp_msg, err_msg);
    if (ret < 0) {
        dbus_message_unref(req_msg);
        dbus_pending_call_unref(pending);
        return ret;
    }

    ret = snapper_dbus_create_snap_unpack(rsp_msg, out_snap_id, err_msg);
    if (ret < 0) {
        dbus_message_unref(req_msg);
        dbus_message_unref(rsp_msg);
        return ret;
    }

    dbus_message_unref(req_msg);
    dbus_message_unref(rsp_msg);

    return 0;
}


static int snapper_dbus_create_post_snap_pack(const char * snapper_conf, uint32_t pre_snap_id, const char * desc, DBusMessage ** req_msg_out, ErrMsgBuf * err_msg)
{
    DBusMessage * msg;
    DBusMessageIter args;
    DBusMessageIter array_iter;
    bool ret;

    msg = dbus_message_new_method_call("org.opensuse.Snapper",  // target for the method call
                                       "/org/opensuse/Snapper", // object to call on
                                       "org.opensuse.Snapper",  // interface to call on
                                       "CreatePostSnapshot");   // method name
    if (!msg) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "failed to create req msg");
        return -ENOMEM;
    }

    // append arguments
    dbus_message_iter_init_append(msg, &args);
    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &snapper_conf)) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "Out Of Memory!");
        return -ENOMEM;
    }

    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &pre_snap_id)) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "Out Of Memory!");
        return -ENOMEM;
    }

    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &desc)) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "Out Of Memory!");
        return -ENOMEM;
    }

    if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &SNAPPER_CLEANUP_ALGORITHM)) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "Out Of Memory!");
        return -ENOMEM;
    }

    ret = dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, SNAPPER_DBUS_SIG_STRING_DICT, &array_iter);
    if (!ret) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "failed to open array container");
        return -ENOMEM;
    }

    dbus_message_iter_close_container(&args, &array_iter);

    *req_msg_out = msg;

    return 0;
}

int snapper_dbus_create_post_snap_call(DBusConnection * conn, const char * snapper_conf, uint32_t pre_snap_id, const char * desc, ErrMsgBuf * err_msg)
{
    int ret;
    DBusMessage * req_msg;
    DBusMessage * rsp_msg;
    DBusPendingCall * pending;
    uint32_t snap_id;

    ret = snapper_dbus_create_post_snap_pack(snapper_conf, pre_snap_id, desc, &req_msg, err_msg);
    if (ret < 0) {
        snprintf(err_msg->msg, sizeof(err_msg->msg), "failed to pack create snap request");
        return ret;
    }

    ret = snapper_dbus_msg_send(conn, req_msg, &pending, err_msg);
    if (ret < 0) {
        dbus_message_unref(req_msg);
        return ret;
    }

    ret = snapper_dbus_msg_recv(pending, &rsp_msg, err_msg);
    if (ret < 0) {
        dbus_message_unref(req_msg);
        dbus_pending_call_unref(pending);
        return ret;
    }

    ret = snapper_dbus_create_snap_unpack(rsp_msg, &snap_id, err_msg);
    if (ret < 0) {
        dbus_message_unref(req_msg);
        dbus_message_unref(rsp_msg);
        return ret;
    }

    dbus_message_unref(req_msg);
    dbus_message_unref(rsp_msg);

    return 0;
}

