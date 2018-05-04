#
# Copyright (C) 2012-2014 Red Hat, Inc.
#
# Licensed under the GNU Lesser General Public License Version 2.1
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#

from __future__ import absolute_import
from . import base

import hawkey
import sys
import unittest

class TestQuery(base.TestCase):
    def setUp(self):
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()

    def test_sanity(self):
        q = hawkey.Query(self.sack)
        q.filterm(name__eq="flying")
        self.assertEqual(q.count(), 1)

    def test_creation_empty_sack(self):
        s = hawkey.Sack(make_cache_dir=True)
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
        self.assertEqual(len(q), 2)
        self.assertEqual(len(q), q.count())
        self.assertTrue(q)

        q = hawkey.Query(self.sack).filter(name="naturalE")
        self.assertFalse(q)
        self.assertEqual(len(q.run()), 0)

    def test_kwargs_check(self):
        q = hawkey.Query(self.sack)
        self.assertRaises(hawkey.ValueException, q.filter, name="flying", upgrades="maracas")

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

    def test_clone_with_evaluation(self):
        q = hawkey.Query(self.sack)
        q.filterm(name__substr="penny")
        q.run()
        q_clone = hawkey.Query(query=q)
        del q
        self.assertTrue(q_clone.evaluated)
        self.assertLength(q_clone.run(), 2)

    def test_immutability(self):
        q = hawkey.Query(self.sack).filter(name="jay")
        q2 = q.filter(evr="5.0-0")
        self.assertEqual(q.count(), 2)
        self.assertEqual(q2.count(), 1)

    def test_copy_lazyness(self):
        q = hawkey.Query(self.sack).filter(name="jay")
        self.assertLength(q.run(), 2)
        q2 = q.filter(evr="5.0-0")
        self.assertLength(q2.run(), 1)

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
        q = hawkey.Query(self.sack).filter(version__glob="1.2*")
        self.assertLength(q, 2)

    def test_package_in(self):
        pkgs = list(hawkey.Query(self.sack).filter(name=["flying", "penny"]))
        q = hawkey.Query(self.sack).filter(pkg=pkgs)
        self.assertEqual(len(q), 2)
        q2 = q.filter(version__gt="3")
        self.assertEqual(len(q2), 1)

    def test_nevra_match(self):
        query = hawkey.Query(self.sack).filter(nevra__glob="*lib*64")
        self.assertEqual(len(query), 1)
        self.assertEqual(str(query[0]), "penny-lib-4-1.x86_64")

    def test_nevra_strict_match(self):
        query = hawkey.Query(self.sack).filter(nevra_strict="penny-lib-4-1.x86_64")
        self.assertEqual(len(query), 1)
        self.assertEqual(str(query[0]), "penny-lib-4-1.x86_64")

    def test_nevra_strict_list_match(self):
        query = hawkey.Query(self.sack).filter(nevra_strict=["penny-lib-4-1.x86_64"])
        self.assertEqual(len(query), 1)
        self.assertEqual(str(query[0]), "penny-lib-4-1.x86_64")

    def test_nevra_strict_epoch0_match(self):
        query = hawkey.Query(self.sack).filter(nevra_strict="penny-lib-0:4-1.x86_64")
        self.assertEqual(len(query), 1)
        self.assertEqual(str(query[0]), "penny-lib-4-1.x86_64")

    def test_repeated(self):
        q = hawkey.Query(self.sack).filter(name="jay")
        q.filterm(latest_per_arch=True)
        self.assertEqual(len(q), 1)

    def test_latest(self):
        q = hawkey.Query(self.sack).filter(name="pilchard")
        q.filterm(latest_per_arch=True)
        self.assertEqual(len(q), 2)
        q.filterm(latest=True)
        self.assertEqual(len(q), 2)

    def test_reldep(self):
        flying = base.by_name(self.sack, "flying")
        requires = flying.requires
        q = hawkey.Query(self.sack).filter(provides=requires[0])
        self.assertEqual(len(q), 1)
        self.assertEqual(str(q[0]), "penny-lib-4-1.x86_64")
        self.assertRaises(hawkey.QueryException, q.filter, provides__gt=requires[0])

        q = hawkey.Query(self.sack).filter(provides="thisisnotgoingtoexist")
        self.assertLength(q.run(), 0)

    def test_reldep_list(self):
        self.sack.load_test_repo("updates", "updates.repo")
        fool = base.by_name_repo(self.sack, "fool", "updates")
        q = hawkey.Query(self.sack).filter(provides=fool.obsoletes)
        self.assertEqual(str(q.run()[0]), "penny-4-1.noarch")

    def test_disabled_repo(self):
        self.sack.disable_repo(hawkey.SYSTEM_REPO_NAME)
        q = hawkey.Query(self.sack).filter(name="jay")
        self.assertLength(q.run(), 0)
        self.sack.enable_repo(hawkey.SYSTEM_REPO_NAME)
        q = hawkey.Query(self.sack).filter(name="jay")
        self.assertLength(q.run(), 2)

    def test_multiple_flags(self):
        q = hawkey.Query(self.sack).filter(name__glob__not=["p*", "j*"])
        self.assertItemsEqual(list(map(lambda p: p.name, q.run())),
                              ["baby", "dog", "flying", "fool", "gun", "tour"])

    def test_apply(self):
        q = hawkey.Query(self.sack).filter(name__glob__not="p*").apply()
        res = q.filter(name__glob__not="j*").run()
        self.assertItemsEqual(list(map(lambda p: p.name, res)),
                              ["baby", "dog", "flying", "fool", "gun", "tour"])

    def test_provides_glob_should_work(self):
        q1 = hawkey.Query(self.sack).filter(provides__glob="penny*")
        self.assertLength(q1, 2)
        q2 = hawkey.Query(self.sack).filter(provides__glob="*")
        q3 = hawkey.Query(self.sack).filter()
        self.assertEqual(len(q2), len(q3))
        q4 = hawkey.Query(self.sack).filter(provides__glob="P-l*b >= 3")
        self.assertLength(q4, 1)

    def test_provides_glob_should_not_work(self):
        q = hawkey.Query(self.sack).filter(provides="*")
        self.assertLength(q, 0)
        q = hawkey.Query(self.sack).filter(provides__glob="nomatch*")
        self.assertLength(q, 0)

    def test_provides_list_of_strings(self):
        q1 = hawkey.Query(self.sack).filter(provides=["penny", "P-lib"])
        q2 = hawkey.Query(self.sack).filter(provides__glob=["penny", "P-lib"])
        self.assertEqual(len(q1), len(q2))
        q3 = hawkey.Query(self.sack).filter(provides__glob=["penny*", "P-lib*"])
        self.assertEqual(len(q1), len(q3))
        q4 = hawkey.Query(self.sack).filter(provides__glob=["nomatch*", "P-lib*"])
        q5 = hawkey.Query(self.sack).filter(provides__glob="P-lib*")
        self.assertEqual(len(q4), len(q5))
        q6 = hawkey.Query(self.sack).filter(provides=["nomatch", "P-lib"])
        q7 = hawkey.Query(self.sack).filter(provides="P-lib")
        self.assertEqual(len(q6), len(q7))
        q8 = hawkey.Query(self.sack).filter(provides=[])
        self.assertLength(q8, 0)
        q9 = hawkey.Query(self.sack).filter(provides__glob=[])
        self.assertLength(q9, 0)

    def test_requires_list_of_strings(self):
        q1 = hawkey.Query(self.sack).filter(requires=["penny", "P-lib"])
        q2 = hawkey.Query(self.sack).filter(requires__glob=["penny", "P-lib"])
        self.assertEqual(len(q1), len(q2))
        q3 = hawkey.Query(self.sack).filter(requires__glob=["penny*", "P-lib*"])
        self.assertEqual(len(q1), len(q3))
        q4 = hawkey.Query(self.sack).filter(requires__glob=["nomatch*", "P-lib*"])
        q5 = hawkey.Query(self.sack).filter(requires__glob="P-lib*")
        self.assertEqual(len(q4), len(q5))
        q6 = hawkey.Query(self.sack).filter(requires=["nomatch", "P-lib"])
        q7 = hawkey.Query(self.sack).filter(requires="P-lib")
        self.assertEqual(len(q6), len(q7))
        q1 = hawkey.Query(self.sack).filter(requires__glob=["*bin/away"])
        self.assertLength(q1, 1)


