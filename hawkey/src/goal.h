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

#ifdef __cplusplus
extern "C" {
#endif

// hawkey
#include "types.h"

enum _hy_goal_op_flags {
    HY_CHECK_INSTALLED	= 1 << 0,
    HY_CLEAN_DEPS	= 1 << 1,
    HY_WEAK_SOLV	= 1 << 2
};

enum _hy_goal_run_flags {
    HY_ALLOW_UNINSTALL = 1 << 0,
    HY_FORCE_BEST = 1 << 1,
    HY_VERIFY = 1 << 2,
    HY_IGNORE_WEAK_DEPS = 1 << 3
};

enum _hy_goal_actions {
    HY_ERASE		= 1 << 0,
    HY_DISTUPGRADE	= 1 << 1,
    HY_DISTUPGRADE_ALL	= 1 << 2,
    HY_DOWNGRADE	= 1 << 3,
    HY_INSTALL		= 1 << 4,
    HY_UPGRADE		= 1 << 5,
    HY_UPGRADE_ALL	= 1 << 6,
};

#define HY_REASON_DEP 1
#define HY_REASON_USER 2

HyGoal hy_goal_create(HySack sack);
HyGoal hy_goal_clone(HyGoal goal);
void hy_goal_free(HyGoal goal);

int hy_goal_distupgrade_all(HyGoal goal);
int hy_goal_distupgrade(HyGoal goal, HyPackage new_pkg);
int hy_goal_distupgrade_selector(HyGoal goal, HySelector);
int hy_goal_downgrade_to(HyGoal goal, HyPackage new_pkg);
int hy_goal_erase(HyGoal goal, HyPackage pkg);
int hy_goal_erase_flags(HyGoal goal, HyPackage pkg, int flags);

/**
 * Erase packages specified by the Selector.
 *
 * @returns	0 on success, HY_E_SELECTOR for an invalid Selector.
 */
int hy_goal_erase_selector(HyGoal goal, HySelector sltr);
int hy_goal_erase_selector_flags(HyGoal goal, HySelector sltr, int flags);
int hy_goal_install(HyGoal goal, HyPackage new_pkg);
int hy_goal_install_optional(HyGoal goal, HyPackage new_pkg);
int hy_goal_install_selector(HyGoal goal, HySelector sltr);
int hy_goal_install_selector_optional(HyGoal goal, HySelector sltr);
int hy_goal_upgrade_all(HyGoal goal);
int hy_goal_upgrade_to(HyGoal goal, HyPackage new_pkg);
int hy_goal_upgrade_to_flags(HyGoal goal, HyPackage new_pkg, int flags);
int hy_goal_upgrade_selector(HyGoal goal, HySelector sltr);
int hy_goal_upgrade_to_selector(HyGoal goal, HySelector sltr);
int hy_goal_userinstalled(HyGoal goal, HyPackage pkg);

/* introspecting the requests */
int hy_goal_has_actions(HyGoal goal, int action);

// deprecated in 0.5.9, will be removed in 1.0.0
// use hy_goal_has_actions(goal, HY_DISTUPGRADE_ALL) instead
int hy_goal_req_has_distupgrade_all(HyGoal goal);

// deprecated in 0.5.9, will be removed in 1.0.0
// use hy_goal_has_actions(goal, HY_DISTUPGRADE_ALL) instead
int hy_goal_req_has_erase(HyGoal goal);

// deprecated in 0.5.9, will be removed in 1.0.0
// use hy_goal_has_actions(goal, HY_UPGRADE_ALL) instead
int hy_goal_req_has_upgrade_all(HyGoal goal);

int hy_goal_req_length(HyGoal goal);

/* resolving the goal */
int hy_goal_run(HyGoal goal);
int hy_goal_run_flags(HyGoal goal, int flags);
int hy_goal_run_all(HyGoal goal, hy_solution_callback cb, void *cb_data);
int hy_goal_run_all_flags(HyGoal goal, hy_solution_callback cb, void *cb_data,
			  int flags);

/* problems */
int hy_goal_count_problems(HyGoal goal);
char *hy_goal_describe_problem(HyGoal goal, unsigned i);
int hy_goal_log_decisions(HyGoal goal);
int hy_goal_write_debugdata(HyGoal goal, const char *dir);

/* result processing */
HyPackageList hy_goal_list_erasures(HyGoal goal);
HyPackageList hy_goal_list_installs(HyGoal goal);
HyPackageList hy_goal_list_obsoleted(HyGoal goal);
HyPackageList hy_goal_list_reinstalls(HyGoal goal);
HyPackageList hy_goal_list_unneeded(HyGoal goal);
HyPackageList hy_goal_list_upgrades(HyGoal goal);
HyPackageList hy_goal_list_downgrades(HyGoal goal);
HyPackageList hy_goal_list_obsoleted_by_package(HyGoal goal, HyPackage pkg);
int hy_goal_get_reason(HyGoal goal, HyPackage pkg);

#ifdef __cplusplus
}
#endif

#endif
