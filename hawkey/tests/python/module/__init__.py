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

import os
import os.path
import tempfile

import hawkey
import _hawkey_test

EXPECT_SYSTEM_NSOLVABLES = _hawkey_test.EXPECT_SYSTEM_NSOLVABLES
EXPECT_MAIN_NSOLVABLES = _hawkey_test.EXPECT_MAIN_NSOLVABLES
EXPECT_UPDATES_NSOLVABLES = _hawkey_test.EXPECT_UPDATES_NSOLVABLES
EXPECT_YUM_NSOLVABLES = _hawkey_test.EXPECT_YUM_NSOLVABLES
FIXED_ARCH = _hawkey_test.FIXED_ARCH
UNITTEST_DIR = _hawkey_test.UNITTEST_DIR
YUM_DIR_SUFFIX = _hawkey_test.YUM_DIR_SUFFIX

glob_for_repofiles = _hawkey_test.glob_for_repofiles

cachedir = None
# inititialize the dir once and share it for all sacks within a single run
if cachedir is None:
    cachedir = tempfile.mkdtemp(dir=os.path.dirname(UNITTEST_DIR),
                                prefix='pyhawkey')

class TestSackMixin(object):
    def __init__(self, repo_dir):
        self.repo_dir = repo_dir

    def load_test_repo(self, name, fn):
        path = os.path.join(self.repo_dir, fn)
        _hawkey_test.load_repo(self, name, path, False)

    def load_system_repo(self, *args, **kwargs):
        path = os.path.join(self.repo_dir, "@System.repo")
        _hawkey_test.load_repo(self, hawkey.SYSTEM_REPO_NAME, path, True)

    def load_yum_repo(self, **args):
        d = os.path.join(self.repo_dir, YUM_DIR_SUFFIX)
        repo = glob_for_repofiles(self, "messerk", d)
        super(TestSackMixin, self).load_yum_repo(repo, **args)

class TestSack(TestSackMixin, hawkey.Sack):
    def __init__(self, repo_dir, PackageClass=None, package_userdata=None,
                 make_cache_dir=True):
        TestSackMixin.__init__(self, repo_dir)
        hawkey.Sack.__init__(self,
                             cachedir=cachedir,
                             arch=FIXED_ARCH,
                             pkgcls=PackageClass,
                             pkginitval=package_userdata,
                             make_cache_dir=make_cache_dir)
