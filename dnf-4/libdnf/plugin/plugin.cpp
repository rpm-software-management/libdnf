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

#include "plugin-private.hpp"

#include "../log.hpp"
#include "bgettext/bgettext-lib.h"
#include "tinyformat/tinyformat.hpp"
#include <utils.hpp>

#include <stdexcept>

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#define PLUGIN_API_VERSION 1

namespace libdnf {

Library::Library(const char * path)
: path(path)
{
    handle = dlopen(path, RTLD_LAZY);
    if (!handle) {
        const char * errMsg = dlerror();
        throw std::runtime_error(tfm::format(_("Can't load shared library \"%s\": %s"), path, errMsg));
    }
}

Library::~Library()
{
    dlclose(handle);
}

Plugin::Plugin(const char * path) : Library(path)
{
    getInfo = reinterpret_cast<decltype(&pluginGetInfo)>(dlsym(handle, "pluginGetInfo"));
    if (!getInfo)
    {
        const char * errMsg = dlerror();
        throw std::runtime_error(tfm::format(_("Can't obtain address of symbol \"%s\": %s"), "pluginGetInfo", errMsg));
    }
    initHandle = reinterpret_cast<decltype(&pluginInitHandle)>(dlsym(handle, "pluginInitHandle"));
    if (!initHandle)
    {
        const char * errMsg = dlerror();
        throw std::runtime_error(tfm::format(_("Can't obtain address of symbol \"%s\": %s"), "pluginInitHandle", errMsg));
    }
    freeHandle = reinterpret_cast<decltype(&pluginFreeHandle)>(dlsym(handle, "pluginFreeHandle"));
    if (!freeHandle)
    {
        const char * errMsg = dlerror();
        throw std::runtime_error(tfm::format(_("Can't obtain address of symbol \"%s\": %s"), "pluginFreeHandle", errMsg));
    }
    hook = reinterpret_cast<decltype(&pluginHook)>(dlsym(handle, "pluginHook"));
    if (!hook)
    {
        const char * errMsg = dlerror();
        throw std::runtime_error(tfm::format(_("Can't obtain address of symbol \"%s\": %s"), "pluginHook", errMsg));
    }
}

void Plugins::loadPlugin(const std::string & path)
{
    auto logger(Log::getLogger());
    logger->debug(tfm::format(_("Loading plugin file=\"%s\""), path));
    pluginsWithData.emplace_back(PluginWithData{std::unique_ptr<Plugin>(new Plugin(path.c_str())), true, nullptr});
    auto info = pluginsWithData.back().plugin->getInfo();
    logger->debug(tfm::format(_("Loaded plugin name=\"%s\", version=\"%s\""), info->name, info->version));
}

void Plugins::loadPlugins(std::string dirPath)
{
    auto logger(Log::getLogger());
    if (dirPath.empty())
        throw std::runtime_error(_("Plugins::loadPlugins() dirPath cannot be empty"));
    if (dirPath.back() != '/')
        dirPath.push_back('/');
    struct dirent **namelist;
    auto count = scandir(dirPath.c_str(), &namelist,
                         [](const struct dirent *dent)->int{return string::endsWith(dent->d_name, ".so");},
                         alphasort);
    if (count == -1) {
        int errnum = errno;
        logger->warning(tfm::format(_("Can't read plugin directory \"%s\": %s"), dirPath, strerror(errnum)));
        return;
    }

    std::string errorMsgs;
    for (int idx = 0; idx < count; ++idx) {
        try {
            loadPlugin((dirPath + namelist[idx]->d_name).c_str());
        } catch (const std::exception & ex) {
            std::string msg = tfm::format(_("Can't load plugin \"%s\": %s"), namelist[idx]->d_name, ex.what());
            logger->error(msg);
            errorMsgs += msg + '\n';
        }
        ::free(namelist[idx]);
    }
    ::free(namelist);
    if (!errorMsgs.empty())
        throw std::runtime_error(errorMsgs);
}

bool Plugins::init(PluginMode mode, PluginInitData * initData)
{
    for (auto & pluginWithData : pluginsWithData) {
        if (!pluginWithData.enabled) {
            continue;
        }
        pluginWithData.handle = pluginWithData.plugin->initHandle(PLUGIN_API_VERSION, mode, initData);
        if (!pluginWithData.handle) {
            return false;
        }
    }
    return true;
}

void Plugins::free()
{
    for (auto it = pluginsWithData.rbegin(); it != pluginsWithData.rend(); ++it) {
        if (it->handle) {
            it->plugin->freeHandle(it->handle);
        }
    }
}

bool Plugins::hook(PluginHookId id, PluginHookData * hookData, DnfPluginError * error)
{
    for (auto & pluginWithData : pluginsWithData) {
        if (!pluginWithData.enabled || !pluginWithData.handle) {
            continue;
        }
        if (!pluginWithData.plugin->hook(pluginWithData.handle, id, hookData, error)) {
            return false;
        }
    }
    return true;
}

}
