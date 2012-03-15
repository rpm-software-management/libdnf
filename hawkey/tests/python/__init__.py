import os

import hawkey
import _hawkey_test

class TestSack(hawkey.Sack):
    def __init__(self, repo_dir, PackageClass=None, package_userdata=None):
        super(TestSack, self).__init__(PackageClass, package_userdata)
        self.repo_dir = repo_dir

    def load_rpm_repo(self):
        path = os.path.join(self.repo_dir, "system.repo")
        _hawkey_test.load_repo(self, hawkey.SYSTEM_REPO_NAME, path, True)
