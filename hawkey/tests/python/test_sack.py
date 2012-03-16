import unittest

import hawkey
import hawkey.test

class Sanity(unittest.TestCase):
    repo_dir = None

    def test_sanity(self):
        assert(self.repo_dir)
        sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        self.assertEqual(sack.nsolvables, 2)

    def test_load_rpm(self):
        sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        sack.load_rpm_repo()
        self.assertEqual(sack.nsolvables, 5)

def suite(repo_dir):
    Sanity.repo_dir = repo_dir
    return unittest.TestLoader().loadTestsFromTestCase(Sanity)
