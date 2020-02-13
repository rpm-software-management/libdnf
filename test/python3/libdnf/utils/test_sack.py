import unittest

import libdnf.sack


class TestSack(unittest.TestCase):
    def test_object_sack(self):
        osack = libdnf.sack.ObjectSack()
        data = osack.get_data()

        o1 = libdnf.sack.Object()
        o1.string = "Test o1"
        o1.int64 = 64
        data.push_back(o1)

        o2 = libdnf.sack.Object()
        o2.string = "Test o2"
        o2.int64 = 264
        data.push_back(o2)

        query = osack.new_query()
        self.assertEqual(query.size(), 2)

        removed = query.filter(libdnf.sack.ObjectQuery_get_string, libdnf.sack.QueryCmp_EQ, "Test o2")
        self.assertEqual(removed, 1)
        self.assertEqual(query.size(), 1)
