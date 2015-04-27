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

"""Tests of the _hawkey.Advisory class."""

from __future__ import absolute_import
from __future__ import unicode_literals
from . import base

import datetime
import hawkey
import itertools
import time

def find_advisory(sack, id_):
    """Find an advisory with given ID."""
    # The function is needed because advisories cannot be retrieved directly.
    advisories_iterable = itertools.chain(
        (pkg.get_advisories(hawkey.LT) for pkg in hawkey.Query(sack)),
        (pkg.get_advisories(hawkey.GT | hawkey.EQ) for pkg in hawkey.Query(sack)))
    for advisory in itertools.chain.from_iterable(advisories_iterable):
        if advisory.id == id_:
            return advisory

class Test(base.TestCase):
    """Test case consisting of one random advisory."""

    def setUp(self):
        """Prepare the test fixture."""
        sack = base.TestSack(repo_dir=self.repo_dir)
        sack.load_repo(load_updateinfo=True)
        self.advisory = find_advisory(sack, 'FEDORA-2008-9969')

    def test_description(self):
        self.assertEqual(self.advisory.description, 'An example update to the tour package.')

    def test_packages(self):
        filenames = [apkg.filename for apkg in self.advisory.packages]
        self.assertEqual(filenames, ['tour.noarch.rpm'])

    def test_filenames(self):
        self.assertEqual(self.advisory.filenames, ['tour.noarch.rpm'])

    def test_id(self):
        self.assertEqual(self.advisory.id, 'FEDORA-2008-9969')

    def test_references(self):
        urls = [ref.url for ref in self.advisory.references]
        self.assertEqual(urls,
                         ['https://bugzilla.redhat.com/show_bug.cgi?id=472090',
                          'https://bugzilla.gnome.com/show_bug.cgi?id=472091'])

    def test_rights(self):
        self.assertIsNone(self.advisory.rights)

    def test_title(self):
        self.assertEqual(self.advisory.title, 'lvm2-2.02.39-7.fc10')

    def test_type(self):
        self.assertEqual(self.advisory.type, hawkey.ADVISORY_BUGFIX)

    def test_updated(self):
        self.assertEqual(self.advisory.updated,
                         datetime.datetime(2008, 12, 9, 11, 31, 26) -
                         datetime.timedelta(seconds=time.timezone))
