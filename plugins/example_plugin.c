/*
 * Copyright (C) 2018 Red Hat, Inc.
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

#define OUT_FILE_PATH "/tmp/libdnf_test_plugin.log"

#include <libdnf/plugin/plugin.h>
#include <libdnf/libdnf.h>

#include <stdio.h>
#include <stdlib.h>

// Information about this plugin
// Pointer to this structure is returned by pluginGetInfo().
static const PluginInfo info = {
    .name = "ExamplePlugin",
    .version = "1.0.0"
};

// plugin context private data
// Pointer to instance of this structure is returned by pluginInitHandle() as handle.
struct _PluginHandle
{
    PluginMode mode;
    DnfContext * context;  // store plugin context specific init data
    FILE * outStream; // stream to write output
};

const PluginInfo * pluginGetInfo(void)
{
    return &info;
}

PluginHandle * pluginInitHandle(int version, PluginMode mode, DnfPluginInitData * initData)
{
    PluginHandle * handle = NULL;
    FILE * outStream = fopen(OUT_FILE_PATH, "a");
    if (!outStream)
        return handle;

    do {
        fprintf(outStream, "===============================================================\n");
        fprintf(outStream, "%s: %s: enter =========================\n", info.name, __func__);
        fprintf(outStream, "Plugin version=\"%s\", received API version=%i, received mode=\"%i\"\n",
                info.version, version, mode);
        if (version != 1) {
            fprintf(outStream, "%s: %s: Error: Unsupported API version\n", info.name, __func__);
            break;
        }
        if (mode != PLUGIN_MODE_CONTEXT) {
            fprintf(outStream, "%s: %s: Warning: Unsupported mode\n", info.name, __func__);
            break;
        }
        handle = malloc(sizeof(*handle));
        handle->mode = mode;
        handle->context = pluginGetContext(initData);
        handle->outStream = outStream;
    } while (0);

    fprintf(outStream, "%s: %s: exit =========================\n", info.name, __func__);
    if (handle)
        fflush(outStream);
    else
        fclose(outStream);
    return handle;
}

void pluginFreeHandle(PluginHandle * handle)
{
    if (handle) {
        fprintf(handle->outStream, "%s: %s: ===========================\n", info.name, __func__);
        fprintf(handle->outStream, "===============================================================\n\n");
        fclose(handle->outStream);
        free(handle);
    }
}

int pluginHook(PluginHandle * handle, PluginHookId id, DnfPluginHookData * hookData, DnfPluginError * error)
{
    if (!handle)
        return 1;
    fprintf(handle->outStream, "%s: %s: id=%i enter  ========================\n", info.name, __func__, id);
    switch (id) {
        case PLUGIN_HOOK_ID_CONTEXT_PRE_TRANSACTION:
            fprintf(handle->outStream, "Info before libdnf context transaction run:\n");

            // write info about loaded repos
            fprintf(handle->outStream, "Info about loaded repos:\n");
            GPtrArray * repos = dnf_context_get_repos(handle->context);
            for (unsigned int i = 0; i < repos->len; ++i) {
                DnfRepo * repo = g_ptr_array_index(repos, i);
                const gchar * repoId = dnf_repo_get_id(repo);
                g_autofree gchar * description = dnf_repo_get_description(repo);
                bool enabled = (dnf_repo_get_enabled(repo) & DNF_REPO_ENABLED_PACKAGES) > 0;
                fprintf(handle->outStream, "Repo enabled=%i,  repoId=%s, repoDescr=\"%s\"\n", enabled, repoId, description);
            }

            // write info about packages in goal
            fprintf(handle->outStream, "Info about packages in goal:\n");
            HyGoal goal = hookContextTransactionGetGoal(hookData);
            if (goal) {
                GPtrArray * packages = hy_goal_list_installs(goal, NULL);
                if (packages) {
                    for (unsigned int i = 0; i < packages->len; ++i) {
                        DnfPackage * pkg = g_ptr_array_index(packages, i);
                        fprintf(handle->outStream, "Action=%-18s%s\n", "install", dnf_package_get_nevra(pkg));
                    }
                    g_ptr_array_unref(packages);
                }
                packages = hy_goal_list_reinstalls(goal, NULL);
                if (packages) {
                    for (unsigned int i = 0; i < packages->len; ++i) {
                        DnfPackage * pkg = g_ptr_array_index(packages, i);
                        fprintf(handle->outStream, "Action=%-18s%s\n", "reinstall", dnf_package_get_nevra(pkg));
                    }
                    g_ptr_array_unref(packages);
                }
                packages = hy_goal_list_downgrades(goal, NULL);
                if (packages) {
                    for (unsigned int i = 0; i < packages->len; ++i) {
                        DnfPackage * pkg = g_ptr_array_index(packages, i);
                        fprintf(handle->outStream, "Action=%-18s%s\n", "downgrade", dnf_package_get_nevra(pkg));
                    }
                    g_ptr_array_unref(packages);
                }
                packages = hy_goal_list_upgrades(goal, NULL);
                if (packages) {
                    for (unsigned int i = 0; i < packages->len; ++i) {
                        DnfPackage * pkg = g_ptr_array_index(packages, i);
                        fprintf(handle->outStream, "Action=%-18s%s\n", "upgrade", dnf_package_get_nevra(pkg));
                    }
                    g_ptr_array_unref(packages);
                }
                packages = hy_goal_list_erasures(goal, NULL);
                if (packages) {
                    for (unsigned int i = 0; i < packages->len; ++i) {
                        DnfPackage * pkg = g_ptr_array_index(packages, i);
                        fprintf(handle->outStream, "Action=%-18s%s\n", "remove", dnf_package_get_nevra(pkg));
                    }
                    g_ptr_array_unref(packages);
                }
                packages = hy_goal_list_obsoleted(goal, NULL);
                if (packages) {
                    for (unsigned int i = 0; i < packages->len; ++i) {
                        DnfPackage * pkg = g_ptr_array_index(packages, i);
                        fprintf(handle->outStream, "Action=%-18s%s\n", "obsolete", dnf_package_get_nevra(pkg));
                    }
                    g_ptr_array_unref(packages);
                }
            }
            break;
        default:
            break;
    }
    fprintf(handle->outStream, "%s: %s: id=%i exit  ========================\n", info.name, __func__, id);
    fflush(handle->outStream);
    return 1;
}