class TestQueryAllRepos(base.TestCase):
    def setUp(self):
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()
        self.sack.load_test_repo("main", "main.repo")
        self.sack.load_test_repo("updates", "updates.repo")

    def test_requires(self):
        reldep = hawkey.Reldep(self.sack, "semolina = 2")
        q = hawkey.Query(self.sack).filter(requires=reldep)
        self.assertItemsEqual(list(map(str, q.run())),
                              ["walrus-2-5.noarch", "walrus-2-6.noarch"])

        reldep = hawkey.Reldep(self.sack, "semolina > 1.0")
        q = hawkey.Query(self.sack).filter(requires=reldep)
        self.assertItemsEqual(list(map(str, q.run())),
                              ["walrus-2-5.noarch", "walrus-2-6.noarch"])

    def test_obsoletes(self):
        reldep = hawkey.Reldep(self.sack, "penny < 4-0")
        q = hawkey.Query(self.sack).filter(obsoletes=reldep)
        self.assertItemsEqual(list(map(str, q.run())), ["fool-1-5.noarch"])

    def test_downgradable(self):
        query = hawkey.Query(self.sack).filter(downgradable=True)
        self.assertEqual({str(pkg) for pkg in query},
                         {"baby-6:5.0-11.x86_64", "jay-5.0-0.x86_64"})

    def test_rco_glob(self):
        q1 = hawkey.Query(self.sack).filter(requires__glob="*")
        self.assertLength(q1, 9)
        q2 = hawkey.Query(self.sack).filter(requires="*")
        self.assertLength(q2, 0)
        q3 = hawkey.Query(self.sack).filter(conflicts__glob="cu*")
        self.assertLength(q3, 3)
        q1 = hawkey.Query(self.sack).filter(obsoletes__glob="*")
        self.assertLength(q1, 1)
        q1 = hawkey.Query(self.sack).filter(recommends__glob="*")
        self.assertLength(q1, 1)
        q1 = hawkey.Query(self.sack).filter(enhances__glob="*")
        self.assertLength(q1, 1)
        q1 = hawkey.Query(self.sack).filter(suggests__glob="*")
        self.assertLength(q1, 1)
        q1 = hawkey.Query(self.sack).filter(supplements__glob="*")
        self.assertLength(q1, 1)
        q1 = hawkey.Query(self.sack).filter(requires__glob="*bin/away")
        self.assertLength(q1, 1)


