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

#ifndef __GOAL_PRIVATE_HPP
#define __GOAL_PRIVATE_HPP

#include "Goal.hpp"
#include "IdQueue.hpp"

namespace libdnf {

class Goal::Impl {
public:
    Impl(DnfSack * sack);
    Impl(const Goal::Impl & src_goal);
    ~Impl();
private:
    friend Goal;
    friend Query;

    DnfSack *sack;
    Queue staging;
    Solver *solv{nullptr};
    ::Transaction *trans{nullptr};
    DnfGoalActions actions{DNF_NONE};
    std::unique_ptr<PackageSet> protectedPkgs;
    std::unique_ptr<PackageSet> removalOfProtected;

    PackageSet listResults(Id type_filter1, Id type_filter2);
    void allowUninstallAllButProtected(Queue *job, DnfGoalActions flags);
    std::unique_ptr<IdQueue> constructJob(DnfGoalActions flags);
    bool solve(Queue *job, DnfGoalActions flags);
    Solver * initSolver();
    int limitInstallonlyPackages(Solver *solv, Queue *job);
    std::unique_ptr<IdQueue> conflictPkgs(unsigned i);
    std::unique_ptr<IdQueue> brokenDependencyPkgs(unsigned i);
    bool protectedInRemovals();
    std::string describeProtectedRemoval();
    std::unique_ptr<PackageSet> brokenDependencyAllPkgs(DnfPackageState pkg_type);
    int countProblems();
};

}

#endif /* __GOAL_PRIVATE_HPP */
