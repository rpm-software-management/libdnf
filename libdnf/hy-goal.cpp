/*
 * Copyright (C) 2012-2018 Red Hat, Inc.
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

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

// libsolv
extern "C" {
#include <solv/evr.h>
#include <solv/policy.h>
#include <solv/selection.h>
#include <solv/solver.h>
#include <solv/solverdebug.h>
#include <solv/testcase.h>
#include <solv/transaction.h>
#include <solv/util.h>
#include <solv/poolid.h>
}

// hawkey
#include "dnf-types.h"
#include "hy-goal-private.hpp"
#include "hy-iutil-private.hpp"
#include "hy-package-private.hpp"
#include "hy-packageset-private.hpp"
#include "hy-query-private.hpp"
#include "hy-repo-private.hpp"
#include "dnf-sack-private.hpp"
#include "dnf-goal.h"
#include "dnf-package.h"
#include "hy-selector-private.hpp"
#include "hy-util-private.hpp"
#include "dnf-package.h"
#include "hy-package.h"
#include "goal/Goal.hpp"
#include "sack/packageset.hpp"

#include "utils/bgettext/bgettext-lib.h"

#define BLOCK_SIZE 15

// internal functions to translate Selector into libsolv Job

inline static void
exceptionToGError(GError **error, const libdnf::Goal::Exception & e)
{
    g_set_error_literal(error, DNF_ERROR, e.getErrCode(), e.what());
}

// public functions

HyGoal
hy_goal_clone(HyGoal goal)
{
    return new libdnf::Goal(*goal);
}

HyGoal
hy_goal_create(DnfSack *sack)
{
    return new libdnf::Goal(sack);
}

void
hy_goal_free(HyGoal goal)
{
    delete goal;
}

DnfSack *
hy_goal_get_sack(HyGoal goal)
{
    return goal->getSack();
}

int
hy_goal_distupgrade_all(HyGoal goal)
{
    goal->distupgrade();
    return 0;
}

int
hy_goal_distupgrade(HyGoal goal, DnfPackage *new_pkg)
{
    goal->distupgrade(new_pkg);
    return 0;
}

int
hy_goal_distupgrade_selector(HyGoal goal, HySelector sltr)
{
    try {
        goal->distupgrade(sltr);
    } catch (libdnf::Goal::Exception & e) {
        return e.getErrCode();
    }
    return 0;
}

int
hy_goal_downgrade_to(HyGoal goal, DnfPackage *new_pkg)
{
    goal->install(new_pkg, false);
    return 0;
}

int
hy_goal_erase(HyGoal goal, DnfPackage *pkg)
{
    goal->erase(pkg);
    return 0;
}

int
hy_goal_erase_flags(HyGoal goal, DnfPackage *pkg, int flags)
{
    goal->erase(pkg, flags);
    return 0;
}

int
hy_goal_erase_selector_flags(HyGoal goal, HySelector sltr, int flags)
{
    try {
        goal->erase(sltr, flags);
    } catch (libdnf::Goal::Exception & e) {
        return e.getErrCode();
    }
    return 0;
}

int
hy_goal_has_actions(HyGoal goal, DnfGoalActions action)
{
    return goal->hasActions(action);
}

int
hy_goal_install(HyGoal goal, DnfPackage *new_pkg)
{
    goal->install(new_pkg, false);
    return 0;
}

int
hy_goal_install_optional(HyGoal goal, DnfPackage *new_pkg)
{
    goal->install(new_pkg, true);
    return 0;
}

gboolean
hy_goal_install_selector(HyGoal goal, HySelector sltr, GError **error)
{
    try {
        goal->install(sltr, false);
    } catch (const libdnf::Goal::Exception & e) {
        exceptionToGError(error, e);
        return FALSE;
    }
    return TRUE;
}

gboolean
hy_goal_install_selector_optional(HyGoal goal, HySelector sltr, GError **error)
{
    try {
        goal->install(sltr, true);
    } catch (const libdnf::Goal::Exception & e) {
        exceptionToGError(error, e);
        return FALSE;
    }
    return TRUE;
}

int
hy_goal_upgrade_all(HyGoal goal)
{
    goal->upgrade();
    return 0;
}

int
hy_goal_upgrade_to(HyGoal goal, DnfPackage *new_pkg)
{
    goal->upgrade(new_pkg);
    return 0;
}

int
hy_goal_upgrade_selector(HyGoal goal, HySelector sltr)
{
    try {
        goal->upgrade(sltr);
    } catch (libdnf::Goal::Exception & e) {
        return e.getErrCode();
    }
    return 0;
}

int
hy_goal_userinstalled(HyGoal goal, DnfPackage *pkg)
{
    goal->userInstalled(pkg);
    return 0;
}

int hy_goal_req_length(HyGoal goal)
{
    return goal->jobLength();
}

int
hy_goal_run_flags(HyGoal goal, DnfGoalActions flags)
{
    return goal->run(flags);
}

int
hy_goal_count_problems(HyGoal goal)
{
    return goal->countProblems();
}

/**
 * Reports packages that has a conflict
 *
 * available - if available it returns set with available packages with conflicts
 * available - if package installed it also excludes available packages with same NEVRA
 *
 * Returns DnfPackageSet with all packages that have a conflict.
 */
