#ifndef GOAL_H
#define GOAL_H

// libsolv
#include "solv/queue.h"
#include "solv/transaction.h"

// hawkey
#include "package.h"
#include "packagelist.h"
#include "sack.h"

struct _Goal {
    Sack sack;
    Queue job;
    Queue problems;
    Transaction *trans;
};

typedef struct _Goal * Goal;

Goal goal_create(Sack sack);
void goal_free(Goal goal);

int goal_erase(Goal goal, Package pkg);
int goal_install(Goal goal, Package new_pkg);
int goal_update(Goal goal, Package new_pkg);
int goal_go(Goal goal);

// problems
int goal_count_problems(Goal goal);
char *goal_describe_problem(Goal goal, unsigned i);

// result processing
PackageList goal_list_erasures(Goal goal);
PackageList goal_list_installs(Goal goal);
PackageList goal_list_upgrades(Goal goal);
Package goal_package_upgrades(Goal goal, Package pkg);

#endif