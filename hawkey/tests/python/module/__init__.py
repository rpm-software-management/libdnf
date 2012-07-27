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

    def load_rpm_repo(self):
        path = os.path.join(self.repo_dir, "system.repo")
        _hawkey_test.load_repo(self, hawkey.SYSTEM_REPO_NAME, path, True)

    def load_yum_repo(self, **args):
        d = os.path.join(self.repo_dir, YUM_DIR_SUFFIX)
        repo = _hawkey_test.glob_for_repofiles(self, "messerk", d)
        super(TestSackMixin, self).load_yum_repo(repo, **args)

class TestSack(TestSackMixin, hawkey.Sack):
    def __init__(self, repo_dir, PackageClass=None, package_userdata=None):
        TestSackMixin.__init__(self, repo_dir)
        hawkey.Sack.__init__(self,
                             cachedir=cachedir,
                             arch=FIXED_ARCH,
                             pkgcls=PackageClass,
                             pkginitval=package_userdata)
