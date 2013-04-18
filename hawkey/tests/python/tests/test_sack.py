import os
import sys
import unittest

import base
import hawkey
import hawkey.test

class TestSackTest(base.TestCase):
    def test_sanity(self):
        assert(self.repo_dir)
        sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        self.assertEqual(len(sack), 0)

    def test_load_rpm(self):
        sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        sack.load_system_repo()
        self.assertEqual(len(sack), hawkey.test.EXPECT_SYSTEM_NSOLVABLES)

    def test_load_yum(self):
        sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        sack.load_system_repo()
        sack.load_yum_repo(build_cache=True)
        self.assertEqual(len(sack), hawkey.test.EXPECT_YUM_NSOLVABLES +
                         hawkey.test.EXPECT_SYSTEM_NSOLVABLES)

    def test_cache_path(self):
        sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        self.assertTrue(sack.cache_path.startswith("/tmp/pyhawkey"))

class BasicTest(unittest.TestCase):
    def test_creation(self):
        hawkey.Sack(arch="noarch")
        hawkey.Sack(arch="x86_64")
        self.assertRaises(hawkey.ArchException, hawkey.Sack, arch="")
        self.assertRaises(hawkey.ValueException, hawkey.Sack, arch="play")

    def test_creation_dir(self):
        sack = hawkey.Sack()
        self.assertFalse(os.access(sack.cache_path, os.F_OK))
        sack = hawkey.Sack(make_cache_dir=True)
        self.assertTrue(os.access(sack.cache_path, os.F_OK))
        self.assertRaises(IOError, hawkey.Sack, "", make_cache_dir=True)

    def test_failed_load(self):
        sack = hawkey.Sack(cachedir=hawkey.test.cachedir)
        repo = hawkey.Repo("name")
        self.assertRaises(IOError, sack.load_yum_repo, repo)
        sack = hawkey.Sack()

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
        sack = hawkey.test.TestSack(repo_dir=self.repo_dir,
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
        self.assertEqual(pkg.name, "fool")
