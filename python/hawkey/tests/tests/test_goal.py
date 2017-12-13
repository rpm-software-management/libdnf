#
# Copyright (C) 2012-2013 Red Hat, Inc.
#
# Licensed under the GNU Lesser General Public License Version 2.1
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#

from __future__ import absolute_import
from . import base
from copy import deepcopy

import hawkey

class GoalTest(base.TestCase):
    def setUp(self):
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()
        self.sack.load_test_repo("main", "main.repo")

    def test_actions(self):
        sltr = hawkey.Selector(self.sack).set(name="walrus")
        goal = hawkey.Goal(self.sack)

        self.assertEqual(0, goal.actions & (hawkey.ERASE | hawkey.DISTUPGRADE |
            hawkey.DISTUPGRADE_ALL | hawkey.DOWNGRADE | hawkey.INSTALL | hawkey.UPGRADE |
            hawkey.UPGRADE_ALL))
        goal.upgrade(select=sltr)
        self.assertEqual(hawkey.UPGRADE, goal.actions)

    def test_clone(self):
        pkg = base.by_name(self.sack, "penny-lib")
        goal = hawkey.Goal(self.sack)
        goal.erase(pkg)
        self.assertFalse(goal.run())

        goal2 = deepcopy(goal)
        self.assertTrue(goal2.run(allow_uninstall=True))
        self.assertEqual(len(goal2.list_erasures()), 2)

        goal3 = deepcopy(goal)
        goal3.add_protected(hawkey.Query(self.sack).filter(name="flying"))
        self.assertFalse(goal3.run(allow_uninstall=True))

    def test_list_err(self):
        goal = hawkey.Goal(self.sack)
        self.assertRaises(hawkey.ValueException, goal.list_installs)

    def test_empty_selector(self):
        sltr = hawkey.Selector(self.sack)
        goal = hawkey.Goal(self.sack)
        # does not raise ValueException
        goal.install(select=sltr)
        goal.run()
        self.assertEqual(goal.list_installs(), [])

    def test_install_selector(self):
        sltr = hawkey.Selector(self.sack).set(name="walrus")
        # without checking versioning, the update is accepted:
        self.assertIsNone(hawkey.Goal(self.sack).upgrade(select=sltr))

    def test_install_selector_weak(self):
        sltr = hawkey.Selector(self.sack).set(name='hello')
        goal = hawkey.Goal(self.sack)
        goal.install(select=sltr, optional=True)
        self.assertTrue(goal.run())

    def test_install_selector_err(self):
        sltr = hawkey.Selector(self.sack)
        self.assertRaises(hawkey.ValueException, sltr.set, undefined="eapoe")

        sltr = hawkey.Selector(self.sack).set(name="semolina", arch="i666")
        goal = hawkey.Goal(self.sack)
        self.assertRaises(hawkey.ArchException, goal.install, select=sltr)

    def test_reinstall(self):
        inst = base.by_name_repo(self.sack, "fool", hawkey.SYSTEM_REPO_NAME)
        avail = base.by_name_repo(self.sack, "fool", "main")
        goal = hawkey.Goal(self.sack)
        goal.install(avail)
        self.assertTrue(goal.run())
        self.assertLength(goal.list_erasures(), 0)
        self.assertLength(goal.list_installs(), 0)
        self.assertLength(goal.list_reinstalls(), 1)
        reinstall = goal.list_reinstalls()[0]
        obsoleted = goal.obsoleted_by_package(reinstall)
        self.assertItemsEqual(list(map(str, obsoleted)), ("fool-1-3.noarch", ))

    def test_req(self):
        goal = hawkey.Goal(self.sack)
        self.assertEqual(goal.req_length(), 0)
        self.assertFalse(goal.req_has_erase())
        sltr = hawkey.Selector(self.sack).set(name="jay")
        goal.erase(select=sltr)
        self.assertEqual(goal.req_length(), 1)
        self.assertTrue(goal.req_has_erase())

        goal = hawkey.Goal(self.sack)
        goal.upgrade(select=sltr)
        self.assertFalse(goal.req_has_erase())

        goal = hawkey.Goal(self.sack)
        pkg = hawkey.Query(self.sack).filter(name="dog")[0]
        goal.erase(pkg, clean_deps=True)
        self.assertTrue(goal.req_has_erase())


class GoalRunAll(base.TestCase):
    def setUp(self):
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()
        self.sack.load_test_repo("greedy", "greedy.repo")
        self.goal = hawkey.Goal(self.sack)

    def test_goal_install_weak_deps(self):
        pkg_b = base.by_name(self.sack, "B")
        self.goal.install(pkg_b)
        self.assertTrue(self.goal.run())
        installs = self.goal.list_installs()
        self.assertEqual(len(installs), 2)
        expected_installs = ("B-1-0.noarch", "C-1-0.noarch")
        self.assertItemsEqual(list(map(str, installs)), expected_installs)
        goal2 = deepcopy(self.goal)
        self.assertTrue(goal2.run(ignore_weak_deps=True))
        installs2 = goal2.list_installs()
        self.assertEqual(len(goal2.list_installs()), 1)
        self.assertEqual(str(installs2[0]), "B-1-0.noarch")
