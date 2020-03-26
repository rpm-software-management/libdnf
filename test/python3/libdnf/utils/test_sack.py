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

        removed = query.filter(libdnf.sack.ObjectQuery.get_string, libdnf.sack.QueryCmp_EQ, "Test o2")
        self.assertEqual(removed, 1)
        self.assertEqual(query.size(), 1)

    def test_installed(self):
        osack = libdnf.sack.ObjectSack()
        data = osack.get_data()

        o1 = libdnf.sack.Object()
        o1.repoid = "@System"
        data.push_back(o1)

        o2 = libdnf.sack.Object()
        o2.repoid = "fedora"
        data.push_back(o2)

        query = osack.new_query()
        self.assertEqual(query.size(), 2)

        query.filter(libdnf.sack.ObjectQuery.match_repoid, libdnf.sack.QueryCmp_EQ, "@System")
        self.assertEqual(query.size(), 1)
#        self.assertEqual(list(query.list())[0], o1)
