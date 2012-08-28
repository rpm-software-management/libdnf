import base
import hawkey
import hawkey.test

class Package(base.TestCase):
    def setUp(self):
        self.sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()
        q = hawkey.Query(self.sack).filter(name__eq="flying")
        self.pkg = list(q)[0]

    def test_repr(self):
        self.assertRegexpMatches(repr(self.pkg),
                                 "<_hawkey.Package object, id: \d+>")

    def test_str(self):
        self.assertEqual(str(self.pkg),
                         "flying-2-9.noarch")

class WithYumRepo(base.TestCase):
    def setUp(self):
        self.sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        self.sack.load_yum_repo()
        self.pkg = hawkey.Query(self.sack).filter(name="mystery")[0]

    def test_checksum(self):
        (chksum_type, chksum) = self.pkg.chksum
        self.assertEqual(len(chksum), 32)
        self.assertEqual(chksum[0], '\xb2')
        self.assertEqual(chksum[31], '\x7a')
        self.assertEqual(chksum_type, hawkey.CHKSUM_SHA256)
