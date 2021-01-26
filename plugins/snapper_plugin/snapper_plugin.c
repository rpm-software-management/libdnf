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

/*
 * This is Snapper libdnf plugin. It works with applications that uses "context" libdnf API
 * (eg. microdnf and PackageKit).
 * The plugin makes filesystem snapshot before and after transaction.
 */

#include "snapper_utils.h"

#include <libdnf/plugin/plugin.h>
#include <libdnf/libdnf.h>

#include <stdbool.h>

// Information about this plugin
static const PluginInfo info = {
    .name = "snapper",
    .version = "0.1.0"
};

// Plugin instance private data
struct _PluginHandle
{
    PluginMode mode;
    DnfContext * context;  // store plugin context specific init data
    bool pre_snap_created;
    uint32_t pre_snap_id;
};

// Returns general information about this plugin.
const PluginInfo * pluginGetInfo(void)
{
    return &info;
}

// Creates new instance of this plugin. Returns its handle.
PluginHandle * pluginInitHandle(int version, PluginMode mode, DnfPluginInitData * initData)
{
    PluginHandle * handle = NULL;
    do {
        g_debug("Plugin name=\"%s\" version=\"%s\": received API version=%i, received mode=\"%i\"", info.name, info.version, version, mode);
        if (version != 1) {
            g_debug("Plugin %s: %s: Error: Unsupported API version", info.name, __func__);
            break;
        }
        if (mode != PLUGIN_MODE_CONTEXT) {
            g_debug("Plugin %s: %s: Warning: Unsupported mode", info.name, __func__);
            break;
        }
        handle = malloc(sizeof(*handle));
        handle->mode = mode;
        handle->context = pluginGetContext(initData);
        handle->pre_snap_created = false;
    } while (0);

    return handle;
}

// Destroys the plugin instance identified by given handle.
void pluginFreeHandle(PluginHandle * handle)
{
    g_debug("Plugin %s: %s", info.name, __func__);

    if (handle) {
        free(handle);
    }
}

// Creates pre snapshot
static bool createPreSnapshot(PluginHandle * handle)
{
    g_debug("Plugin %s: %s", info.name, __func__);

    ErrMsgBuf err_msg = {.msg = ""};

    DBusConnection * dbusConn = snapper_dbus_conn(&err_msg);
    if (!dbusConn) {
        g_debug("Plugin %s: %s: connect failed: %s", info.name, __func__, err_msg.msg);
        return false;
    }

    int ret = snapper_dbus_create_pre_snap_call(dbusConn, "root", "libdnf-snapper-plugin", &handle->pre_snap_id, &err_msg);
    if (ret < 0) {
        g_warning("Plugin %s: %s: snapper_dbus_create_pre_snap_call failed: %s", info.name, __func__, err_msg.msg);
        dbus_connection_unref(dbusConn);
        return false;
    }

    dbus_connection_unref(dbusConn);

    return true;
}

// Creates post snapshot
static bool createPostSnapshot(PluginHandle * handle)
{
    g_debug("Plugin %s: %s", info.name, __func__);

    ErrMsgBuf err_msg = {.msg = ""};

    DBusConnection * dbusConn = snapper_dbus_conn(&err_msg);
    if (!dbusConn) {
        g_debug("Plugin %s: %s: connect failed: %s", info.name, __func__, err_msg.msg);
        return false;
    }

    int ret = snapper_dbus_create_post_snap_call(dbusConn, "root", handle->pre_snap_id, "libdnf-snapper-plugin", &err_msg);
    if (ret < 0) {
        g_warning("Plugin %s: %s: snapper_dbus_create_post_snap_call failed: %s", info.name, __func__, err_msg.msg);
        dbus_connection_unref(dbusConn);
        return false;
    }

    dbus_connection_unref(dbusConn);

    return true;
}

int pluginHook(PluginHandle * handle, PluginHookId id, DnfPluginHookData * hookData, DnfPluginError * error)
{
    g_debug("Plugin %s: %s: id=%i", info.name, __func__, id);

    if (!handle)
        return 1;
    switch (id) {
        case PLUGIN_HOOK_ID_CONTEXT_PRE_TRANSACTION:
            handle->pre_snap_created = createPreSnapshot(handle);
            break;
        case PLUGIN_HOOK_ID_CONTEXT_TRANSACTION:
            if (!handle->pre_snap_created) {
                g_warning("Plugin %s: %s: skipping post_snapshot because creation of pre_snapshot failed", info.name, __func__);
                break;
            }
            createPostSnapshot(handle);
            break;
        default:
            break;
    }
    return 1;
}
