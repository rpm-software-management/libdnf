/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2013-2015 Richard Hughes <richard@hughsie.com>
 *
 * Most of this code was taken from Zif, libzif/zif-transaction.c
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or(at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

/**
 * SECTION:dnf-goal
 * @short_description: Helper methods for dealing with hawkey goals.
 * @include: libdnf.h
 * @stability: Unstable
 *
 * These methods make it easier to deal with hawkey goals.
 */


#include <glib.h>

#include "catch-error.hpp"
#include "hy-util.h"
#include "hy-goal-private.hpp"
#include "dnf-goal.h"
#include "dnf-package.h"
#include "hy-packageset-private.hpp"
#include "hy-iutil-private.hpp"
#include "dnf-context.hpp"
#include "dnf-sack-private.hpp"
#include "dnf-utils.h"
#include "utils/bgettext/bgettext-lib.h"
#include "../goal/Goal.hpp"

#include <vector>

/**
 * dnf_goal_depsolve:
 * @goal: a #HyGoal.
 * @flags: some #DnfGoalActions to enable.
 * @error: a #GError or %NULL
 *
 * Returns: %TRUE if depsolve is successful.
 *
 * Since: 0.7.0
 */
gboolean
dnf_goal_depsolve(HyGoal goal, DnfGoalActions flags, GError **error) try
{
    gint cnt;
    gint j;
    gint rc;
    g_autoptr(GString) string = NULL;

    DnfSack * sack = hy_goal_get_sack(goal);

    libdnf::Query query(sack);
    const auto & protected_packages = libdnf::getGlobalMainConfig().protected_packages().getValue();
    std::vector<const char *> cprotected_packages;
    cprotected_packages.reserve(protected_packages.size() + 1);
    for (const auto & package : protected_packages) {
        cprotected_packages.push_back(package.c_str());
    }
    cprotected_packages.push_back(nullptr);
    query.addFilter(HY_PKG_NAME, HY_EQ, cprotected_packages.data());
    auto pkgset = *query.runSet();
    goal->addProtected(pkgset);

    rc = hy_goal_run_flags(goal, flags);
    if (rc) {
        string = g_string_new(_("Could not depsolve transaction; "));
        cnt = hy_goal_count_problems(goal);
        g_string_append_printf(string, P_("%i problem detected:\n", "%i problems detected:\n", cnt),
                               cnt);
        for (j = 0; j < cnt; j++) {
            auto tmp = goal->describeProblemRules(j, true);
            bool first = true;
            for (auto & iter: tmp) {
                if (first) {
                    if (cnt != 1) {
                        g_string_append_printf(string, _(" Problem %1$i: %2$s\n"), j + 1, iter.c_str());
                    } else {
                        g_string_append_printf(string, _(" Problem: %s\n"), iter.c_str());
                    }
                    first = false;
                } else {
                    g_string_append_printf(string, "  - %s\n",  iter.c_str());
                }
            }
        }
        g_string_truncate(string, string->len - 1);
        g_set_error_literal(error,
                            DNF_ERROR,
                            DNF_ERROR_PACKAGE_CONFLICTS,
                            string->str);
        return FALSE;
    }

    /* anything to do? */
    if (hy_goal_req_length(goal) == 0) {
        g_set_error_literal(error,
                            DNF_ERROR,
                            DNF_ERROR_NO_PACKAGES_TO_UPDATE,
                            "The transaction was empty");
        return FALSE;
    }
    auto moduleContainer = dnf_sack_get_module_container(sack);
    if (moduleContainer) {
        auto installSet = goal->listInstalls();
        auto modulesToEnable = requiresModuleEnablement(sack, &installSet);
        for (auto module: modulesToEnable) {
            moduleContainer->enable(module->getName(), module->getStream());
        }
    }
    return TRUE;
} CATCH_TO_GERROR(FALSE)

/**
 * dnf_goal_get_packages:
 */
GPtrArray *
dnf_goal_get_packages(HyGoal goal, ...)
{
    GPtrArray *array;
    DnfPackage *pkg;
    gint info_tmp;
    guint i;
    guint j;
    va_list args;

    /* process the valist */
    va_start(args, goal);
    array = g_ptr_array_new_with_free_func((GDestroyNotify) g_object_unref);
    for (j = 0;; j++) {
        GPtrArray *pkglist = NULL;
        info_tmp = va_arg(args, gint);
        if (info_tmp == -1)
            break;
        switch(info_tmp) {
        case DNF_PACKAGE_INFO_REMOVE:
            pkglist = hy_goal_list_erasures(goal, NULL);
            for (i = 0; i < pkglist->len; i++) {
                pkg = (DnfPackage*)g_ptr_array_index (pkglist, i);
                dnf_package_set_action(pkg, DNF_STATE_ACTION_REMOVE);
                g_ptr_array_add(array, g_object_ref(pkg));
            }
            break;
        case DNF_PACKAGE_INFO_INSTALL:
            pkglist = hy_goal_list_installs(goal, NULL);
            for (i = 0; i < pkglist->len; i++) {
                pkg = (DnfPackage*)g_ptr_array_index (pkglist, i);
                dnf_package_set_action(pkg, DNF_STATE_ACTION_INSTALL);
                g_ptr_array_add(array, g_object_ref(pkg));
            }
            break;
        case DNF_PACKAGE_INFO_OBSOLETE:
            pkglist = hy_goal_list_obsoleted(goal, NULL);
            for (i = 0; i < pkglist->len; i++) {
                pkg = (DnfPackage*)g_ptr_array_index (pkglist, i);
                dnf_package_set_action(pkg, DNF_STATE_ACTION_OBSOLETE);
                g_ptr_array_add(array, g_object_ref(pkg));
            }
            break;
        case DNF_PACKAGE_INFO_REINSTALL:
            pkglist = hy_goal_list_reinstalls(goal, NULL);
            for (i = 0; i < pkglist->len; i++) {
                pkg = (DnfPackage*)g_ptr_array_index (pkglist, i);
                dnf_package_set_action(pkg, DNF_STATE_ACTION_REINSTALL);
                g_ptr_array_add(array, g_object_ref(pkg));
            }
            break;
        case DNF_PACKAGE_INFO_UPDATE:
            pkglist = hy_goal_list_upgrades(goal, NULL);
            for (i = 0; i < pkglist->len; i++) {
                pkg = (DnfPackage*)g_ptr_array_index (pkglist, i);
                dnf_package_set_action(pkg, DNF_STATE_ACTION_UPDATE);
                g_ptr_array_add(array, g_object_ref(pkg));
            }
            break;
        case DNF_PACKAGE_INFO_DOWNGRADE:
            pkglist = hy_goal_list_downgrades(goal, NULL);
            for (i = 0; i < pkglist->len; i++) {
                pkg = (DnfPackage*)g_ptr_array_index (pkglist, i);
                dnf_package_set_action(pkg, DNF_STATE_ACTION_DOWNGRADE);
                g_ptr_array_add(array, g_object_ref(pkg));
            }
            break;
        default:
            g_assert_not_reached();
        }
        g_ptr_array_unref(pkglist);
    }
    va_end(args);
    return array;
}

/**
 * dnf_goal_add_protected:
 * @goal: a #HyGoal.
 * @pset: a #DnfPackageSet that would be added to the protected packages.
 *
 * Since: 0.7.0
 */
void
dnf_goal_add_protected(HyGoal goal, DnfPackageSet *pset)
{
    goal->addProtected(*pset);
}

/**
 * dnf_goal_set_protected:
 * @goal: a #HyGoal.
 * @pset: a #DnfPackageSet of protected packages (the previous setup will be overridden).
 *
 * Since: 0.7.0
 */
void
dnf_goal_set_protected(HyGoal goal, DnfPackageSet *pset)
{
    goal->setProtected(*pset);
}
