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

/*
 * This is example libdnf plugin. It works with applications that uses "context" libdnf API
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
    .name = "ExamplePlugin",
    .version = "1.0.0"
};

// Plugin instance private data
// Pointer to instance of this structure is returned by pluginInitHandle() as handle.
struct _PluginHandle
{
    PluginMode mode;
    DnfContext * context;  // store plugin context specific init data
    FILE * outStream; // stream to write output
};

// Returns general information about this plugin.
// Can be called at any time.
const PluginInfo * pluginGetInfo(void)
{
    return &info;
}

// Creates new instance of this plugin. Returns its handle.
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
	if (handle) {
            handle->mode = mode;
            handle->context = pluginGetContext(initData);
            handle->outStream = outStream;
	}
    } while (0);

    fprintf(outStream, "%s: %s: exit =========================\n", info.name, __func__);
    if (handle)
        fflush(outStream);
    else
        fclose(outStream);
    return handle;
}

// Destroys the plugin instance identified by given handle.
void pluginFreeHandle(PluginHandle * handle)
{
    if (handle) {
        fprintf(handle->outStream, "%s: %s: ===========================\n", info.name, __func__);
        fprintf(handle->outStream, "===============================================================\n\n");
        fclose(handle->outStream);
        free(handle);
    }
}

// Writes a list of "packages". For each package is written a list of packages
// thats are upgraded/downgraded/obsoleted by this package. These packages are added into
// the "obsoleted" array.
static void writeInfo(FILE * f, HyGoal goal, GPtrArray * packages, GPtrArray * obsoleted,
                      const char * header, const char * obsoleteText)
{
    if (packages->len == 0)
        return;
    fprintf(f, "%s", header);
    for (unsigned int i = 0; i < packages->len; ++i) {
        DnfPackage * pkg = g_ptr_array_index(packages, i);
        fprintf(f, "  %s@%s\n", dnf_package_get_nevra(pkg), dnf_package_get_reponame(pkg));

        // list of upgraded, downgraded, obsoleted packages
        g_autoptr(GPtrArray) obsoletedPackages = hy_goal_list_obsoleted_by_package(goal, pkg);
        for (unsigned int obsIdx = 0; obsIdx < obsoletedPackages->len; ++obsIdx) {
            DnfPackage * obsPkg = g_ptr_array_index(obsoletedPackages, obsIdx);
            fprintf(f, "    %-25s%s\n", obsoleteText, dnf_package_get_nevra(obsPkg));

            // Package can be obsoleted by more packages.
            // Do not add it into the "obsoleted" array more that once.
            gboolean found = FALSE;
            for (unsigned int allObsIdx = 0; allObsIdx < obsoleted->len; ++allObsIdx) {
                found = dnf_package_get_identical(g_ptr_array_index(obsoleted, allObsIdx), obsPkg);
                if (found)
                    break;
            }
            if (!found)
                g_ptr_array_add(obsoleted, g_object_ref(obsPkg));
        }
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

                // "obsoleted" array will be filled with obsoleted/updated/downgraded packages
                g_autoptr(GPtrArray) obsoleted = g_ptr_array_new_with_free_func((GDestroyNotify)g_object_unref);

                GPtrArray * packages = hy_goal_list_installs(goal, NULL);
                if (packages) {
                    writeInfo(handle->outStream, goal, packages, obsoleted, "Install:\n", "obsoleted");
                    g_ptr_array_unref(packages);
                }
                packages = hy_goal_list_reinstalls(goal, NULL);
                if (packages) {
                    writeInfo(handle->outStream, goal, packages, obsoleted, "Reinstall:\n", "obsoleted");
                    g_ptr_array_unref(packages);
                }
                packages = hy_goal_list_downgrades(goal, NULL);
                if (packages) {
                    writeInfo(handle->outStream, goal, packages, obsoleted, "Downgrade:\n", "downgraded/obsoleted");
                    g_ptr_array_unref(packages);
                }
                packages = hy_goal_list_upgrades(goal, NULL);
                if (packages) {
                    writeInfo(handle->outStream, goal, packages, obsoleted, "Upgrade:\n", "upgraded/obsoleted");
                    g_ptr_array_unref(packages);
                }
                packages = hy_goal_list_erasures(goal, NULL);
                if (packages) {
                    if (packages->len)
                        fprintf(handle->outStream, "Remove:\n");
                    for (unsigned int i = 0; i < packages->len; ++i) {
                        DnfPackage * pkg = g_ptr_array_index(packages, i);
                        fprintf(handle->outStream, "  %s\n", dnf_package_get_nevra(pkg));
                    }
                    g_ptr_array_unref(packages);
                }
                if (obsoleted->len) {
                    fprintf(handle->outStream, "Summary of upgraded, downgraded, obsoleted:\n");
                    for (unsigned int i = 0; i < obsoleted->len; ++i) {
                        DnfPackage * pkg = g_ptr_array_index(obsoleted, i);
                        fprintf(handle->outStream, "  %s\n", dnf_package_get_nevra(pkg));
                        g_autoptr(GPtrArray) obsoletedByPackages = hy_goal_list_obsoleted_by_package(goal, pkg);
                        for (unsigned int obsIdx = 0; obsIdx < obsoletedByPackages->len; ++obsIdx) {
                            DnfPackage * obsPkg = g_ptr_array_index(obsoletedByPackages, obsIdx);
                            fprintf(handle->outStream, "    %-25s%s@%s\n", "by",
                                    dnf_package_get_nevra(obsPkg), dnf_package_get_reponame(obsPkg));
                        }
                    }
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
