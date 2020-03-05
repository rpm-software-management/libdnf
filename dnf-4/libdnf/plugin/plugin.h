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

#ifndef _LIBDNF_PLUGIN_H
#define _LIBDNF_PLUGIN_H

#include "../dnf-types.h"
#include "../hy-types.h"

typedef struct {
    const char * name;
    const char * version;
} PluginInfo;

typedef enum {
    PLUGIN_MODE_CONTEXT = 10000
} PluginMode;

typedef enum {
    PLUGIN_HOOK_ID_CONTEXT_PRE_CONF = 10000,
    PLUGIN_HOOK_ID_CONTEXT_CONF,
    PLUGIN_HOOK_ID_CONTEXT_PRE_TRANSACTION,
    PLUGIN_HOOK_ID_CONTEXT_TRANSACTION,
    PLUGIN_HOOK_ID_CONTEXT_PRE_REPOS_RELOAD
} PluginHookId;

#ifdef __cplusplus
namespace libdnf {
    struct PluginError;
    struct PluginInitData;
    struct PluginHookData;
}
typedef struct libdnf::PluginError DnfPluginError;
typedef struct libdnf::PluginInitData DnfPluginInitData;
typedef struct libdnf::PluginHookData DnfPluginHookData;
#else
typedef struct PluginError DnfPluginError;
typedef struct PluginInitData DnfPluginInitData;
typedef struct PluginHookData DnfPluginHookData;
#endif

typedef struct _PluginHandle PluginHandle;

#ifdef __cplusplus
extern "C" {
#endif

// Functions to access initData.
DnfContext * pluginGetContext(DnfPluginInitData * data);

// Functions to access hookData.
// Usable with PLUGIN_HOOK_ID_CONTEXT_PRE_TRANSACTION and PLUGIN_HOOK_ID_CONTEXT_TRANSACTION,
DnfTransaction * hookContextTransactionGetTransaction(DnfPluginHookData * data);
HyGoal hookContextTransactionGetGoal(DnfPluginHookData * data);
DnfState * hookContextTransactionGetState(DnfPluginHookData * data);

// code below will be implemented in plugins
struct _PluginHandle;

const PluginInfo * pluginGetInfo(void);
PluginHandle * pluginInitHandle(int version, PluginMode mode, DnfPluginInitData * initData);
void pluginFreeHandle(PluginHandle * handle);
int pluginHook(PluginHandle * handle, PluginHookId id, DnfPluginHookData * data, DnfPluginError * error);

#ifdef __cplusplus
}
#endif

#endif
