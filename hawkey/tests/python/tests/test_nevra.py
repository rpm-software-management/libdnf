#
# Copyright (C) 2012-2013 Red Hat, Inc.
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

class NEVRATest(unittest.TestCase):
    def test_evr_cmp(self):
        sack = hawkey.Sack()
        n1 = hawkey.split_nevra("jay-3:3.10-4.fc3.x86_64")
        n2 = hawkey.split_nevra("jay-4.10-4.fc3.x86_64")
        self.assertGreater(n1.evr_cmp(n2, sack), 0)
        self.assertLess(n2.evr_cmp(n1, sack), 0)

        n1 = hawkey.split_nevra("jay-3.10-4.fc3.x86_64")
        n2 = hawkey.split_nevra("jay-3.10-5.fc3.x86_64")
        self.assertLess(n1.evr_cmp(n2, sack), 0)
