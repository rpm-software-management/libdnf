import unittest
import hawkey

class Version(unittest.TestCase):
    def test_version(self):
        self.assertIsInstance(hawkey.VERSION, unicode)
        self.assertGreaterEqual(len(hawkey.VERSION), 5)
        self.assertEqual(hawkey.VERSION.count('.'), 2)
