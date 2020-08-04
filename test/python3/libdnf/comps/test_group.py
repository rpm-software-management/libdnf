# Copyright (C) 2020 Red Hat, Inc.
#
# This file is part of libdnf: https://github.com/rpm-software-management/libdnf/
#
# Libdnf is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# Libdnf is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with libdnf.  If not, see <https://www.gnu.org/licenses/>.


import os
import unittest

import libdnf.comps


class TestGroup(unittest.TestCase):
    def test_group(self):
        comps = libdnf.comps.Comps()
        comps.load_installed()

        # TODO(dmach): get_group_sack() returns generic SwigPyObject that has no new_query attribute
        #q_core = comps.get_group_sack().new_query()
        #core = q_core.get()
