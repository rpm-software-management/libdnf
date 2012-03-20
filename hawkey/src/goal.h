#ifndef GOAL_H
#define GOAL_H

// libsolv
#include "solv/queue.h"
#include "solv/transaction.h"

// hawkey
#include "package.h"
#include "packagelist.h"
#include "sack.h"
#include "types.h"

HyGoal hy_goal_create(HySack sack);
void hy_goal_free(HyGoal goal);

int hy_goal_erase(HyGoal goal, HyPackage pkg);
int hy_goal_install(HyGoal goal, HyPackage new_pkg);
int hy_goal_update(HyGoal goal, HyPackage new_pkg);
int hy_goal_go(HyGoal goal);

// problems
int hy_goal_count_problems(HyGoal goal);
char *hy_goal_describe_problem(HyGoal goal, unsigned i);

// result processing
HyPackageList hy_goal_list_erasures(HyGoal goal);
HyPackageList hy_goal_list_installs(HyGoal goal);
HyPackageList hy_goal_list_upgrades(HyGoal goal);
HyPackage hy_goal_package_upgrades(HyGoal goal, HyPackage pkg);

#endif