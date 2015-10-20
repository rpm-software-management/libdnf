#
# Copyright (C) 2012-2015 Red Hat, Inc.
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

import copy
import os
import sys
import unittest

from . import base
import hawkey
import hawkey.test

class TestSackTest(base.TestCase):
    def test_sanity(self):
        assert(self.repo_dir)
        sack = base.TestSack(repo_dir=self.repo_dir)
        self.assertEqual(len(sack), 0)

    def test_load_rpm(self):
        sack = base.TestSack(repo_dir=self.repo_dir)
        sack.load_system_repo()
        self.assertEqual(len(sack), hawkey.test.EXPECT_SYSTEM_NSOLVABLES)

    def test_load_yum(self):
        sack = base.TestSack(repo_dir=self.repo_dir)
        sack.load_system_repo()
        sack.load_repo(build_cache=True)
        self.assertEqual(len(sack), hawkey.test.EXPECT_YUM_NSOLVABLES +
                         hawkey.test.EXPECT_SYSTEM_NSOLVABLES)

    def test_cache_dir(self):
        sack = base.TestSack(repo_dir=self.repo_dir)
        self.assertTrue(sack.cache_dir.startswith("/tmp/pyhawkey"))

class BasicTest(unittest.TestCase):
    def test_creation(self):
        hawkey.Sack(arch="noarch")
        hawkey.Sack(arch="x86_64")
        self.assertRaises(hawkey.ArchException, hawkey.Sack, arch="")
        self.assertRaises(hawkey.ValueException, hawkey.Sack, arch="play")

    def test_deepcopy(self):
        sack = hawkey.Sack()
        self.assertRaises(NotImplementedError, copy.deepcopy, sack)

    def test_creation_dir(self):
        sack = hawkey.Sack()
        self.assertFalse(os.access(sack.cache_dir, os.F_OK))
        sack = hawkey.Sack(make_cache_dir=True)
        self.assertTrue(os.access(sack.cache_dir, os.F_OK))
        self.assertRaises(IOError, hawkey.Sack, "", make_cache_dir=True)

    def test_failed_load(self):
        sack = hawkey.Sack(cachedir=base.cachedir)
        repo = hawkey.Repo("name")
        self.assertRaises(IOError, sack.load_repo, repo)
        sack = hawkey.Sack()

    def test_unicoded_cachedir(self):
        # does not raise UnicodeEncodeError
        hawkey.Sack(cachedir=u"unicod\xe9")

    def test_evr_cmp(self):
        sack = hawkey.Sack()
        self.assertEqual(sack.evr_cmp("3:3.10-4", "3:3.10-4"), 0)
        self.assertLess(sack.evr_cmp("3.10-4", "3.10-5"), 0)
        self.assertGreater(sack.evr_cmp("3.11-4", "3.10-5"), 0)
        self.assertGreater(sack.evr_cmp("1:3.10-4", "3.10-5"), 0)

class PackageWrappingTest(base.TestCase):
    class MyPackage(hawkey.Package):
        def __init__(self, initobject, myval):
            super(PackageWrappingTest.MyPackage, self).__init__(initobject)
            self.initobject = initobject
            self.myval = myval

    def test_wrapping(self):
        sack = base.TestSack(repo_dir=self.repo_dir,
                                    PackageClass=self.MyPackage,
                                    package_userdata=42)
        sack.load_system_repo()
        pkg = sack.create_package(2)
        # it is the correct type:
        self.assertIsInstance(pkg, self.MyPackage)
        self.assertIsInstance(pkg, hawkey.Package)
        # it received userdata:
        self.assertEqual(pkg.myval, 42)
        # the common attributes are working:
        self.assertEqual(pkg.name, "baby")
