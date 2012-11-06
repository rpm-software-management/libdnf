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
        self.assertIsInstance(reldep, hawkey.Reldep)
        self.assertEqual(str(reldep), "P-lib >= 3")
