import base
import hawkey
import hawkey.test

class Goal(base.TestCase):
    def setUp(self):
        self.sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        self.sack.load_rpm_repo()
        self.sack.load_test_repo("main", "main.repo")

    def test_update(self):
        goal = hawkey.Goal(self.sack)
        # select the installed "fool":
        pkg = hawkey.Query(self.sack).filter(name="walrus").run()[0]
        # without checking versioning, the update is accepted:
        self.assertTrue(hawkey.Goal(self.sack).update(pkg,
                                                      check_later_version=False))
        # with the check it is not:
        self.assertFalse(hawkey.Goal(self.sack).update(pkg,
                                                       check_later_version=True))
