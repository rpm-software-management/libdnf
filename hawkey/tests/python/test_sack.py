import os
import sys
import unittest

REPO_DIR=""
if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.stderr.write("synopsis: %s <module directory> <repo directory>\n" %
                         (sys.argv[0]))
        sys.exit(1)
    sys.path.insert(0, sys.argv[1])
    REPO_DIR = sys.argv[2]

import hawkey
import hawkey.test

class Sanity(unittest.TestCase):
    def test_sanity(self):
        assert(REPO_DIR)
        sack = hawkey.test.TestSack(repo_dir=REPO_DIR)
        self.assertEqual(sack.nsolvables, 2)

    def test_load_rpm(self):
        sack = hawkey.test.TestSack(repo_dir=REPO_DIR)
        sack.load_rpm_repo()
        self.assertEqual(sack.nsolvables, 5)

if __name__ == '__main__':
    unittest.main(argv=sys.argv[0:1])
