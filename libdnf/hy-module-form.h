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

#ifndef LIBDNF_HY_MODULE_FORM_H
#define LIBDNF_HY_MODULE_FORM_H

#include "hy-types.h"
#include "dnf-sack.h"

#include <glib.h>

G_BEGIN_DECLS

enum _hy_module_form_param_e {
    HY_MODULE_FORM_NAME = 0,
    HY_MODULE_FORM_STREAM = 1,
    HY_MODULE_FORM_VERSION = 2,
    HY_MODULE_FORM_CONTEXT = 3,
    HY_MODULE_FORM_ARCH = 4,
    HY_MODULE_FORM_PROFILE = 5
};

HyModuleForm hy_module_form_create(void);
void hy_module_form_free(HyModuleForm module_form);
HyModuleForm hy_module_form_clone(HyModuleForm module_form);
const char *hy_module_form_get_string(HyModuleForm module_form, int which);
long long hy_module_form_get_version(HyModuleForm module_form);
void hy_module_form_set_version(HyModuleForm module_form, long long version);
void hy_module_form_set_string(HyModuleForm module_form, int which, const char* str_val);

G_END_DECLS

#endif //LIBDNF_HY_MODULE_FORM_H
