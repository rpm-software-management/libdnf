import os

import hawkey
import _hawkey_test

EXPECT_SYSTEM_NSOLVABLES = _hawkey_test.EXPECT_SYSTEM_NSOLVABLES
EXPECT_MAIN_NSOLVABLES = _hawkey_test.EXPECT_MAIN_NSOLVABLES
EXPECT_UPDATES_NSOLVABLES = _hawkey_test.EXPECT_UPDATES_NSOLVABLES

class TestSack(hawkey.Sack):
    def __init__(self, repo_dir, PackageClass=None, package_userdata=None):
        super(TestSack, self).__init__(PackageClass, package_userdata)
        self.repo_dir = repo_dir

    def load_test_repo(self, name, fn):
        path = os.path.join(self.repo_dir, fn)
        _hawkey_test.load_repo(self, name, path, False)

    def load_rpm_repo(self):
        path = os.path.join(self.repo_dir, "system.repo")
        _hawkey_test.load_repo(self, hawkey.SYSTEM_REPO_NAME, path, True)

    def load_yum_repo(self):
        raise NotImplementedError("not implemented for unittests")
