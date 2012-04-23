import sys
import unittest

import base
import hawkey
import hawkey.test

class Package(base.TestCase):
    def setUp(self):
        self.sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        self.sack.load_rpm_repo()
        q = hawkey.Query(self.sack)
        q.filter(name__eq="flying")
        self.pkg = list(q)[0]

    def test_repr(self):
        self.assertRegexpMatches(repr(self.pkg),
                                 "<_hawkey.Package object, id: \d+>")

    def test_str(self):
        self.assertEqual(str(self.pkg),
                         "flying-2-9.noarch")

def suite(repo_dir):
    return unittest.TestLoader().loadTestsFromModule(sys.modules[__name__])
