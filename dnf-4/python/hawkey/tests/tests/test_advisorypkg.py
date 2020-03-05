#
# Copyright (C) 2014 Red Hat, Inc.
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

"""Tests of the _hawkey.AdvisoryPkg class."""

from __future__ import absolute_import
from __future__ import unicode_literals
from . import base

import hawkey
import itertools

def find_apackage(sack, filename):
    """Find an advisory package with given file name."""
    # The function is needed because packages cannot be retrieved directly.
    advisories_iterable = itertools.chain(
        (pkg.get_advisories(hawkey.LT) for pkg in hawkey.Query(sack)),
        (pkg.get_advisories(hawkey.GT | hawkey.EQ) for pkg in hawkey.Query(sack)))
    advisories = itertools.chain.from_iterable(advisories_iterable)
    apackages_iterable = (advisory.packages for advisory in advisories)
    for apackage in itertools.chain.from_iterable(apackages_iterable):
        if apackage.filename == filename:
            return apackage

class Test(base.TestCase):
    """Test case consisting of one random advisory package."""

    def setUp(self):
        """Prepare the test fixture."""
        sack = base.TestSack(repo_dir=self.repo_dir)
        sack.load_repo(load_updateinfo=True)
        self.apackage = find_apackage(sack, 'tour.noarch.rpm')

    def test_name(self):
        self.assertEqual(self.apackage.name, 'tour')

    def test_evr(self):
        self.assertEqual(self.apackage.evr, '4-7')

    def test_arch(self):
        self.assertEqual(self.apackage.arch, 'noarch')

    def test_filename(self):
        self.assertEqual(self.apackage.filename, 'tour.noarch.rpm')
