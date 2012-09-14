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
        self.assertIsNone(hawkey.Goal(self.sack).
                          upgrade_to(pkg, check_installed=False))
        # with the check it is not:
        goal = hawkey.Goal(self.sack)
        self.assertRaises(hawkey.Exception, goal.upgrade_to, package=pkg,
                          check_installed=True)
        # default value for check_installed is False:
        self.assertIsNone(hawkey.Goal(self.sack).upgrade_to(pkg))

    def test_install_query(self):
        query = hawkey.Query(self.sack).filter(name="walrus")
        # without checking versioning, the update is accepted:
        self.assertIsNone(hawkey.Goal(self.sack).upgrade(query=query));

    def test_install_erase_err(self):
        query = hawkey.Query(self.sack).filter(repo="karma")
        goal = hawkey.Goal(self.sack)
        self.assertRaises(hawkey.QueryException, goal.erase, query=query);

class Collector(object):
    def __init__(self):
        self.cnt = 0
        self.erasures = set()
        self.pkgs = set()

    def new_solution_cb(self, goal):
        self.cnt += 1
        self.erasures.update(goal.list_erasures())
        self.pkgs.update(goal.list_installs())

    def new_solution_cb_borked(self, goal):
        """ Raises AttributeError. """
        self.pkgs_borked.update(goal.list_erasures())

class GoalRun(base.TestCase):
    def test_run_callback(self):
        "Test goal.run() can use callback parameter just as well as run_all()"
        sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        sack.load_system_repo()
        sack.load_test_repo("main", "main.repo")

        pkg = base.by_name(sack, "penny-lib")
        goal = hawkey.Goal(sack)
        goal.erase(pkg)
        collector = Collector()
        self.assertTrue(goal.run(allow_uninstall=True,
                                 callback=collector.new_solution_cb))
        self.assertEqual(collector.cnt, 1)
        self.assertEqual(len(collector.erasures), 2)

class GoalRunAll(base.TestCase):
    def setUp(self):
        self.sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()
        self.sack.load_test_repo("greedy", "greedy.repo")
        self.goal = hawkey.Goal(self.sack)

    def test_cb(self):
        pkg_a = base.by_name(self.sack, "A")
        pkg_b = base.by_name(self.sack, "B")
        pkg_c = base.by_name(self.sack, "C")
        self.goal.install(pkg_a)

        collector = Collector()
        self.assertTrue(self.goal.run_all(collector.new_solution_cb))
        self.assertItemsEqual(collector.pkgs, [pkg_a, pkg_b, pkg_c])

    def test_cb_borked(self):
        """ Check exceptions are propagated from the callback. """
        self.goal.install(base.by_name(self.sack, "A"))
        collector = Collector()
        self.assertRaises(AttributeError,
                          self.goal.run_all, collector.new_solution_cb_borked)

class Problems(base.TestCase):
    def setUp(self):
        self.sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()
        self.sack.load_test_repo("main", "main.repo")
        self.goal = hawkey.Goal(self.sack)

    def test_errors(self):
        pkg = base.by_name(self.sack, "hello")
        self.goal.install(pkg)
        self.assertFalse(self.goal.run())
        self.assertEqual(len(self.goal.problems), 1)
        self.assertRaises(hawkey.RuntimeException, self.goal.list_erasures)