class TestQueryUpdates(base.TestCase):
    def setUp(self):
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()
        self.sack.load_test_repo("updates", "updates.repo")

    def test_upgradable(self):
        query = hawkey.Query(self.sack).filter(upgradable=True)
        self.assertEqual({str(pkg) for pkg in query},
                         {"dog-1-1.x86_64", "flying-2-9.noarch",
                          "fool-1-3.noarch", "pilchard-1.2.3-1.i686",
                          "pilchard-1.2.3-1.x86_64"})

    def test_updates_noarch(self):
        q = hawkey.Query(self.sack)
        q.filterm(name="flying", upgrades=1)
        self.assertEqual(q.count(), 3)

    def test_updates_arch(self):
        q = hawkey.Query(self.sack)
        pilchard = q.filter(name="dog", upgrades=True)
        self.assertItemsEqual(list(map(str, pilchard.run())), ["dog-1-2.x86_64"])

    def test_glob_arch(self):
        q = hawkey.Query(self.sack)
        pilchard = q.filter(name="pilchard", version="1.2.4", release="1",
                            arch__glob="*6*")
        res = list(map(str, pilchard.run()))
        self.assertItemsEqual(res, ["pilchard-1.2.4-1.x86_64",
                              "pilchard-1.2.4-1.i686"])

    def test_obsoletes(self):
        q = hawkey.Query(self.sack).filter(name="penny")
        o = hawkey.Query(self.sack)
        self.assertRaises(hawkey.QueryException, o.filter, obsoletes__gt=q)
        self.assertRaises(hawkey.ValueException, o.filter, requires=q)

        o = hawkey.Query(self.sack).filter(obsoletes=q)
        self.assertLength(o, 1)
        self.assertEqual(str(o[0]), "fool-1-5.noarch")

    def test_subquery_evaluated(self):
        q = hawkey.Query(self.sack).filter(name="penny")
        self.assertFalse(q.evaluated)
        self.assertLength(q.run(), 1)
        self.assertTrue(q.evaluated)
        o = hawkey.Query(self.sack).filter(obsoletes=q)
        self.assertTrue(q.evaluated)
        self.assertLength(q.run(), 1)
        self.assertLength(o, 1)

