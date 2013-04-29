#ifndef HY_GOAL_H
#define HY_GOAL_H

#ifdef __cplusplus
extern "C" {
#endif

// hawkey
#include "types.h"

enum _hy_goal_op_flags {
    HY_CHECK_INSTALLED	= 1 << 0,
    HY_CLEAN_DEPS	= 1 << 1
};

enum _hy_goal_run_flags {
    HY_ALLOW_UNINSTALL = 1 << 0,
    HY_FORCE_BEST = 1 << 1
};

#define HY_REASON_DEP 1
#define HY_REASON_USER 2

HyGoal hy_goal_create(HySack sack);
void hy_goal_free(HyGoal goal);

int hy_goal_distupgrade_all(HyGoal goal);
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
int hy_goal_install_selector(HyGoal goal, HySelector sltr);
int hy_goal_upgrade_all(HyGoal goal);
int hy_goal_upgrade_to(HyGoal goal, HyPackage new_pkg);
int hy_goal_upgrade_to_flags(HyGoal goal, HyPackage new_pkg, int flags);
int hy_goal_upgrade_selector(HyGoal goal, HySelector sltr);
int hy_goal_upgrade_to_selector(HyGoal goal, HySelector sltr);
int hy_goal_userinstalled(HyGoal goal, HyPackage pkg);
int hy_goal_run(HyGoal goal);
int hy_goal_run_flags(HyGoal goal, int flags);
int hy_goal_run_all(HyGoal goal, hy_solution_callback cb, void *cb_data);
int hy_goal_run_all_flags(HyGoal goal, hy_solution_callback cb, void *cb_data,
			     int flags);

/* problems */
int hy_goal_count_problems(HyGoal goal);
char *hy_goal_describe_problem(HyGoal goal, unsigned i);
int hy_goal_log_decisions(HyGoal goal);
int hy_goal_write_debugdata(HyGoal goal);

/* result processing */
HyPackageList hy_goal_list_erasures(HyGoal goal);
HyPackageList hy_goal_list_installs(HyGoal goal);
HyPackageList hy_goal_list_obsoleted(HyGoal goal);
HyPackageList hy_goal_list_reinstalls(HyGoal goal);
HyPackageList hy_goal_list_upgrades(HyGoal goal);
HyPackageList hy_goal_list_downgrades(HyGoal goal);
HyPackageList hy_goal_list_obsoleted_by_package(HyGoal goal, HyPackage pkg);
int hy_goal_get_reason(HyGoal goal, HyPackage pkg);

#ifdef __cplusplus
}
#endif

#endif
