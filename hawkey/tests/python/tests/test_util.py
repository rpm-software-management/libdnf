import hawkey
import unittest

class Util(unittest.TestCase):
    def test_chksum_name(self):
        name = hawkey.chksum_name(hawkey.CHKSUM_SHA256)
        self.assertEqual(name, "sha256")
