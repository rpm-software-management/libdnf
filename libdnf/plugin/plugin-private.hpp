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

#ifndef _LIBDNF_PLUGIN_PRIVATE_HPP
#define _LIBDNF_PLUGIN_PRIVATE_HPP

#include "plugin.h"

#include <memory>
#include <string>
#include <vector>

namespace libdnf {

class Library {
public:
    Library(const char * path);
    ~Library();
    const std::string & getPath() const { return path; }
protected:
    std::string path;
    void * handle;
};

struct PluginError {
    int code;
    char * path;
    char * message;
};

struct PluginInitData {
    PluginInitData(PluginMode mode) : mode(mode) {}
    PluginMode mode;
};

struct PluginHookData {
    PluginHookData(PluginHookId hookId) : hookId(hookId) {}
    PluginHookId hookId;
};

class Plugin : public Library {
public:
    Plugin(const char * path);
    Plugin(const Plugin &) = delete;
    Plugin & operator=(const Plugin &) = delete;
    decltype(&pluginGetInfo) getInfo;
    decltype(&pluginInitHandle) initHandle;
    decltype(&pluginFreeHandle) freeHandle;
    decltype(&pluginHook) hook;
};

class Plugins {
public:
    void loadPlugin(const std::string & filePath);
    void loadPlugins(std::string dirPath);
    bool init(PluginMode mode, PluginInitData * initData);
    void free();
    bool hook(PluginHookId id, PluginHookData * hookData, DnfPluginError * error);
    size_t count() const { return pluginsWithHandles.size(); }

private:
    struct PluginWithHandle {
        std::unique_ptr<Plugin> plugin;
        PluginHandle * handle;
    };
    std::vector<PluginWithHandle> pluginsWithHandles;
};

}

#endif
