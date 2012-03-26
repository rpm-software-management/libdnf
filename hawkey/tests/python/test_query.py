import sys
import unittest

import hawkey
import hawkey.test

class Query(unittest.TestCase):
    repo_dir = None

    def setUp(self):
        self.sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        self.sack.load_rpm_repo()

    def test_sanity(self):
        q = hawkey.Query(self.sack)
        q.filter(name__eq="flying")
        self.assertEqual(q.count(), 1)

    def test_kwargs(self):
        q = hawkey.Query(self.sack)
        # test combining several criteria
        q.filter(name__glob="*enny*", summary__substr="eyes")
        self.assertEqual(q.count(), 1)
        # test shortcutting for equality comparison type
        q = hawkey.Query(self.sack)
        q.filter(name="flying")
        self.assertEqual(q.count(), 1)
        # test flags parsing
        q = hawkey.Query(self.sack).filter(name="FLYING")
        self.assertEqual(q.count(), 0)
        q = hawkey.Query(self.sack).filter(hawkey.ICASE, name="FLYING")
        self.assertEqual(q.count(), 1)

def suite(repo_dir):
    Query.repo_dir = repo_dir
    return unittest.TestLoader().loadTestsFromModule(sys.modules[__name__])
