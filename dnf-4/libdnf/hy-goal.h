/*
 * Copyright (C) 2012-2013 Red Hat, Inc.
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

#ifndef HY_GOAL_H
#define HY_GOAL_H

#include <glib.h>
#include <stdbool.h>

G_BEGIN_DECLS

#include "hy-types.h"
#include "dnf-sack.h"
#include "dnf-utils.h"

enum _hy_goal_op_flags {
    HY_CHECK_INSTALLED          = 1 << 0,
    HY_CLEAN_DEPS               = 1 << 1,
    HY_WEAK_SOLV                = 1 << 2
};

typedef enum {
    DNF_NONE                     = 0,
    DNF_ERASE                    = 1 << 0,
    DNF_DISTUPGRADE              = 1 << 1,
    DNF_DISTUPGRADE_ALL          = 1 << 2,
    DNF_DOWNGRADE                = 1 << 3,
    DNF_INSTALL                  = 1 << 4,
    DNF_UPGRADE                  = 1 << 5,
    DNF_UPGRADE_ALL              = 1 << 6,

    // hy_goal_run flags
    DNF_ALLOW_UNINSTALL          = 1 << 10,
    DNF_FORCE_BEST               = 1 << 11,
    DNF_VERIFY                   = 1 << 12,
    DNF_IGNORE_WEAK_DEPS         = 1 << 13,
    DNF_ALLOW_DOWNGRADE          = 1 << 14,
    DNF_IGNORE_WEAK              = 1 << 15
} DnfGoalActions;

typedef enum {
    DNF_PACKAGE_STATE_ALL              = 0,
    DNF_PACKAGE_STATE_AVAILABLE        = 1,
    DNF_PACKAGE_STATE_INSTALLED        = 2
} DnfPackageState;

#define HY_REASON_DEP 1
#define HY_REASON_USER 2
#define HY_REASON_CLEAN 3
#define HY_REASON_WEAKDEP 4

HyGoal hy_goal_create(DnfSack *sack);
HyGoal hy_goal_clone(HyGoal goal);
void hy_goal_free(HyGoal goal);
DnfSack *hy_goal_get_sack(HyGoal goal);

int hy_goal_distupgrade_all(HyGoal goal);
int hy_goal_distupgrade(HyGoal goal, DnfPackage *new_pkg);
int hy_goal_distupgrade_selector(HyGoal goal, HySelector);

/**
* @brief Mark a package to install. It doesn't check if the package has a lower version than
* installed package or even installed. It allows to downgrade dependencies if needed. It return
* always 0.
*
* @param goal HyGoal
* @param new_pkg DnfPackage
* @return int
*/
DEPRECATED("Will be removed after 2018-03-01. Use hy_goal_install() instead.")
int hy_goal_downgrade_to(HyGoal goal, DnfPackage *new_pkg);
int hy_goal_erase(HyGoal goal, DnfPackage *pkg);
int hy_goal_erase_flags(HyGoal goal, DnfPackage *pkg, int flags);

/**
 * Erase packages specified by the Selector.
 *
 * @returns        0 on success, DNF_ERROR_BAD_SELECTOR for an invalid Selector.
 */
int hy_goal_erase_selector_flags(HyGoal goal, HySelector sltr, int flags);

/**
* @brief Mark package to install or if installed to downgrade or upgrade. It allows to downgrade
* dependencies if needed. Return value is always 0.
*
* @param goal HyGoal
* @param new_pkg Package to install
* @return int
*/
int hy_goal_install(HyGoal goal, DnfPackage *new_pkg);

/**
* @brief Mark package to install or if installed to downgrade or upgrade. In case that package
* cannot be install, it can be skipped without an error. It allows to downgrade dependencies if
* needed. Return value is always 0.
*
* @param goal HyGoal
* @param new_pkg Package to install
* @return int
*/
int hy_goal_install_optional(HyGoal goal, DnfPackage *new_pkg);

/**
* @brief Mark content of HySelector to install or if installed to downgrade or upgrade. Only one
* option will be chosen. It allows to downgrade dependencies if needed. If not supported
* combination in selectort, it triggers assertion raise.
*
* @param goal HyGoal
* @param sltr HySelector
* @param error p_error:...
* @return gboolean
*/
gboolean hy_goal_install_selector(HyGoal goal, HySelector sltr, GError **error);

/**
* @brief Mark content of HySelector to install or if installed to downgrade or upgrade. In case that
* any package in selector cannot be install, it can be skipped without an error. Only one option
* will be chosen. It allows to downgrade dependencies if needed. If not supported combination in
* selectort, it triggers assertion raise.
*
* @param goal HyGoal
* @param sltr HySelector
* @param error p_error:...
* @return gboolean
*/
gboolean hy_goal_install_selector_optional(HyGoal goal, HySelector sltr, GError **error);
int hy_goal_upgrade_all(HyGoal goal);
int hy_goal_upgrade_to(HyGoal goal, DnfPackage *new_pkg);
int hy_goal_upgrade_selector(HyGoal goal, HySelector sltr);
int hy_goal_userinstalled(HyGoal goal, DnfPackage *pkg);

/* introspecting the requests */
int hy_goal_has_actions(HyGoal goal, DnfGoalActions action);

int hy_goal_req_length(HyGoal goal);

/* resolving the goal */
int hy_goal_run_flags(HyGoal goal, DnfGoalActions flags);

/* problems */
int hy_goal_count_problems(HyGoal goal);
DnfPackageSet *hy_goal_conflict_all_pkgs(HyGoal goal, DnfPackageState pkg_type);
DnfPackageSet *hy_goal_broken_dependency_all_pkgs(HyGoal goal, DnfPackageState pkg_type);

int hy_goal_log_decisions(HyGoal goal);
bool hy_goal_write_debugdata(HyGoal goal, const char *dir, GError **error);

/* result processing */
GPtrArray *hy_goal_list_erasures(HyGoal goal, GError **error);
GPtrArray *hy_goal_list_installs(HyGoal goal, GError **error);
GPtrArray *hy_goal_list_obsoleted(HyGoal goal, GError **error);
GPtrArray *hy_goal_list_reinstalls(HyGoal goal, GError **error);
GPtrArray *hy_goal_list_unneeded(HyGoal goal, GError **error);
GPtrArray *hy_goal_list_suggested(HyGoal goal, GError **error);
GPtrArray *hy_goal_list_upgrades(HyGoal goal, GError **error);
GPtrArray *hy_goal_list_downgrades(HyGoal goal, GError **error);
GPtrArray *hy_goal_list_obsoleted_by_package(HyGoal goal, DnfPackage *pkg);
int hy_goal_get_reason(HyGoal goal, DnfPackage *pkg);

G_END_DECLS

#endif
