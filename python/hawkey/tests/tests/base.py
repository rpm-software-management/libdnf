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

from sys import version_info as python_version

import hawkey
import hawkey.test
import os
import tempfile
import unittest

cachedir = None
# inititialize the dir once and share it for all sacks within a single run
if cachedir is None:
    cachedir = tempfile.mkdtemp(dir=os.path.dirname(hawkey.test.UNITTEST_DIR),
                                prefix='pyhawkey')

class TestCase(unittest.TestCase):
    repo_dir = os.path.normpath(os.path.join(__file__, "../../../repos"))

    def assertLength(self, collection, length):
        return self.assertEqual(len(collection), length)

    def assertItemsEqual(self, item1, item2):
        # assertItemsEqual renamed in Python3
        if python_version.major < 3:
            super(TestCase, self).assertItemsEqual(item1, item2)
        else:
            super(TestCase, self).assertCountEqual(item1, item2)

skip = unittest.skip

class TestSack(hawkey.test.TestSackMixin, hawkey.Sack):
    def __init__(self, repo_dir, PackageClass=None, package_userdata=None,
                 make_cache_dir=True):
        hawkey.test.TestSackMixin.__init__(self, repo_dir)
        hawkey.Sack.__init__(self,
                             cachedir=cachedir,
                             arch=hawkey.test.FIXED_ARCH,
                             pkgcls=PackageClass,
                             pkginitval=package_userdata,
                             make_cache_dir=make_cache_dir)

    def load_repo(self, **kwargs):
        d = os.path.join(self.repo_dir, hawkey.test.YUM_DIR_SUFFIX)
        repo = hawkey.test.glob_for_repofiles(self, "messerk", d)
        super(TestSack, self).load_repo(repo, **kwargs)

def by_name(sack, name):
    return hawkey.Query(sack).filter(name=name)[0]

def by_name_repo(sack, name, repo):
    return hawkey.Query(sack).filter(name=name, reponame=repo)[0]
