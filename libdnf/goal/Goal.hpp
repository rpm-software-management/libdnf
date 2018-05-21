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

#ifndef __GOAL_HPP
#define __GOAL_HPP

#include <memory>

#include "../dnf-types.h"
#include "../hy-goal.h"
#include "../hy-package.h"

namespace libdnf {

struct Goal {
public:
    Goal(DnfSack *sack);
    Goal(const Goal & goal_src);
    Goal(Goal && goal_src) = delete;
    ~Goal();

    DnfGoalActions getActions();
    int getReason(DnfPackage *pkg);
    DnfSack * getSack();
    Solver * getSolv();

    void addProtected(PackageSet & pset);
    void setProtected(const PackageSet & pset);

    void distupgrade();
    void distupgrade(DnfPackage *new_pkg);
    void distupgrade(HySelector);
    
    void erase(DnfPackage *pkg, int flags = 0);
    void erase(HySelector sltr, int flags);
    int install(DnfPackage *new_pkg, bool optional);
    bool install(HySelector sltr, bool optional, GError **error);
    int upgrade();
    int upgrade(DnfPackage *new_pkg);
    int upgrade(HySelector sltr);
    void userInstalled(DnfPackage *pkg);
    void userInstalled(PackageSet & pset);
    
    /* introspecting the requests */
    bool hasActions(DnfGoalActions action);

    int jobLength();

    /* resolving the goal */
    int run(DnfGoalActions flags);

    /* problems */
    int countProblems();
    
    DnfPackageSet * listConflictPkgs(DnfPackageState pkg_type);
    DnfPackageSet * listBrokenDependencyPkgs(DnfPackageState pkg_type);

    /**
    * @brief List describing failed rules in solving problem 'i'. Caller is responsible for freeing the
    * returned string list by g_free().
    *
    * @param goal HyGoal
    * @param i ingex of problem
    * @return char**
    */
    char ** describeProblemRules(unsigned i);
    int logDecisions();
    bool writeDebugdata(const char *dir, GError **error);

    /* result processing */
    std::unique_ptr<PackageSet> listErasures(GError **error);
    std::unique_ptr<PackageSet> listInstalls(GError **error);
    std::unique_ptr<PackageSet> listObsoleted(GError **error);
    std::unique_ptr<PackageSet> listReinstalls(GError **error);
    std::unique_ptr<PackageSet> listUnneeded(GError **error);
    std::unique_ptr<PackageSet> listUpgrades(GError **error);
    std::unique_ptr<PackageSet> listDowngrades(GError **error);
    std::unique_ptr<PackageSet> listObsoletedByPackage(DnfPackage *pkg);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}

#endif /* __GOAL_HPP */
