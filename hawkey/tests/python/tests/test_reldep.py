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

from __future__ import absolute_import
from . import base

import hawkey

class Reldep(base.TestCase):
    def setUp(self):
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()

    def test_basic(self):
        flying = base.by_name(self.sack, "flying")
        requires = flying.requires
        self.assertLength(requires, 1)
        reldep = requires[0]
        self.assertEqual(str(reldep), "P-lib >= 3")

    def test_custom_creation(self):
        reldep_str = "P-lib >= 3"
        reldep = hawkey.Reldep(self.sack, reldep_str)
        self.assertEqual(reldep_str, str(reldep))

    def test_custom_creation_fail(self):
        reldep_str = "lane = 4"
        self.assertRaises(hawkey.ValueException, hawkey.Reldep, self.sack,
                          reldep_str)
        reldep_str = "P-lib >="
        self.assertRaises(hawkey.ValueException, hawkey.Reldep, self.sack,
                          reldep_str)

    def test_custom_querying(self):
        reldep = hawkey.Reldep(self.sack, "P-lib = 3-3")
        q = hawkey.Query(self.sack).filter(provides=reldep)
        self.assertLength(q, 1)
        reldep = hawkey.Reldep(self.sack, "P-lib >= 3")
        q = hawkey.Query(self.sack).filter(provides=reldep)
        self.assertLength(q, 1)
        reldep = hawkey.Reldep(self.sack, "P-lib < 3-3")
        q = hawkey.Query(self.sack).filter(provides=reldep)
        self.assertLength(q, 0)

    def test_query_name_only(self):
        reldep_str = "P-lib"
        reldep = hawkey.Reldep(self.sack, reldep_str)
        self.assertEqual(reldep_str, str(reldep))
        q = hawkey.Query(self.sack).filter(provides=reldep)
        self.assertLength(q, 1)
        self.assertEqual(str(q[0]), "penny-lib-4-1.x86_64")

    def test_not_crash(self):
        self.assertRaises(ValueError, hawkey.Reldep)

    def test_non_num_version(self):
        reldep_str = 'foo >= 1.0-1.fc20'
        with self.assertRaises(hawkey.ValueException) as ctx:
            hawkey.Reldep(self.sack, reldep_str)
        self.assertEqual(ctx.exception.args[0], "No such reldep: %s" % reldep_str)
