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
        result = list(subj.nevra_possibilities(form=hawkey.FORM_NEVRA))
        self.assertLength(result, 1)
        self.assertEqual(result[0], NEVRA(name='four-of-fish', epoch=8,
                                                 version='3.6.9',
                                                 release='11.fc100',
                                                 arch='x86_64'))

        subj = hawkey.Subject(INP_FOF_NOEPOCH)
        result = list(subj.nevra_possibilities(form=hawkey.FORM_NEVRA))
        self.assertEqual(result[0], NEVRA(name='four-of-fish', epoch=None,
                                                 version='3.6.9',
                                                 release='11.fc100',
                                                 arch='x86_64'))

    def test_nevr(self):
        subj = hawkey.Subject(INP_FOF)
        expect = NEVRA(name='four-of-fish', epoch=8, version='3.6.9',
                              release='11.fc100.x86_64', arch=None)
        self.assertEqual(list(subj.nevra_possibilities(form=hawkey.FORM_NEVR))[0],
                         expect)

    def test_nevr_fail(self):
        subj = hawkey.Subject("four-of")
        self.assertLength(list(subj.nevra_possibilities(form=hawkey.FORM_NEVR)),
                          0)

    def test_nev(self):
        subj = hawkey.Subject(INP_FOF_NEV)
        nevra_possibilities = list(subj.nevra_possibilities(form=hawkey.FORM_NEV))
        self.assertLength(nevra_possibilities, 1)
        self.assertEqual(nevra_possibilities[0],
                         NEVRA(name='four-of-fish', epoch=8,
                                      version='3.6.9', release=None, arch=None))

    def test_na(self):
        subj = hawkey.Subject(INP_FOF_NA)
        nevra_possibilities = list(subj.nevra_possibilities(form=hawkey.FORM_NA))
        self.assertLength(nevra_possibilities, 1)
        self.assertEqual(nevra_possibilities[0],
                         NEVRA(name='four-of-fish-3.6.9', epoch=None,
                                      version=None, release=None, arch='i686'))

    def test_custom_list(self):
        subj = hawkey.Subject(INP_FOF)
        result = list(subj.nevra_possibilities(form=[hawkey.FORM_NEVRA,
            hawkey.FORM_NEVR]))
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
        nevras = subj.nevra_possibilities()
        # the epoch in INP_FOF nicely limits the nevra_possibilities:
        self.assertEqual(next(nevras), NEVRA(name='four-of-fish', epoch=8,
                                                     version='3.6.9',
                                                     release='11.fc100',
                                                     arch='x86_64'))
        self.assertEqual(next(nevras), NEVRA(name='four-of-fish', epoch=8,
                                                     version='3.6.9',
                                                     release='11.fc100.x86_64',
                                                     arch=None))
        self.assertRaises(StopIteration, next, nevras)

        subj = hawkey.Subject(INP_FOF_NOEPOCH)
        nevras = subj.nevra_possibilities()
        self.assertEqual(next(nevras), NEVRA(name='four-of-fish',
                                                     epoch=None, version='3.6.9',
                                                     release='11.fc100',
                                                     arch='x86_64'))
        self.assertEqual(next(nevras), NEVRA(name='four-of-fish',
                                                     epoch=None, version='3.6.9',
                                                     release='11.fc100.x86_64',
                                                     arch=None))
        self.assertEqual(next(nevras), NEVRA(name='four-of-fish-3.6.9',
                                                     epoch=None,
                                                     version='11.fc100.x86_64',
                                                     release=None, arch=None))
        self.assertEqual(next(nevras), NEVRA(
                name='four-of-fish-3.6.9-11.fc100', epoch=None, version=None,
                release=None, arch='x86_64'))
        self.assertEqual(next(nevras), NEVRA(
                name='four-of-fish-3.6.9-11.fc100.x86_64', epoch=None,
                version=None, release=None, arch=None))
        self.assertRaises(StopIteration, next, nevras)

