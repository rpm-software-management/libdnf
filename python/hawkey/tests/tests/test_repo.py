#
# Copyright (C) 2012-2019 Red Hat, Inc.
#
# Licensed under the GNU Lesser General Public License Version 2.1
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
#

import hawkey
import unittest


class Package(unittest.TestCase):
    def test_create(self):
        r = hawkey.Repo("fog")
        self.assertIsNotNone(r)
        self.assertRaises(TypeError, hawkey.Repo);
        self.assertRaises(TypeError, hawkey.Repo, 3)
        self.assertRaises(TypeError, hawkey.Repo, rain="pouring")

    def test_cost_assignment(self):
        r = hawkey.Repo("fog")
        r.cost = 300
        self.assertEqual(300, r.cost)

        r2 = hawkey.Repo("blizzard")
        self.assertEqual(1000, r2.cost)
        with self.assertRaises(TypeError):
            r2.cost = '4'

    def test_max_parallel_downloads(self):
        r = hawkey.Repo("fog")
        r.max_parallel_downloads = 10
        self.assertEqual(10, r.max_parallel_downloads)

    def test_max_downloads_per_mirror(self):
        r = hawkey.Repo("fog")
        r.max_downloads_per_mirror = 10
        self.assertEqual(10, r.max_downloads_per_mirror)

    def test_str_assignment(self):
        r = hawkey.Repo('fog')
        with self.assertRaises(TypeError):
            r.repomd_fn = 3
        r.repomd_fn = 'rain'
        self.assertEqual(r.repomd_fn, 'rain')

