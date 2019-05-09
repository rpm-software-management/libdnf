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

#include <libdnf/plugin/plugin.h>
#include <libdnf/libdnf.h>
#include <libdnf/log.hpp>
#include <rhsm/rhsm.h>

#include <string>

#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// Information about this plugin
// Pointer to this structure is returned by pluginGetInfo().
static const PluginInfo info = {
    .name = "rhsm",
    .version = "1.0.0"
};

// plugin context private data
// Pointer to instance of this structure is returned by pluginInitHandle() as handle.
struct _PluginHandle
{
    PluginMode mode;
    DnfContext * context;  // store plugin context specific init data
};

const PluginInfo * pluginGetInfo(void)
{
    return &info;
}

PluginHandle * pluginInitHandle(int version, PluginMode mode, DnfPluginInitData * initData)
{
    auto logger(libdnf::Log::getLogger());
    if (version != 1) {
        auto msg = std::string(info.name) + ": " + __func__ + ": Error: Unsupported API version";
        logger->error(msg);
        return nullptr;
    }
    if (mode != PLUGIN_MODE_CONTEXT) {
        auto msg = std::string(info.name) + ": " + __func__ + ": Warning: Unsupported mode";
        logger->warning(msg);
        return nullptr;
    }
    auto handle = new PluginHandle;
    handle->mode = mode;
    handle->context = pluginGetContext(initData);
    return handle;
}

void pluginFreeHandle(PluginHandle * handle)
{
    if (handle)
        delete handle;
}

/**
 * dnf_context_setup_enrollments:
 * @context: a #DnfContext instance.
 *
 * Resyncs the enrollment with the vendor system. This may change the contents
 * of files in repos.d according to subscription levels.
 *
 * Returns: %true for success, %false otherwise
 **/
static bool setupEnrollments(PluginHandle *handle)
{
    auto logger(libdnf::Log::getLogger());

    if (dnf_context_get_cache_only(handle->context))
        return true;

    /* Let's assume that alternative installation roots don't
     * require entitlement.  We only want to do system things if
     * we're actually affecting the system.  A more accurate test
     * here would be checking that we're using /etc/yum.repos.d or
     * so, but that can come later.
     */
    auto installRoot = dnf_context_get_install_root(handle->context);
    if (installRoot && strcmp(installRoot, "/") != 0)
        return true;

    /* Also, all of the subman stuff only works as root, if we're not
     * root, assume we're running in the test suite, or we're part of
     * e.g. rpm-ostree which is trying to run totally as non-root.
     */
    if (getuid() != 0)
        return true;

    auto repoDir = dnf_context_get_repo_dir(handle->context);
    g_autofree gchar *repofname = g_build_filename(repoDir, "redhat.repo", NULL);

    g_autoptr(RHSMContext) rhsm_ctx = rhsm_context_new();
    g_autoptr(GKeyFile) repofile = rhsm_utils_yum_repo_from_context(rhsm_ctx);

    /* We don't want rewrite "redhat.repo" file with the same content.
     * So we do a comparison of contents. Only If file is missing or
     * contents differs new content is written to the file.
     */
    bool sameContent = false;
    int fd;
    if ((fd = open(repofname, O_RDONLY)) != -1) {
        gsize length;
        g_autofree gchar *data = g_key_file_to_data(repofile, &length, NULL);
        auto fileLen = lseek(fd, 0, SEEK_END);
        if (fileLen == static_cast<long>(length)) {
            if (fileLen > 0) {
                auto addr = mmap(NULL, fileLen, PROT_READ, MAP_PRIVATE, fd, 0);
                if (addr != reinterpret_cast<void *>(-1)) {
                    sameContent = memcmp(addr, data, fileLen) == 0;
                    munmap(addr, fileLen);
                }
            } else {
                sameContent = true;
            }
        }
        close(fd);
    }
    if (!sameContent) {
        if (g_mkdir_with_parents(repoDir, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1) {
            logger->error(std::string(info.name) + ": " + __func__ + ": Error: Can't create directory " + repoDir);
            return false;
        }
        GError * error = nullptr;
        if (!g_key_file_save_to_file(repofile, repofname, &error)) {
            logger->error(std::string(info.name) + ": " + __func__ + ": Error: Can't save repo file: " + error->message);
            g_error_free(error);
            return false;
        }
    }

    return true;
}

int pluginHook(PluginHandle * handle, PluginHookId id, DnfPluginHookData * hookData, DnfPluginError * error)
{
    if (!handle)
        return 1;

    switch (id) {
        case PLUGIN_HOOK_ID_CONTEXT_PRE_REPOS_RELOAD:
            return setupEnrollments(handle);
        default:
            break;
    }
    return 1;
}
