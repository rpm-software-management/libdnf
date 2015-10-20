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
        self.assertEqual(set(), goal.actions)
        goal.upgrade(select=sltr)
        self.assertEqual(set([hawkey.UPGRADE]), goal.actions)
        goal.install(name="semolina")
        self.assertEqual(set([hawkey.UPGRADE, hawkey.INSTALL]), goal.actions)

    def test_clone(self):
        pkg = base.by_name(self.sack, "penny-lib")
        goal = hawkey.Goal(self.sack)
        goal.erase(pkg)
        self.assertFalse(goal.run())

        goal2 = deepcopy(goal)
        self.assertTrue(goal2.run(allow_uninstall=True))
        self.assertEqual(len(goal2.list_erasures()), 2)

    def test_list_err(self):
        goal = hawkey.Goal(self.sack)
        self.assertRaises(hawkey.ValueException, goal.list_installs)

    def test_upgrade(self):
        # select the installed "fool":
        pkg = hawkey.Query(self.sack).filter(name="walrus")[0]
        # without checking versioning, the update is accepted:
        self.assertIsNone(hawkey.Goal(self.sack).
                          upgrade_to(pkg, check_installed=False))
        # with the check it is not:
        goal = hawkey.Goal(self.sack)
        self.assertRaises(hawkey.Exception, goal.upgrade_to, package=pkg,
                          check_installed=True)
        # default value for check_installed is False:
        self.assertIsNone(hawkey.Goal(self.sack).upgrade_to(pkg))

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

        goal = hawkey.Goal(self.sack)
        goal.install(name="semolina")
        goal.run()
        self.assertEqual(str(goal.list_installs()[0]), 'semolina-2-0.x86_64')

    def test_install_selector_weak(self):
        sltr = hawkey.Selector(self.sack).set(name='hello')
        goal = hawkey.Goal(self.sack)
        goal.install(select=sltr, optional=True)
        self.assertTrue(goal.run())

    def test_erase_selector(self):
        """ Tests automatic Selector from keyword arguments, with special
            keywords that don't become a part of the Selector.
        """
        goal = hawkey.Goal(self.sack)
        goal.erase(clean_deps=True, name="flying")
        goal.run()
        self.assertEqual(len(goal.list_erasures()), 2)

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
        goal.upgrade_to(select=sltr)
        self.assertFalse(goal.req_has_erase())

        goal = hawkey.Goal(self.sack)
        pkg = hawkey.Query(self.sack).filter(name="dog")[0]
        goal.erase(pkg, clean_deps=True)
        self.assertTrue(goal.req_has_erase())

class Collector(object):
    def __init__(self):
        self.cnt = 0
        self.erasures = set()
        self.pkgs = set()

    def new_solution_cb(self, goal):
        self.cnt += 1
        self.erasures.update(goal.list_erasures())
        self.pkgs.update(goal.list_installs())

    def new_solution_cb_borked(self, goal):
        """ Raises AttributeError. """
        self.pkgs_borked.update(goal.list_erasures())

class GoalRun(base.TestCase):
    def test_run_callback(self):
        "Test goal.run() can use callback parameter just as well as run_all()"
        sack = base.TestSack(repo_dir=self.repo_dir)
        sack.load_system_repo()
        sack.load_test_repo("main", "main.repo")

        pkg = base.by_name(sack, "penny-lib")
        goal = hawkey.Goal(sack)
        goal.erase(pkg)
        collector = Collector()
        self.assertTrue(goal.run(allow_uninstall=True,
                                 callback=collector.new_solution_cb))
        self.assertEqual(collector.cnt, 1)
        self.assertEqual(len(collector.erasures), 2)

class GoalRunAll(base.TestCase):
    def setUp(self):
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()
        self.sack.load_test_repo("greedy", "greedy.repo")
        self.goal = hawkey.Goal(self.sack)

    def test_cb(self):
        pkg_a = base.by_name(self.sack, "A")
        pkg_b = base.by_name(self.sack, "B")
        pkg_c = base.by_name(self.sack, "C")
        self.goal.install(pkg_a)

        collector = Collector()
        self.assertTrue(self.goal.run_all(collector.new_solution_cb))
        self.assertItemsEqual(collector.pkgs, [pkg_a, pkg_b, pkg_c])

    def test_cb_borked(self):
        """ Check exceptions are propagated from the callback. """
        self.goal.install(base.by_name(self.sack, "A"))
        collector = Collector()
        self.assertRaises(AttributeError,
                          self.goal.run_all, collector.new_solution_cb_borked)

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


class Problems(base.TestCase):
    def setUp(self):
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()
        self.sack.load_test_repo("main", "main.repo")
        self.goal = hawkey.Goal(self.sack)

    def test_errors(self):
        pkg = base.by_name(self.sack, "hello")
        self.goal.install(pkg)
        self.assertFalse(self.goal.run())
        self.assertEqual(len(self.goal.problems), 1)
        self.assertRaises(hawkey.RuntimeException, self.goal.list_erasures)
        self.assertRaises(ValueError, self.goal.describe_problem, 1);

class Verify(base.TestCase):
    def setUp(self):
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_test_repo(hawkey.SYSTEM_REPO_NAME, "@System-broken.repo", True)
        self.goal = hawkey.Goal(self.sack)

    def test_verify(self):
        self.assertFalse(self.goal.run(verify=True))
        self.assertEqual(len(self.goal.problems), 2)
        self.assertEqual(self.goal.problems[0],
            "nothing provides missing-dep needed by missing-1-0.x86_64")
        self.assertEqual(self.goal.problems[1],
            "package conflict-1-0.x86_64 conflicts with ok provided by ok-1-0.x86_64")
