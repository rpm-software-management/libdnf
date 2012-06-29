#ifndef HY_GOAL_H
#define HY_GOAL_H

#ifdef __cplusplus
extern "C" {
#endif

/* libsolv */
#include "solv/queue.h"
#include "solv/transaction.h"

/* hawkey */
#include "package.h"
#include "packagelist.h"
#include "sack.h"
#include "types.h"

enum _hy_goal_op_flags {
    HY_CHECK_INSTALLED = 1,
};

enum _hy_goal_go_flags {
    HY_ALLOW_UNINSTALL = 1
};

HyGoal hy_goal_create(HySack sack);
void hy_goal_free(HyGoal goal);

int hy_goal_downgrade_to(HyGoal goal, HyPackage new_pkg);
int hy_goal_erase(HyGoal goal, HyPackage pkg);
int hy_goal_erase_query(HyGoal goal, HyQuery query);
int hy_goal_install(HyGoal goal, HyPackage new_pkg);
int hy_goal_install_query(HyGoal goal, HyQuery query);
int hy_goal_upgrade_all(HyGoal goal);
int hy_goal_upgrade_to(HyGoal goal, HyPackage new_pkg);
int hy_goal_upgrade_to_flags(HyGoal goal, HyPackage new_pkg, int flags);
int hy_goal_upgrade_query(HyGoal goal, HyQuery query);
int hy_goal_go(HyGoal goal);
int hy_goal_go_flags(HyGoal goal, int flags);

/* problems */
int hy_goal_count_problems(HyGoal goal);
char *hy_goal_describe_problem(HyGoal goal, unsigned i);

/* result processing */
HyPackageList hy_goal_list_erasures(HyGoal goal);
HyPackageList hy_goal_list_installs(HyGoal goal);
HyPackageList hy_goal_list_upgrades(HyGoal goal);
HyPackageList hy_goal_list_downgrades(HyGoal goal);
HyPackage hy_goal_package_obsoletes(HyGoal goal, HyPackage pkg);

#ifdef __cplusplus
}
#endif

#endif
