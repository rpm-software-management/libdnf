import base
import hawkey
import hawkey.test

class Goal(base.TestCase):
    def setUp(self):
        self.sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()
        self.sack.load_test_repo("main", "main.repo")

    def test_upgrade(self):
        # select the installed "fool":
        pkg = hawkey.Query(self.sack).filter(name="walrus")[0]
        # without checking versioning, the update is accepted:
        self.assertTrue(hawkey.Goal(self.sack).upgrade_to(pkg,
                                                          check_installed=False))
        # with the check it is not:
        self.assertFalse(hawkey.Goal(self.sack).upgrade_to(package=pkg,
                                                           check_installed=True))
        # default value for check_installed is False:
        self.assertTrue(hawkey.Goal(self.sack).upgrade_to(pkg))

    def test_install_query(self):
        query = hawkey.Query(self.sack).filter(name="walrus")
        # without checking versioning, the update is accepted:
        self.assertTrue(hawkey.Goal(self.sack).upgrade(query=query));
