import hawkey
import unittest

class Util(unittest.TestCase):
    def test_chksum_name(self):
        name = hawkey.chksum_name(hawkey.CHKSUM_SHA256)
        self.assertEqual(name, "sha256")

    def test_chksum_type(self):
        t = hawkey.chksum_type("SHA1")
        self.assertEqual(t, hawkey.CHKSUM_SHA1)
        self.assertRaises(ValueError, hawkey.chksum_type, "maID")
