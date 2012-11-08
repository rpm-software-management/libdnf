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

    def test_creation_empty_sack(self):
        s = hawkey.Sack()
        q = hawkey.Query(s)

    def test_exception(self):
        q = hawkey.Query(self.sack)
        self.assertRaises(hawkey.ValueException, q.filter, flying__eq="name")
        self.assertRaises(hawkey.ValueException, q.filter, flying="name")

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
        self.assertRaises(hawkey.ValueException, q.filter,
                          name="flying", upgrades="maracas")

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

    def test_in_set(self):
        q = hawkey.Query(self.sack)
        q.filterm(name__substr=set(["ool", "enny-li"]))
        self.assertEqual(q.count(), 2)

    def test_iteration(self):
        q = hawkey.Query(self.sack)
        q.filterm(name__substr=["penny"])
        self.assertEqual(q.count(), 2)
        self.assertNotEqual(q[0], q[1])

    def test_clone(self):
        q = hawkey.Query(self.sack)
        q.filterm(name__substr=["penny"])
        q_clone = hawkey.Query(query=q)
        del q

        self.assertEqual(q_clone.count(), 2)
        self.assertNotEqual(q_clone[0], q_clone[1])

    def test_immutability(self):
        q = hawkey.Query(self.sack).filter(name="jay")
        q2 = q.filter(evr="5.0-0")
        self.assertEqual(q.count(), 2)
        self.assertEqual(q2.count(), 1)

    def test_copy_lazyness(self):
        q = hawkey.Query(self.sack).filter(name="jay")
        self.assertIsNone(q.result)
        q2 = q.filter(evr="5.0-0")
        self.assertIsNone(q.result)

    def test_empty(self):
        q = hawkey.Query(self.sack).filter(empty=True)
        self.assertLength(q, 0)
        q = hawkey.Query(self.sack)
        self.assertRaises(hawkey.ValueException, q.filter, empty=False)

    def test_epoch(self):
        q = hawkey.Query(self.sack).filter(epoch__gt=4)
        self.assertEqual(len(q), 1)
        self.assertEqual(q[0].epoch, 6)

    def test_version(self):
        q = hawkey.Query(self.sack).filter(version__gte="5.0")
        self.assertEqual(len(q), 3)

    def test_package_in(self):
        pkgs = list(hawkey.Query(self.sack).filter(name=["flying", "penny"]))
        q = hawkey.Query(self.sack).filter(pkg=pkgs)
        self.assertEqual(len(q), 2)
        q2 = q.filter(version__gt="3")
        self.assertEqual(len(q2), 1)

    def test_repeated(self):
        q = hawkey.Query(self.sack).filter(name="jay")
        q.run()
        q.filterm(latest=True)
        self.assertEqual(len(q), 1)

    def test_reldep(self):
        flying = base.by_name(self.sack, "flying")
        requires = flying.requires
        q = hawkey.Query(self.sack).filter(provides=requires[0])
        self.assertEqual(len(q), 1)
        self.assertEqual(str(q[0]), "penny-lib-4-1.x86_64")

        self.assertRaises(hawkey.QueryException, q.filter,
                          provides__gt=requires[0])

    def test_reldep_list(self):
        self.sack.load_test_repo("updates", "updates.repo")
        fool = base.by_name_repo(self.sack, "fool", "updates")
        q = hawkey.Query(self.sack).filter(provides=fool.obsoletes)
        self.assertEqual(str(q.run()[0]), "penny-4-1.noarch")

class QueryUpdates(base.TestCase):
    def setUp(self):
        self.sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()
        self.sack.load_test_repo("updates", "updates.repo")

    def test_updates(self):
        q = hawkey.Query(self.sack)
        q.filterm(name="flying", upgrades=1)
        self.assertEqual(q.count(), 1)

    def test_obsoletes(self):
        q = hawkey.Query(self.sack).filter(name="penny")
        o = hawkey.Query(self.sack)
        self.assertRaises(hawkey.QueryException, o.filter, obsoletes__gt=q)
        self.assertRaises(hawkey.ValueException, o.filter, requires=q)

        o = hawkey.Query(self.sack).filter(obsoletes=q)
        self.assertLength(o, 1)
        self.assertEqual(str(o[0]), 'fool-1-5.noarch')
