import base
import hawkey
import hawkey.test

class Reldep(base.TestCase):
    def setUp(self):
        self.sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()

    def test_basic(self):
        flying = base.by_name(self.sack, "flying")
        requires = flying.requires
        self.assertLength(requires, 1)
        reldep = requires[0]
        self.assertEqual(str(reldep), "P-lib >= 3")

    def test_custom_creation(self):
        reldep_str = "P-lib >= 3"
        reldep = hawkey.Reldep(self.sack, reldep_str)
        self.assertEqual(reldep_str, str(reldep))

    def test_custom_creation_fail(self):
        reldep_str = "lane = 4"
        self.assertRaises(hawkey.ValueException, hawkey.Reldep, self.sack,
                          reldep_str)
        reldep_str = "P-lib >="
        self.assertRaises(hawkey.ValueException, hawkey.Reldep, self.sack,
                          reldep_str)

    def test_custom_querying(self):
        reldep = hawkey.Reldep(self.sack, "P-lib = 3-3")
        q = hawkey.Query(self.sack).filter(provides=reldep)
        self.assertLength(q, 1)
        reldep = hawkey.Reldep(self.sack, "P-lib >= 3")
        q = hawkey.Query(self.sack).filter(provides=reldep)
        self.assertLength(q, 1)
        reldep = hawkey.Reldep(self.sack, "P-lib < 3-3")
        q = hawkey.Query(self.sack).filter(provides=reldep)
        self.assertLength(q, 0)

    def test_query_name_only(self):
        reldep_str = "P-lib"
        reldep = hawkey.Reldep(self.sack, reldep_str)
        self.assertEqual(reldep_str, str(reldep))
        q = hawkey.Query(self.sack).filter(provides=reldep)
        self.assertLength(q, 1)
        self.assertEqual(str(q[0]), "penny-lib-4-1.x86_64")
