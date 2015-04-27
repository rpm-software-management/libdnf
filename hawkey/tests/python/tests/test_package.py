#
# Copyright (C) 2012-2014 Red Hat, Inc.
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
from sys import version_info as python_version

import hawkey

class AdvisoriesTest(base.TestCase):
    """Tests related to packages and updateinfo metadata."""

    def setUp(self):
        """Prepare the test fixture."""
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_repo(load_updateinfo=True)

    def test(self):
        pkg = hawkey.Query(self.sack).filter(name='tour')[0]
        advisories = pkg.get_advisories(hawkey.GT)
        self.assertEqual(len(advisories), 1)
        self.assertEqual(advisories[0].id, u'FEDORA-2008-9969')

    def test_noadvisory(self):
        pkg = hawkey.Query(self.sack).filter(name='mystery-devel')[0]
        advisories = pkg.get_advisories(hawkey.GT | hawkey.EQ)
        self.assertEqual(len(advisories), 0)

class PackageTest(base.TestCase):
    def setUp(self):
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()
        q = hawkey.Query(self.sack).filter(name__eq="flying")
        self.pkg = list(q)[0]

    def test_instance(self):
        self.assertTrue(isinstance(self.pkg, hawkey.Package))

    def test_repr(self):
        regexp = "<hawkey.Package object id \d+, flying-2-9\.noarch, @System>"
        self.assertRegexpMatches(repr(self.pkg), regexp)

    def test_str(self):
        self.assertEqual(str(self.pkg),
                         "flying-2-9.noarch")

    def test_hashability(self):
        """ Test that hawkey.Package objects are hashable.

            So they can be keys in a dict.
        """
        d = {}
        d[self.pkg] = True
        pkg2 = hawkey.Query(self.sack).filter(name__eq="fool").run()[0]
        d[pkg2] = 1
        self.assertEqual(len(d), 2)

        # if we get the same package twice it is a different object (hawkey does
        # not pool Packages at this time). It however compares equal.
        pkg3 = hawkey.Query(self.sack).filter(name__eq="fool").run()[0]
        self.assertIsNot(pkg2, pkg3)
        self.assertEquals(pkg2, pkg3)
        d[pkg3] += 1
        self.assertEqual(d[pkg2], 2)
        self.assertEqual(len(d), 2)

    def test_conflicts(self):
        pkg = base.by_name(self.sack, 'dog')
        self.assertItemsEqual(list(map(str, pkg.conflicts)), ('custard = 1.1',))

    def test_files(self):
        pkg = base.by_name(self.sack, 'fool')
        self.assertSequenceEqual(pkg.files, ('/no/answers',))

class PackageCmpTest(base.TestCase):
    def setUp(self):
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()
        self.sack.load_test_repo("main", "main.repo")
        self.pkg1 = base.by_name_repo(self.sack, "fool", hawkey.SYSTEM_REPO_NAME)

    def test_cmp(self):
        pkg2 = base.by_name_repo(self.sack, "fool", "main")
        # if nevra matches the packages are equal:
        self.assertEqual(self.pkg1, pkg2)

        # if the name doesn't match they are not equal:
        pkg2 = base.by_name_repo(self.sack, "hello", "main")
        self.assertNotEqual(self.pkg1, pkg2)

        # if nevr matches, but not arch, they are not equal:
        pkg1 = hawkey.split_nevra("semolina-2-0.x86_64").to_query(self.sack)[0]
        pkg2 = hawkey.split_nevra("semolina-2-0.i686").to_query(self.sack)[0]
        self.assertNotEqual(pkg1, pkg2)
        # however evr_cmp compares only the evr part:
        self.assertEqual(pkg1.evr_cmp(pkg2), 0)

        pkg1 = hawkey.split_nevra("jay-6.0-0.x86_64").to_query(self.sack)[0]
        pkg2 = hawkey.split_nevra("jay-5.0-0.x86_64").to_query(self.sack)[0]
        self.assertLess(pkg2, pkg1)
        self.assertGreater(pkg1.evr_cmp(pkg2), 0)

    def test_cmp_fail(self):
        # should not throw TypeError
        self.assertNotEqual(self.pkg1, "hawkey-package")
        self.assertNotEqual(self.pkg1, self.sack)

        if python_version.major > 3:
            self.assertRaises(TypeError, lambda: self.pkg1 <= "hawkey-package")

    def test_cmp_lt(self):
        greater_package = base.by_name_repo(self.sack, "hello", "main")

        self.assertLess(self.pkg1, greater_package)

    def test_cmp_gt(self):
        less_package = base.by_name_repo(self.sack, "baby", "main")

        self.assertGreater(self.pkg1, less_package)

class ChecksumsTest(base.TestCase):
    def setUp(self):
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_repo()
        self.pkg = hawkey.Query(self.sack).filter(name="mystery-devel")[0]

    def test_checksum(self):
        (chksum_type, chksum) = self.pkg.chksum
        self.assertEqual(len(chksum), 32)
        if python_version.major < 3:
            self.assertEqual(chksum[0], b'\x2e')
            self.assertEqual(chksum[31], b'\xf5')
        else:
            self.assertEqual(chksum[0], 0x2e)
            self.assertEqual(chksum[31], 0xf5)
        self.assertEqual(chksum_type, hawkey.CHKSUM_SHA256)

    def test_checksum_fail(self):
        sack = base.TestSack(repo_dir=self.repo_dir)
        sack.load_system_repo()
        pkg = base.by_name(sack, "fool")
        self.assertRaises(AttributeError, lambda: pkg.chksum)
        self.assertRaises(AttributeError, lambda: pkg.hdr_chksum)

    def test_sizes(self):
        pkg = base.by_name(self.sack, "mystery-devel")
        self.assertEqual(pkg.installsize, 59)
        self.assertEqual(pkg.downloadsize, 2329)

class FullPropertiesTest(base.TestCase):
    """Tests properties as seen in a real primary.xml."""

    def setUp(self):
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_repo()
        self.pkg = hawkey.Query(self.sack).filter(name="mystery-devel")[0]

    def test_baseurl(self):
        self.assertEqual(self.pkg.baseurl, 'disagree')

    def test_hdr_end(self):
        self.assertEqual(self.pkg.hdr_end, 2081)
