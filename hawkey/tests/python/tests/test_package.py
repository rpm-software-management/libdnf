import base
import hawkey
import hawkey.test

class PackageTest(base.TestCase):
    def setUp(self):
        self.sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()
        q = hawkey.Query(self.sack).filter(name__eq="flying")
        self.pkg = list(q)[0]

    def test_instance(self):
        self.assertTrue(isinstance(self.pkg, hawkey.Package))

    def test_repr(self):
        regexp = "<hawkey.Package object id \d+, flying-2-9\.noarch, @System>"
        self.assertRegexpMatches(repr(self.pkg), regexp)

    def test_str(self):
        self.assertEqual(str(self.pkg),
                         "flying-2-9.noarch")

    def test_hashability(self):
        """ Test that hawkey.Package objects are hashable.

            So they can be keys in a dict.
        """
        d = {}
        d[self.pkg] = True
        pkg2 = hawkey.Query(self.sack).filter(name__eq="fool").run()[0]
        d[pkg2] = 1
        self.assertEqual(len(d), 2)

        # if we get the same package twice it is a different object (hawkey does
        # not pool Packages at this time). It however compares equal.
        pkg3 = hawkey.Query(self.sack).filter(name__eq="fool").run()[0]
        self.assertIsNot(pkg2, pkg3)
        self.assertEquals(pkg2, pkg3)
        d[pkg3] += 1
        self.assertEqual(d[pkg2], 2)
        self.assertEqual(len(d), 2)

    def test_conflicts(self):
        pkg = base.by_name(self.sack, 'dog')
        self.assertItemsEqual(map(str, pkg.conflicts), ('custard = 1.1',))

class PackageCmpTest(base.TestCase):
    def setUp(self):
        self.sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()
        self.sack.load_test_repo("main", "main.repo")

    def test_cmp(self):
        pkg1 = base.by_name_repo(self.sack, "fool", hawkey.SYSTEM_REPO_NAME)
        pkg2 = base.by_name_repo(self.sack, "fool", "main")
        # if nevra matches the packages are equal:
        self.assertEqual(pkg1, pkg2)

        # if the name doesn't match they are not equal:
        pkg2 = base.by_name_repo(self.sack, "hello", "main")
        self.assertNotEqual(pkg1, pkg2)

        # if nevr matches, but not arch, they are not equal:
        pkg1 = hawkey.split_nevra("semolina-2-0.x86_64").to_query(self.sack)[0]
        pkg2 = hawkey.split_nevra("semolina-2-0.i686").to_query(self.sack)[0]
        self.assertNotEqual(pkg1, pkg2)
        # however evr_cmp compares only the evr part:
        self.assertEqual(pkg1.evr_cmp(pkg2), 0)

        pkg1 = hawkey.split_nevra("jay-6.0-0.x86_64").to_query(self.sack)[0]
        pkg2 = hawkey.split_nevra("jay-5.0-0.x86_64").to_query(self.sack)[0]
        self.assertLess(pkg2, pkg1)
        self.assertGreater(pkg1.evr_cmp(pkg2), 0)

class ChecksumsTest(base.TestCase):
    def setUp(self):
        self.sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        self.sack.load_yum_repo()
        self.pkg = hawkey.Query(self.sack).filter(name="mystery-devel")[0]

    def test_checksum(self):
        (chksum_type, chksum) = self.pkg.chksum
        self.assertEqual(len(chksum), 32)
        self.assertEqual(chksum[0], '\x2e')
        self.assertEqual(chksum[31], '\xf5')
        self.assertEqual(chksum_type, hawkey.CHKSUM_SHA256)

    def test_checksum_fail(self):
        sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        sack.load_system_repo()
        pkg = base.by_name(sack, "fool")
        self.assertRaises(AttributeError, lambda: pkg.chksum)
        self.assertRaises(AttributeError, lambda: pkg.hdr_chksum)

class BaseurlTest(base.TestCase):
    def setUp(self):
        self.sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        self.sack.load_yum_repo()
        self.pkg = hawkey.Query(self.sack).filter(name="mystery-devel")[0]

    def test_baseurl(self):
        self.assertEqual(self.pkg.baseurl, 'blah')