class TestOddArch(base.TestCase):
    def setUp(self):
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_test_repo("ppc", "ppc.repo")

    def test_latest(self):
        q = hawkey.Query(self.sack).filter(latest=True)
        self.assertEqual(len(q), 1)

        q = hawkey.Query(self.sack).filter(latest_per_arch=True)
        self.assertEqual(len(q), 1)


class TestAllArch(base.TestCase):

    def setUp(self):
        self.sack1 = base.TestSack(repo_dir=self.repo_dir, arch="ppc64")
        self.sack2 = base.TestSack(repo_dir=self.repo_dir, arch="x86_64")
        self.sack3 = base.TestSack(repo_dir=self.repo_dir, all_arch=True)
        self.sack1.load_test_repo("test_ppc", "ppc.repo")
        self.sack2.load_test_repo("test_ppc", "ppc.repo")
        self.sack3.load_test_repo("test_ppc", "ppc.repo")

    def test_provides_all_arch_query(self):
        ppc_pkgs = hawkey.Query(self.sack1)
        self.assertGreater(len(ppc_pkgs), 0)
        pkg1 = ppc_pkgs[0]

        query_ppc = hawkey.Query(self.sack1).filter(provides=pkg1.provides[0])
        query_x86 = hawkey.Query(self.sack2).filter(provides=pkg1.provides[0])
        query_all = hawkey.Query(self.sack3).filter(provides=pkg1.provides[0])

        self.assertEqual(len(query_ppc), 1)
        self.assertEqual(len(query_x86), 0)
        self.assertEqual(len(query_all), 1)


class TestQuerySubclass(base.TestCase):
    class CustomQuery(hawkey.Query):
        pass

    def test_instance(self):
        sack = base.TestSack(repo_dir=self.repo_dir)
        q = self.CustomQuery(sack)
        self.assertIsInstance(q, self.CustomQuery)
        q = q.filter(name="pepper")
        self.assertIsInstance(q, self.CustomQuery)

class TestQueryFilterAdvisory(base.TestCase):
    """Tests related to packages and updateinfo metadata."""

    def setUp(self):
        """Prepare the test fixture."""
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_repo(load_updateinfo=True)
        self.expected_pkgs = hawkey.Query(self.sack).filter(nevra=[
                                                "mystery-devel-19.67-1.noarch",
                                                "tour-4-6.noarch"]
                                                ).run()

    def test_advisory(self):
        pkgs = hawkey.Query(self.sack).filter(advisory="BEATLES-1967-1127").run()
        self.assertEqual(pkgs, self.expected_pkgs)

    def test_advisory_type(self):
        pkgs = hawkey.Query(self.sack).filter(advisory_type="security").run()
        self.assertEqual(pkgs, self.expected_pkgs)

    def test_advisory_bug(self):
        pkgs = hawkey.Query(self.sack).filter(advisory_bug="0#paul").run()
        self.assertEqual(pkgs, self.expected_pkgs)

    def test_advisory_cve(self):
        pkgs = hawkey.Query(self.sack).filter(advisory_cve="CVE-1967-BEATLES").run()
        self.assertEqual(pkgs, self.expected_pkgs)

class TestQuerySetOperations(base.TestCase):
    def setUp(self):
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()
        self.q = hawkey.Query(self.sack)
        self.q1 = self.q.filter(version="4")
        self.q2 = self.q.filter(name__glob="p*")

    def test_difference(self):
        qi = self.q1.difference(self.q2)
        difference = [p for p in self.q1 if p not in self.q2]
        self.assertEqual(qi.run(), difference)

    def test_intersection(self):
        qi = self.q1.intersection(self.q2)
        intersection = self.q.filter(version="4", name__glob="p*")
        self.assertEqual(qi.run(), intersection.run())

    def test_union(self):
        qu = self.q1.union(self.q2)
        union = set(self.q1.run() + self.q2.run())
        self.assertEqual(set(qu), union)

    def test_zzz_queries_not_modified(self):
        self.assertEqual(len(self.q1), 5)
        self.assertEqual(len(self.q2), 5)
