import sys
import unittest

import base
import hawkey
import hawkey.test

class Query(base.TestCase):
    def setUp(self):
        self.sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()

    def test_sanity(self):
        q = hawkey.Query(self.sack)
        q.filterm(name__eq="flying")
        self.assertEqual(q.count(), 1)

    def test_unicode(self):
        q = hawkey.Query(self.sack)
        q.filterm(name__eq=u"flying")
        self.assertEqual(q.count(), 1)

        q = hawkey.Query(self.sack)
        q.filterm(name__eq=[u"flying", "penny"])
        self.assertEqual(q.count(), 2)

    def test_count(self):
        q = hawkey.Query(self.sack).filter(name=["flying", "penny"])

        self.assertIsNone(q.result)
        self.assertEqual(len(q), 2)
        self.assertIsNotNone(q.result)
        self.assertEqual(len(q), q.count())
        self.assertTrue(q)

        q = hawkey.Query(self.sack).filter(name="naturalE")
        self.assertFalse(q)
        self.assertIsNotNone(q.result)

    def test_kwargs_check(self):
        q = hawkey.Query(self.sack)
        self.assertRaises(ValueError, q.filter, name="flying", upgrades="maracas")

    def test_kwargs(self):
        q = hawkey.Query(self.sack)
        # test combining several criteria
        q.filterm(name__glob="*enny*", summary__substr="eyes")
        self.assertEqual(q.count(), 1)
        # test shortcutting for equality comparison type
        q = hawkey.Query(self.sack)
        q.filterm(name="flying")
        self.assertEqual(q.count(), 1)
        # test flags parsing
        q = hawkey.Query(self.sack).filter(name="FLYING")
        self.assertEqual(q.count(), 0)
        q = hawkey.Query(self.sack).filter(hawkey.ICASE, name="FLYING")
        self.assertEqual(q.count(), 1)

    def test_in(self):
        q = hawkey.Query(self.sack)
        q.filterm(name__substr=["ool", "enny-li"])
        self.assertEqual(q.count(), 2)

    def test_iteration(self):
        q = hawkey.Query(self.sack)
        q.filterm(name__substr=["penny"])
        self.assertEqual(q.count(), 2)
        self.assertNotEqual(q[0], q[1])

    def test_clone(self):
        q = hawkey.Query(self.sack)
        q.filterm(name__substr=["penny"])
        q_clone = hawkey.Query(q)
        del q

        self.assertEqual(q_clone.count(), 2)
        self.assertNotEqual(q_clone[0], q_clone[1])

    def test_immutability(self):
        q = hawkey.Query(self.sack).filter(name="jay")
        q2 = q.filter(evr="5.0-0")
        self.assertEqual(q.count(), 2)
        self.assertEqual(q2.count(), 1)

class QueryUpdates(base.TestCase):
    def setUp(self):
        self.sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()
        self.sack.load_test_repo("updates", "updates.repo")

    def test_updates(self):
        q = hawkey.Query(self.sack)
        q.filterm(name="flying", upgrades=1)
        self.assertEqual(q.count(), 1)
