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

"""Tests of the _hawkey.AdvisoryRef class."""

from __future__ import absolute_import
from __future__ import unicode_literals
from . import base

import hawkey
import itertools

def find_reference(sack, url):
    """Find an advisory reference with given URL."""
    # The function is needed because references cannot be retrieved directly.
    advisories_iterable = itertools.chain(
        (pkg.get_advisories(hawkey.LT) for pkg in hawkey.Query(sack)),
        (pkg.get_advisories(hawkey.GT | hawkey.EQ) for pkg in hawkey.Query(sack)))
    advisories = itertools.chain.from_iterable(advisories_iterable)
    references_iterable = (advisory.references for advisory in advisories)
    for reference in itertools.chain.from_iterable(references_iterable):
        if reference.url == url:
            return reference

class Test(base.TestCase):
    """Test case consisting of one random advisory reference."""

    def setUp(self):
        """Prepare the test fixture."""
        sack = base.TestSack(repo_dir=self.repo_dir)
        sack.load_repo(load_updateinfo=True)
        self.reference = find_reference(
            sack, 'https://bugzilla.redhat.com/show_bug.cgi?id=472090')

    def test_type(self):
        self.assertEqual(self.reference.type, hawkey.REFERENCE_BUGZILLA)

    def test_id(self):
        self.assertEqual(self.reference.id, '472090')

    def test_title(self):
        self.assertEqual(self.reference.title,
                         '/etc/init.d/clvmd points to /usr/sbin for LVM tools')

    def test_url(self):
        self.assertEqual(self.reference.url,
                         'https://bugzilla.redhat.com/show_bug.cgi?id=472090')
