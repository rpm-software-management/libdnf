import unittest

import libdnf.utils


class TestUtilsNumber(unittest.TestCase):
    def test_random_int32(self):
        value = libdnf.utils.random_int32(42, 42)
        self.assertEqual(42, value)
        1/0