class SubjectRealPossibilitiesTest(base.TestCase):
    def setUp(self):
        self.sack = base.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()
        self.sack.load_test_repo("main", "main.repo")
        self.sack.load_test_repo("updates", "updates.repo")

    def test_none(self):
        subj = hawkey.Subject(INP_FOF_NOEPOCH)
        ret = list(subj.nevra_possibilities_real(self.sack))
        self.assertLength(ret, 0)

    def test_nevra(self):
        subj = hawkey.Subject("pilchard-1.2.4-1.x86_64")
        nevra_possibilities = subj.nevra_possibilities_real(self.sack)
        nevra = next(nevra_possibilities)

        self.assertEqual(nevra, NEVRA(name='pilchard', epoch=None,
                                             version='1.2.4', release='1',
                                             arch='x86_64'))
        nevra = next(nevra_possibilities)
        self.assertEqual(nevra, NEVRA(name='pilchard', epoch=None,
                                             version='1.2.4', release='1.x86_64',
                                             arch=None))
        self.assertRaises(StopIteration, next, nevra_possibilities)

    def test_nevra_toquery(self):
        subj = hawkey.Subject("pilchard-1.2.4-1.x86_64")
        nevra_possibilities = subj.nevra_possibilities_real(self.sack)
        nevra = next(nevra_possibilities)
        self.assertEqual(len(nevra.to_query(self.sack)), 1)

    def test_dash(self):
        """ Test that if a dash is present in otherwise simple subject, we take
            it as a name as the first guess.
        """
        subj = hawkey.Subject("penny-lib")
        nevra = next(subj.nevra_possibilities_real(self.sack))
        self.assertEqual(nevra, NEVRA(name='penny-lib', epoch=None,
                                             version=None, release=None,
                                             arch=None))

    def test_dash_version(self):
        subj = hawkey.Subject("penny-lib-4")

        nevra = next(subj.nevra_possibilities_real(self.sack))
        self.assertEqual(nevra, NEVRA(name="penny-lib", epoch=None,
                                             version='4', release=None,
                                             arch=None))

    def test_two_dashes(self):
        """ Even two dashes can happen, make sure they can still form a name. """
        subj = hawkey.Subject("penny-lib-devel")
        nevra = next(subj.nevra_possibilities_real(self.sack))
        self.assertEqual(nevra, NEVRA(name='penny-lib-devel', epoch=None,
                                             version=None, release=None,
                                             arch=None))

    def test_wrong_arch(self):
        subj = hawkey.Subject("pilchard-1.2.4-1.ppc64")
        ret = list(subj.nevra_possibilities_real(self.sack))
        self.assertLength(ret, 1)

    def test_globs(self):
        subj = hawkey.Subject("*-1.2.4-1.x86_64")
        nevras = list(subj.nevra_possibilities_real(self.sack, allow_globs=True))
        self.assertEqual(
            [NEVRA(name='*', epoch=None, version='1.2.4',
                          release='1', arch='x86_64'),
             NEVRA(name='*', epoch=None, version='1.2.4',
                          release='1.x86_64', arch=None)],
            nevras)

    def test_version_glob(self):
        subj = hawkey.Subject("pilchard-1.*-1.x86_64")
        nevras = list(subj.nevra_possibilities_real(self.sack, allow_globs=True))
        self.assertEqual(
            [NEVRA(name='pilchard', epoch=None, version='1.*',
                          release='1', arch='x86_64'),
             NEVRA(name='pilchard', epoch=None, version='1.*',
                          release='1.x86_64', arch=None)],
            nevras)

    def test_reldep(self):
        subj = hawkey.Subject("P-lib")
        self.assertRaises(StopIteration, next, subj.nevra_possibilities_real(self.sack))
        reldeps = subj.reldep_possibilities_real(self.sack)
        reldep = next(reldeps)
        self.assertEqual(str(reldep), "P-lib")
        self.assertRaises(StopIteration, next, reldeps)

    def test_reldep_fail(self):
        subj = hawkey.Subject("Package not exist")
        reldeps = subj.reldep_possibilities_real(self.sack)
        self.assertRaises(StopIteration, next, reldeps)

    def test_reldep_flags(self):
        subj = hawkey.Subject('fool <= 1-3')
        self.assertRaises(StopIteration, next,
                          subj.nevra_possibilities_real(self.sack))
        reldeps = subj.reldep_possibilities_real(self.sack)
        reldep = next(reldeps)
        self.assertEqual(str(reldep), "fool <= 1-3")
        self.assertRaises(StopIteration, next, reldeps)

        subj = hawkey.Subject('meanwhile <= 1-3')
        reldeps = subj.reldep_possibilities_real(self.sack)
        self.assertRaises(StopIteration, next, reldeps)

    def test_icase(self):
        subj = hawkey.Subject("penny-lib-DEVEL")
        nevra = next(subj.nevra_possibilities_real(self.sack, icase=True))
        self.assertEqual(nevra.name, "penny-lib-DEVEL")

    def test_nonexistent_version(self):
        subj = hawkey.Subject("penny-5")
        self.assertLength(list(subj.nevra_possibilities_real(self.sack)), 0)

    def test_glob_arches(self):
        subj = hawkey.Subject("pilchard-1.2.3-1.*6*")
        nevras = list(subj.nevra_possibilities_real(self.sack, allow_globs=True))
        self.assertLength(nevras, 2)
        self.assertEqual(nevras[0].arch, "*6*")
        self.assertEqual(nevras[1].arch, None)
