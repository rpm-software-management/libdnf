/*
 * Copyright (C) 2020 Red Hat, Inc.
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

#ifndef _DNF_CONF_H_
#define _DNF_CONF_H_

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* redefined: enum class libdnf::Option::Priority */
enum DnfConfPriority {
    DNF_CONF_EMPTY = 0,
    DNF_CONF_DEFAULT = 10,
    DNF_CONF_MAINCONFIG = 20,
    DNF_CONF_AUTOMATICCONFIG = 30,
    DNF_CONF_REPOCONFIG = 40,
    DNF_CONF_PLUGINDEFAULT = 50,
    DNF_CONF_PLUGINCONFIG = 60,
    DNF_CONF_DROPINCONFIG = 65,
    DNF_CONF_COMMANDLINE = 70,
    DNF_CONF_RUNTIME = 80
};

gchar *dnf_conf_main_get_option(const gchar *name, enum DnfConfPriority *priority, GError ** error);
gboolean dnf_conf_main_set_option(const gchar *name, enum DnfConfPriority priority, const gchar *value, GError ** error);
gboolean dnf_conf_add_setopt(const gchar *key, enum DnfConfPriority priority, const gchar *value, GError ** error);

#ifdef __cplusplus
}
#endif

#endif /* _DNF_CONF_H_ */
