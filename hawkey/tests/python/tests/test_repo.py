import unittest

import hawkey

class Package(unittest.TestCase):
    def test_create(self):
        r = hawkey.Repo("fog")
        self.assertIsNotNone(r)
        self.assertRaises(TypeError, hawkey.Repo);
        self.assertRaises(TypeError, hawkey.Repo, 3)
        self.assertRaises(TypeError, hawkey.Repo, rain="pouring")
