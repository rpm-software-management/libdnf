import hawkey
import unittest

class NEVRATest(unittest.TestCase):
    def test_evr_cmp(self):
        sack = hawkey.Sack()
        n1 = hawkey.split_nevra("jay-3:3.10-4.fc3.x86_64")
        n2 = hawkey.split_nevra("jay-4.10-4.fc3.x86_64")
        self.assertGreater(n1.evr_cmp(n2, sack), 0)
        self.assertLess(n2.evr_cmp(n1, sack), 0)

        n1 = hawkey.split_nevra("jay-3.10-4.fc3.x86_64")
        n2 = hawkey.split_nevra("jay-3.10-5.fc3.x86_64")
        self.assertLess(n1.evr_cmp(n2, sack), 0)