DnfPackageSet *
hy_goal_conflict_all_pkgs(HyGoal goal, DnfPackageState pkg_type)
{
    return goal->listConflictPkgs(pkg_type).release();
}

/**
 * Reports all packages that have broken dependency
 * available - if available returns only available packages with broken dependencies
 * available - if package installed it also excludes available packages with same NEVRA
 * Returns DnfPackageSet with all packages with broken dependency
 */
DnfPackageSet *
hy_goal_broken_dependency_all_pkgs(HyGoal goal, DnfPackageState pkg_type)
{
    return goal->listBrokenDependencyPkgs(pkg_type).release();
}

/**
 * Write all the solving decisions to the hawkey logfile.
 */
int
hy_goal_log_decisions(HyGoal goal)
{
    return goal->logDecisions();
}

/**
 * hy_goal_write_debugdata:
 * @goal: A #HyGoal
 * @dir: The directory to write to
 * @error: A #GError, or %NULL
 *
 * Writes details about the testcase to a directory.
 *
 * Returns: %FALSE if an error was set
 *
 * Since: 0.7.0
 */
bool
hy_goal_write_debugdata(HyGoal goal, const char *dir, GError **error)
{
    try {
        goal->writeDebugdata(dir);
    } catch (const libdnf::Goal::Exception & e) {
        exceptionToGError(error, e);
        return false;
    }
    return true;
}

GPtrArray *
hy_goal_list_erasures(HyGoal goal, GError **error)
{
    try {
        auto pset = goal->listErasures();
        return packageSet2GPtrArray(&pset);
    } catch (const libdnf::Goal::Exception & e) {
        exceptionToGError(error, e);
        return NULL;
    }
}

GPtrArray *
hy_goal_list_installs(HyGoal goal, GError **error)
{
    try {
        auto pset = goal->listInstalls();
        return packageSet2GPtrArray(&pset);
    } catch (const libdnf::Goal::Exception & e) {
        exceptionToGError(error, e);
        return NULL;
    }
}

GPtrArray *
hy_goal_list_obsoleted(HyGoal goal, GError **error)
{
    try {
        auto pset = goal->listObsoleted();
        return packageSet2GPtrArray(&pset);
    } catch (const libdnf::Goal::Exception & e) {
        exceptionToGError(error, e);
        return NULL;
    }
}

GPtrArray *
hy_goal_list_reinstalls(HyGoal goal, GError **error)
{
    try {
        auto pset = goal->listReinstalls();
        return packageSet2GPtrArray(&pset);
    } catch (const libdnf::Goal::Exception & e) {
        exceptionToGError(error, e);
        return NULL;
    }
}

GPtrArray *
hy_goal_list_unneeded(HyGoal goal, GError **error)
{
    try {
        auto pset = goal->listUnneeded();
        return packageSet2GPtrArray(&pset);
    } catch (const libdnf::Goal::Exception & e) {
        exceptionToGError(error, e);
        return NULL;
    }
}

GPtrArray *
hy_goal_list_suggested(HyGoal goal, GError **error)
{
    try {
        auto pset = goal->listSuggested();
        return packageSet2GPtrArray(&pset);
    } catch (const libdnf::Goal::Exception & e) {
        exceptionToGError(error, e);
        return NULL;
    }
}

GPtrArray *
hy_goal_list_upgrades(HyGoal goal, GError **error)
{
    try {
        auto pset = goal->listUpgrades();
        return packageSet2GPtrArray(&pset);
    } catch (const libdnf::Goal::Exception & e) {
        exceptionToGError(error, e);
        return NULL;
    }
}

GPtrArray *
hy_goal_list_downgrades(HyGoal goal, GError **error)
{
    try {
        auto pset = goal->listDowngrades();
        return packageSet2GPtrArray(&pset);
    } catch (const libdnf::Goal::Exception & e) {
        exceptionToGError(error, e);
        return NULL;
    }
}

GPtrArray *
hy_goal_list_obsoleted_by_package(HyGoal goal, DnfPackage *pkg)
{
    auto pset = goal->listObsoletedByPackage(pkg);
    return packageSet2GPtrArray(&pset);
}

int
hy_goal_get_reason(HyGoal goal, DnfPackage *pkg)
{
    return goal->getReason(pkg);
}
