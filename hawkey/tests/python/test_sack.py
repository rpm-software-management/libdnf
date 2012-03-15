import sys
import unittest

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
    def test_living(self):
        self.assertEqual(hawkey.test.TESTING, "some situations")

if __name__ == '__main__':
    unittest.main(argv=sys.argv[0:1])
