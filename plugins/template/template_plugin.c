/*
 * Copyright (C) 2019 Red Hat, Inc.
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
 * This is template_plugin for libdnf. It works with applications that uses "context" libdnf API
 * (eg. microdnf and PackageKit).
 * The plugin writes information about repositories in the context and about packages in
 * the goal into the file.
 */

#define OUT_FILE_PATH "/tmp/libdnf_test_plugin.log"

#include <libdnf/plugin/plugin.h>
#include <libdnf/libdnf.h>

#include <stdio.h>
#include <stdlib.h>

// Information about this plugin
// Pointer to this structure is returned by pluginGetInfo().
static const PluginInfo info = {
    .name = "template_plugin",
    .version = "1.0.0"
};

// Plugin instance private data
// Pointer to instance of this structure is returned by pluginInitHandle() as handle.
struct _PluginHandle
{
    PluginMode mode;
    DnfContext *context;  // store plugin context specific init data
};

// Returns general information about this plugin.
// Can be called at any time.
const PluginInfo *pluginGetInfo(void)
{
    return &info;
}

// Creates new instance of this plugin. Returns its handle.
PluginHandle *pluginInitHandle(int version, PluginMode mode, DnfPluginInitData *initData)
{
    if (version != 1)
        return NULL;
    if (mode != PLUGIN_MODE_CONTEXT)
        return NULL;

    PluginHandle *handle = NULL;
    handle = malloc(sizeof(*handle));
    if (handle == NULL)
        return NULL;

    handle->mode = mode;
    handle->context = pluginGetContext(initData);

    return handle;
}

// Destroys the plugin instance identified by given handle.
void pluginFreeHandle(PluginHandle *handle)
{
    if (handle) {
        free(handle);
    }
}

int pluginHook(PluginHandle *handle, PluginHookId id, DnfPluginHookData *hookData, DnfPluginError *error)
{
    if (!handle)
        return 1;

    switch (id) {
        case PLUGIN_HOOK_ID_CONTEXT_PRE_CONF:
            printf("Triggering PRE_CONF hook.\n");
            break;
        case PLUGIN_HOOK_ID_CONTEXT_PRE_REPOS_RELOAD:
            printf("Triggering PRE_REPOS_RELOAD hook.\n");
            break;
        case PLUGIN_HOOK_ID_CONTEXT_CONF:
            printf("Triggering CONF hook.\n");
            break;
        case PLUGIN_HOOK_ID_CONTEXT_PRE_TRANSACTION:
            printf("Triggering PRE_TRANSACTION hook.\n");
            break;
        case PLUGIN_HOOK_ID_CONTEXT_TRANSACTION:
            printf("Triggering TRANSACTION hook.\n");
            break;
        default:
            break;
    }

    return 1;
}
