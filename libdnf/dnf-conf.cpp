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
 * License along with this library; if not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "dnf-conf.h"

#include "dnf-context.hpp"

/**
 * dnf_conf_main_get_option:
 * @opt_name: option name
 * @priority: pointer to priority - output value
 * @error: A #GError or %NULL
 *
 * Gets option value and priority.
 *
 * Returns: string value or %NULL, %NULL means error
 *
 * Since: 0.56.0
 **/gchar *
dnf_conf_main_get_option(const gchar * name, enum DnfConfPriority * priority, GError ** error)
{
    auto & optBinds = libdnf::getGlobalMainConfig().optBinds();
    auto optBindsIter = optBinds.find(name);

    if (optBindsIter == optBinds.end()) {
        g_set_error(error, DNF_ERROR, DNF_ERROR_UNKNOWN_OPTION, "Unknown option \"%s\"", name);
        return NULL;
    }

    try {
        gchar * ret = g_strdup(optBindsIter->second.getValueString().c_str());
        *priority = static_cast<DnfConfPriority>(optBindsIter->second.getPriority());
        return ret;
    } catch (const std::exception & ex) {
        g_set_error(error, DNF_ERROR, DNF_ERROR_FILE_INVALID, "Option \"%s\": %s", name, ex.what());
        return NULL;
    }
}

/**
 * dnf_conf_main_set_option:
 * @opt_name: option name
 * @priority: priority
 * @value: string value
 * @error: A #GError or %NULL
 *
 * Sets option value and priority.
 *
 * Returns: %TRUE  if option opt_name was set, %FALSE means error - error value is set
 *
 * Since: 0.56.0
 **/
gboolean
dnf_conf_main_set_option(const gchar * name, enum DnfConfPriority priority, const gchar * value, GError ** error)
{
    auto & optBinds = libdnf::getGlobalMainConfig().optBinds();
    auto optBindsIter = optBinds.find(name);

    if (optBindsIter == optBinds.end()) {
        g_set_error(error, DNF_ERROR, DNF_ERROR_UNKNOWN_OPTION, "Unknown option \"%s\"", name);
        return FALSE;
    }

    try {
        optBindsIter->second.newString(static_cast<libdnf::Option::Priority>(priority), value);
    } catch (const std::exception & ex) {
        g_set_error(error, DNF_ERROR, DNF_ERROR_FILE_INVALID, "Option \"%s\": %s", name, ex.what());
        return FALSE;
    }

    return TRUE;
}

/**
 * dnf_conf_add_setopt:
 * @key: opton_name or repo_id.option_name (repo_id can contain globs)
 * @priority: priority
 * @value: string value
 * @error: A #GError or %NULL
 *
 * Add setopt. Supports also repositories options.
 *
 * Returns: %TRUE  setopt was accepted, %FALSE means error - setopt was not accepted
 *
 * Since: 0.56.0
 **/
gboolean
dnf_conf_add_setopt(const gchar * key, enum DnfConfPriority priority, const gchar * value, GError ** error)
{
    return libdnf::addSetopt(key, static_cast<libdnf::Option::Priority>(priority), value, error);
}
