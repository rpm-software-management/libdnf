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

HyGoal goal_create(HySack sack);
void goal_free(HyGoal goal);

int goal_erase(HyGoal goal, HyPackage pkg);
int goal_install(HyGoal goal, HyPackage new_pkg);
int goal_update(HyGoal goal, HyPackage new_pkg);
int goal_go(HyGoal goal);

// problems
int goal_count_problems(HyGoal goal);
char *goal_describe_problem(HyGoal goal, unsigned i);

// result processing
HyPackageList goal_list_erasures(HyGoal goal);
HyPackageList goal_list_installs(HyGoal goal);
HyPackageList goal_list_upgrades(HyGoal goal);
HyPackage goal_package_upgrades(HyGoal goal, HyPackage pkg);

#endif