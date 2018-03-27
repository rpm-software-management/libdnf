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
from hawkey import NEVRA
from sys import version_info as python_version

import hawkey

INP_FOF="four-of-fish-8:3.6.9-11.fc100.x86_64"
INP_FOF_NOEPOCH="four-of-fish-3.6.9-11.fc100.x86_64"
INP_FOF_NEV="four-of-fish-8:3.6.9"
INP_FOF_NA="four-of-fish-3.6.9.i686"

class SubjectTest(base.TestCase):
    def test_nevra(self):
        subj = hawkey.Subject(INP_FOF)
        result = subj.get_nevra_possibilities(forms=hawkey.FORM_NEVRA)
        self.assertLength(result, 1)
        self.assertEqual(result[0], NEVRA(name='four-of-fish', epoch=8,
                                                 version='3.6.9',
                                                 release='11.fc100',
                                                 arch='x86_64'))

        subj = hawkey.Subject(INP_FOF_NOEPOCH)
        result = subj.get_nevra_possibilities(forms=hawkey.FORM_NEVRA)
        self.assertEqual(result[0], NEVRA(name='four-of-fish', epoch=None,
                                                 version='3.6.9',
                                                 release='11.fc100',
                                                 arch='x86_64'))

    def test_nevr(self):
        subj = hawkey.Subject(INP_FOF)
        expect = NEVRA(name='four-of-fish', epoch=8, version='3.6.9',
                              release='11.fc100.x86_64', arch=None)
        self.assertEqual(subj.get_nevra_possibilities(forms=hawkey.FORM_NEVR)[0],
                         expect)

    def test_nevr_fail(self):
        subj = hawkey.Subject("four-of")
        self.assertLength(subj.get_nevra_possibilities(forms=hawkey.FORM_NEVR),
                          0)

    def test_nev(self):
        subj = hawkey.Subject(INP_FOF_NEV)
        nevra_possibilities = subj.get_nevra_possibilities(forms=hawkey.FORM_NEV)
        self.assertLength(nevra_possibilities, 1)
        self.assertEqual(nevra_possibilities[0],
                         NEVRA(name='four-of-fish', epoch=8,
                                      version='3.6.9', release=None, arch=None))

    def test_na(self):
        subj = hawkey.Subject(INP_FOF_NA)
        nevra_possibilities = subj.get_nevra_possibilities(forms=hawkey.FORM_NA)
        self.assertLength(nevra_possibilities, 1)
        self.assertEqual(nevra_possibilities[0],
                         NEVRA(name='four-of-fish-3.6.9', epoch=None,
                                      version=None, release=None, arch='i686'))

    def test_custom_list(self):
        subj = hawkey.Subject(INP_FOF)
        result = subj.get_nevra_possibilities(forms=[hawkey.FORM_NEVRA,
                                              hawkey.FORM_NEVR])
        self.assertLength(result, 2)
        self.assertEqual(result[0], NEVRA(name='four-of-fish', epoch=8,
                                                 version='3.6.9',
                                                 release='11.fc100',
                                                 arch='x86_64'))
        self.assertEqual(result[1], NEVRA(name='four-of-fish', epoch=8, version='3.6.9',
                              release='11.fc100.x86_64', arch=None))

    def test_combined(self):
        """ Test we get all the possible NEVRA parses. """
        subj = hawkey.Subject(INP_FOF)
        # the epoch in INP_FOF nicely limits the nevra_possibilities:
        expect = (
            NEVRA(name='four-of-fish', epoch=8, version='3.6.9', release='11.fc100', arch='x86_64'),
            NEVRA(name='four-of-fish', epoch=8, version='3.6.9', release='11.fc100.x86_64',
                  arch=None)
        )
        result = subj.get_nevra_possibilities()
        self.assertEqual(len(result), len(expect))
        for idx in range(0, len(expect)):
            self.assertEqual(expect[idx], result[idx])

        subj = hawkey.Subject(INP_FOF_NOEPOCH)
        expect = (
            NEVRA(name='four-of-fish', epoch=None, version='3.6.9',
                  release='11.fc100', arch='x86_64'),
            NEVRA(name='four-of-fish-3.6.9-11.fc100', epoch=None, version=None,
                  release=None, arch='x86_64'),
            NEVRA(name='four-of-fish-3.6.9-11.fc100.x86_64', epoch=None, version=None,
                  release=None, arch=None),
            NEVRA(name='four-of-fish', epoch=None, version='3.6.9',
                  release='11.fc100.x86_64', arch=None),
            NEVRA(name='four-of-fish-3.6.9', epoch=None, version='11.fc100.x86_64',
                  release=None, arch=None)
        )
        result = subj.get_nevra_possibilities()
        self.assertEqual(len(result), len(expect))
        for idx in range(0, len(expect)):
            self.assertEqual(expect[idx], result[idx])

