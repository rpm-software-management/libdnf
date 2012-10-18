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

    def test_split_nevra(self):
        self.assertRaises(hawkey.ValueException, hawkey.split_nevra, "no.go")

        nevra = hawkey.split_nevra("eyes-8:1.2.3-4.fc18.x86_64")
        self.assertEqual(nevra.name, "eyes")
        self.assertEqual(nevra.epoch, 8)
        self.assertEqual(nevra.version, "1.2.3")
        self.assertEqual(nevra.release, "4.fc18")
        self.assertEqual(nevra.arch, "x86_64")
