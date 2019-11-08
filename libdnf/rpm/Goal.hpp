#pragma once


#include "Package.hpp"
#include "PackageSet.hpp"
#include "Selector.hpp"


namespace libdnf::rpm {


/// @replaces libdnf:libdnf/goal/Goal.hpp:class:Goal
class Goal {
public:
    /// @replaces libdnf:libdnf/goal/Goal.hpp:method:Goal.getSack()
    /// @replaces libdnf:libdnf/hy-goal.h:function:hy_goal_get_sack(HyGoal goal)
    Sack & getSack() const;


    // ACTIONS

    /// @replaces dnf:dnf/base.py:method:Base.package_downgrade(self, pkg, strict=False)
    /// @replaces libdnf:libdnf/hy-goal.h:function:hy_goal_downgrade_to(HyGoal goal, DnfPackage * new_pkg)
    void downgrade(const Package & pkg);

    /// @replaces dnf:dnf/base.py:method:Base.package_install(self, pkg, strict=True)
    /// @replaces libdnf:libdnf/hy-goal.h:function:hy_goal_install(HyGoal goal, DnfPackage * new_pkg)
    void install(const Package & pkg);

    /// @replaces dnf:dnf/base.py:method:Base.package_reinstall(self, pkg)
    void reinstall(const Package & pkg);

    /// @replaces dnf:dnf/base.py:method:Base.package_remove(self, pkg)
    /// @replaces libdnf:libdnf/hy-goal.h:function:hy_goal_erase(HyGoal goal, DnfPackage * pkg)
    void remove(const Package & pkg);

    /// @replaces dnf:dnf/base.py:method:Base.package_upgrade(self, pkg)
    /// @replaces libdnf:libdnf/goal/Goal.hpp:method:Goal.upgrade()
    /// @replaces libdnf:libdnf/hy-goal.h:function:hy_goal_upgrade_to(HyGoal goal, DnfPackage * new_pkg)
    void upgrade(const Package & pkg);

    /// @replaces libdnf:libdnf/hy-goal.h:function:hy_goal_upgrade_all(HyGoal goal)
    void upgradeAll();

    /// @replaces libdnf:libdnf/hy-goal.h:function:hy_goal_distupgrade(HyGoal goal, DnfPackage * new_pkg)
    void distroSync(const Package & pkg);

    /// @replaces libdnf:libdnf/hy-goal.h:function:hy_goal_distupgrade_selector(HyGoal goal, HySelector )
    void distroSync(const Selector & sel);

    /// @replaces libdnf:libdnf/hy-goal.h:function:hy_goal_distupgrade_all(HyGoal goal)
    void distroSyncAll();


    // PROTECTED PACKAGES

    /// @replaces libdnf:libdnf/goal/Goal.hpp:method:Goal.addProtected(libdnf::PackageSet & pset)
    /// @replaces libdnf:libdnf/dnf-goal.h:function:dnf_goal_add_protected(HyGoal goal, DnfPackageSet * pset)
    void addProtected(const PackageSet & pset);

    /// @replaces libdnf:libdnf/goal/Goal.hpp:method:Goal.setProtected(const libdnf::PackageSet & pset)
    /// @replaces libdnf:libdnf/dnf-goal.h:function:dnf_goal_set_protected(HyGoal goal, DnfPackageSet * pset)
    void setProtected(const PackageSet & pset);


    // COMMON

    void reset();


    // RESULTS

    /// @replaces libdnf:libdnf/goal/Goal.hpp:method:Goal.listDowngrades()
    /// @replaces libdnf:libdnf/hy-goal.h:function:hy_goal_list_downgrades(HyGoal goal, GError ** error)
    void listDowngraded() const;

    /// @replaces libdnf:libdnf/goal/Goal.hpp:method:Goal.listInstalls()
    /// @replaces libdnf:libdnf/hy-goal.h:function:hy_goal_list_installs(HyGoal goal, GError ** error)
    void listInstalled() const;

    /// @replaces libdnf:libdnf/goal/Goal.hpp:method:Goal.listReinstalls()
    /// @replaces libdnf:libdnf/hy-goal.h:function:hy_goal_list_reinstalls(HyGoal goal, GError ** error)
    void listReinstalled() const;

    /// @replaces libdnf:libdnf/goal/Goal.hpp:method:Goal.listErasures()
    /// @replaces libdnf:libdnf/hy-goal.h:function:hy_goal_list_erasures(HyGoal goal, GError ** error)
    void listRemoved() const;

    /// @replaces libdnf:libdnf/goal/Goal.hpp:method:Goal.listUpgrades()
    /// @replaces libdnf:libdnf/hy-goal.h:function:hy_goal_list_upgrades(HyGoal goal, GError ** error)
    void listUpgraded() const;

    /// @replaces libdnf:libdnf/goal/Goal.hpp:method:Goal.listObsoleted()
    /// @replaces libdnf:libdnf/hy-goal.h:function:hy_goal_list_obsoleted(HyGoal goal, GError ** error)
    void listObsoleted() const;

    /// @replaces libdnf:libdnf/goal/Goal.hpp:method:Goal.listObsoletedByPackage(DnfPackage * pkg)
    /// @replaces libdnf:libdnf/hy-goal.h:function:hy_goal_list_obsoleted_by_package(HyGoal goal, DnfPackage * pkg)
    void listObsoletedByPackage(const Package & pkg) const;

    /// @replaces libdnf:libdnf/goal/Goal.hpp:method:Goal.listUnneeded()
    /// @replaces libdnf:libdnf/hy-goal.h:function:hy_goal_list_unneeded(HyGoal goal, GError ** error)
    void listUnneeded() const;

    /// @replaces libdnf:libdnf/goal/Goal.hpp:method:Goal.listSuggested()
    /// @replaces libdnf:libdnf/hy-goal.h:function:hy_goal_list_suggested(HyGoal goal, GError ** error)
    void listSuggested() const;

private:
    const Base & rpmBase;
    /// @replaces libdnf:libdnf/hy-goal.h:function:hy_goal_create(DnfSack * sack)
    Goal(Base & rpmBase);
};


Goal::Goal(Base & rpmBase)
    : rpmBase{rpmBase}
{
}


}  // namespace libdnf::rpm
