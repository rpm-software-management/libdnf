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
#include <stdexcept>
#include <vector>

#include "../dnf-types.h"
#include "../error.hpp"
#include "../hy-goal.h"
#include "../hy-package.h"

namespace libdnf {

struct Goal {
public:
    struct Error : public libdnf::Error {
        Error(const std::string & msg, int errCode) : libdnf::Error(msg), errCode(errCode) {}
        Error(const char * msg, int errCode) : libdnf::Error(msg), errCode(errCode) {}
        int getErrCode() const noexcept { return errCode; }
    private:
        int errCode;
    };

    Goal(DnfSack *sack);
    Goal(const Goal & goal_src);
    Goal(Goal && goal_src) = delete;
    ~Goal();

    DnfGoalActions getActions();
    int getReason(DnfPackage *pkg);
    DnfSack * getSack();

    void addProtected(PackageSet & pset);
    void setProtected(const PackageSet & pset);

    bool get_protect_running_kernel() const noexcept;
    void set_protect_running_kernel(bool value);

    void distupgrade();
    void distupgrade(DnfPackage *new_pkg);

    /**
    * @brief If selector ill formed, it rises std::runtime_error()
    */
    void distupgrade(HySelector);
    void erase(DnfPackage *pkg, int flags = 0);

    /**
    * @brief If selector ill formed, it rises std::runtime_error()
    *
    * @param sltr 
    * @param flags p_flags:...
    */
    void erase(HySelector sltr, int flags);
    void install(DnfPackage *new_pkg, bool optional);
    void lock(DnfPackage *new_pkg);
    void favor(DnfPackage *new_pkg);
    void add_exclude_from_weak(const DnfPackageSet & pset);
    void add_exclude_from_weak(DnfPackage *pkg);
    void reset_exclude_from_weak();
    void exclude_from_weak_autodetect();
    void disfavor(DnfPackage *new_pkg);

    /**
    * @brief If selector ill formed, it rises std::runtime_error()
    *
    * @param sltr p_sltr:...
    * @param optional Set true for optional tasks, false for strict tasks
    */
    void install(HySelector sltr, bool optional);

    /**
    * @brief Add upgrade all packages request to job. It adds obsoletes automatically to transaction
    */
    void upgrade();
    void upgrade(DnfPackage *new_pkg);

    /**
    * @brief If selector ill formed, it rises std::runtime_error()
    *
    * @param sltr p_sltr: It contains upgrade-to packages and obsoletes. The presence of installed
    * packages prevents reinstalling packages with the same NEVRA but changed contant. To honor repo
    * priority all relevant packages must be present. To upgrade package foo from priority repo, all
    * installed and available packages of the foo must be in selector plus obsoletes of foo.
    */
    void upgrade(HySelector sltr);
    void userInstalled(DnfPackage *pkg);
    void userInstalled(PackageSet & pset);

    /* introspecting the requests */
    bool hasActions(DnfGoalActions action);

    int jobLength();

    /* resolving the goal */
    bool run(DnfGoalActions flags);

    /* problems */
    int countProblems();

    std::unique_ptr<PackageSet> listConflictPkgs(DnfPackageState pkg_type);
    std::unique_ptr<PackageSet> listBrokenDependencyPkgs(DnfPackageState pkg_type);
    std::vector<std::vector<std::string>> describeAllProblemRules(bool pkgs);

    /**
    * @brief Check if any solver resolution problem is a file dependency issue
    */
    bool isBrokenFileDependencyPresent();

    /**
    * @brief List describing failed rules in solving problem 'i'. Caller is responsible for freeing the
    * returned string list by g_free().
    *
    * @param goal HyGoal
    * @param i ingex of problem
    * @param pkgs if true packages problem messages, othewise module messages are used
    * @return char**
    */
    std::vector<std::string> describeProblemRules(unsigned i, bool pkgs);
    int logDecisions();
    void writeDebugdata(const char *dir);

    /* result processing */
    PackageSet listErasures();
    PackageSet listInstalls();
    PackageSet listObsoleted();
    PackageSet listReinstalls();
    PackageSet listUnneeded();
    PackageSet listSuggested();
    PackageSet listUpgrades();
    PackageSet listDowngrades();
    PackageSet listObsoletedByPackage(DnfPackage * pkg);

    /// Concentrate all problems into a string
    static std::string formatAllProblemRules(const std::vector<std::vector<std::string>> & problems);
private:
    friend Query;
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}

#endif /* __GOAL_HPP */
