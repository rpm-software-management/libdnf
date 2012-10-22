import hawkey
import unittest

class Sanity(unittest.TestCase):
    def test_imports(self):
        self.assertIn('test', hawkey.__all__)
        self.assertIn('Goal', hawkey.__all__)
        self.assertNotIn('_QUERY_KEYNAME_MAP', hawkey.__all__)
