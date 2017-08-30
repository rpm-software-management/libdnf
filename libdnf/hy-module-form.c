/*
 * Copyright (C) 2017 Red Hat, Inc.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <solv/util.h>
#include "hy-query.h"
#include "hy-module-form.h"
#include "hy-module-form-private.h"
#include "dnf-sack.h"
#include "hy-types.h"


static void
hy_module_form_clear(HyModuleForm module_form)
{
    module_form->name = NULL;
    module_form->stream = NULL;
    module_form->version = -1L;
    module_form->context = NULL;
    module_form->arch = NULL;
    module_form->profile = NULL;
}

HyModuleForm
hy_module_form_create()
{
    HyModuleForm module_form = g_malloc0(sizeof(*module_form));
    hy_module_form_clear(module_form);
    return module_form;
}

void
hy_module_form_free(HyModuleForm module_form)
{
    g_free(module_form->name);
    g_free(module_form->stream);
    g_free(module_form->context);
    g_free(module_form->arch);
    g_free(module_form->profile);
    g_free(module_form);
}

HyModuleForm
hy_module_form_clone(HyModuleForm module_form)
{
    HyModuleForm clone = hy_module_form_create();
    clone->name = g_strdup(module_form->name);
    clone->stream = g_strdup(module_form->stream);
    clone->version = module_form->version;
    clone->context = g_strdup(module_form->context);
    clone->arch = g_strdup(module_form->arch);
    clone->profile = g_strdup(module_form->profile);
    return clone;
}

static inline char **
get_string(HyModuleForm module_form, int which)
{
    switch (which) {
        case HY_MODULE_FORM_NAME:
            return &(module_form->name);
        case HY_MODULE_FORM_STREAM:
            return &(module_form->stream);
        case HY_MODULE_FORM_CONTEXT:
            return &(module_form->context);
        case HY_MODULE_FORM_ARCH:
            return &(module_form->arch);
        case HY_MODULE_FORM_PROFILE:
            return &(module_form->profile);
        default:
            return NULL;
    }
}

const char *
hy_module_form_get_string(HyModuleForm module_form, int which)
{
    return *get_string(module_form, which);
}

void
hy_module_form_set_string(HyModuleForm module_form, int which, const char* str_val)
{
    char** attr = get_string(module_form, which);
    g_free(*attr);
    *attr = g_strdup(str_val);
}

long long
hy_module_form_get_version(HyModuleForm module_form)
{
    return module_form->version;
}

void
hy_module_form_set_version(HyModuleForm module_form, long long version)
{
    module_form->version = version;
}
