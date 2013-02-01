import base
import hawkey
import hawkey.test

INP_FOF="four-of-fish-8:3.6.9-11.fc100.x86_64"
INP_FOF_NOEPOCH="four-of-fish-3.6.9-11.fc100.x86_64"
INP_FOF_NEV="four-of-fish-8:3.6.9"
INP_FOF_NA="four-of-fish-3.6.9.i686"

class SubjectTest(base.TestCase):
    def test_tokenizer(self):
        subj = hawkey.Subject(INP_FOF)
        rejoin = "".join(map(str, subj.tokens))
        self.assertEqual(rejoin, INP_FOF)

    def test_nevra(self):
        subj = hawkey.Subject(INP_FOF, form=hawkey.FORM_NEVRA)
        result = list(subj.nevra_possibilities())
        self.assertLength(result, 1)
        self.assertEqual(result[0], hawkey.NEVRA(name='four-of-fish', epoch=8,
                                                 version='3.6.9',
                                                 release='11.fc100',
                                                 arch='x86_64'))

        subj = hawkey.Subject(INP_FOF_NOEPOCH, form=hawkey.FORM_NEVRA)
        result = list(subj.nevra_possibilities())
        self.assertEqual(result[0], hawkey.NEVRA(name='four-of-fish', epoch=None,
                                                 version='3.6.9',
                                                 release='11.fc100',
                                                 arch='x86_64'))

    def test_nevr(self):
        subj = hawkey.Subject(INP_FOF, form=hawkey.FORM_NEVR)
        expect = hawkey.NEVRA(name='four-of-fish', epoch=8, version='3.6.9',
                              release='11.fc100.x86_64', arch=None)
        self.assertEqual(list(subj.nevra_possibilities())[0], expect)

    def test_nevr_fail(self):
        subj = hawkey.Subject("four-of", form=hawkey.FORM_NEVR)
        self.assertLength(list(subj.nevra_possibilities()), 0)

    def test_nev(self):
        subj = hawkey.Subject(INP_FOF_NEV, form=hawkey.FORM_NEV)
        nevra_possibilities = list(subj.nevra_possibilities())
        self.assertLength(nevra_possibilities, 1)
        self.assertEqual(nevra_possibilities[0],
                         hawkey.NEVRA(name='four-of-fish', epoch=8,
                                      version='3.6.9', release=None, arch=None))

    def test_na(self):
        subj = hawkey.Subject(INP_FOF_NA, form=hawkey.FORM_NA)
        nevra_possibilities = list(subj.nevra_possibilities())
        self.assertLength(nevra_possibilities, 1)
        self.assertEqual(nevra_possibilities[0],
                         hawkey.NEVRA(name='four-of-fish-3.6.9', epoch=None,
                                      version=None, release=None, arch='i686'))

    def test_combined(self):
        """ Test we get all the possible NEVRA parses. """
        subj = hawkey.Subject(INP_FOF)
        nevras = subj.nevra_possibilities()
        # the epoch in INP_FOF nicely limits the nevra_possibilities:
        self.assertEqual(nevras.next(), hawkey.NEVRA(name='four-of-fish', epoch=8,
                                                     version='3.6.9',
                                                     release='11.fc100',
                                                     arch='x86_64'))
        self.assertEqual(nevras.next(), hawkey.NEVRA(name='four-of-fish', epoch=8,
                                                     version='3.6.9',
                                                     release='11.fc100.x86_64',
                                                     arch=None))
        self.assertRaises(StopIteration, nevras.next)

        subj = hawkey.Subject(INP_FOF_NOEPOCH)
        nevras = subj.nevra_possibilities()
        self.assertEqual(nevras.next(), hawkey.NEVRA(name='four-of-fish',
                                                     epoch=None, version='3.6.9',
                                                     release='11.fc100',
                                                     arch='x86_64'))
        self.assertEqual(nevras.next(), hawkey.NEVRA(
                name='four-of-fish-3.6.9-11.fc100', epoch=None, version=None,
                release=None, arch='x86_64'))
        self.assertEqual(nevras.next(), hawkey.NEVRA(
                name='four-of-fish-3.6.9-11.fc100.x86_64', epoch=None,
                version=None, release=None, arch=None))
        self.assertEqual(nevras.next(), hawkey.NEVRA(name='four-of-fish',
                                                     epoch=None, version='3.6.9',
                                                     release='11.fc100.x86_64',
                                                     arch=None))
        self.assertEqual(nevras.next(), hawkey.NEVRA(name='four-of-fish-3.6.9',
                                                     epoch=None,
                                                     version='11.fc100.x86_64',
                                                     release=None, arch=None))
        self.assertRaises(StopIteration, nevras.next)

class SubjectRealPossibilitiesTest(base.TestCase):
    def setUp(self):
        self.sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
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
        nevra = nevra_possibilities.next()
        self.assertEqual(nevra, hawkey.NEVRA(name='pilchard', epoch=None,
                                             version='1.2.4', release='1',
                                             arch='x86_64'))
        nevra = nevra_possibilities.next()
        self.assertEqual(nevra, hawkey.NEVRA(name='pilchard', epoch=None,
                                             version='1.2.4', release='1.x86_64',
                                             arch=None))
        self.assertRaises(StopIteration, nevra_possibilities.next)

    def test_dash(self):
        """ Test that if a dash is present in otherwise simple subject, we take
            it as a name as the first guess.
        """
        subj = hawkey.Subject("penny-lib")
        nevra = subj.nevra_possibilities_real(self.sack).next()
        self.assertEqual(nevra, hawkey.NEVRA(name='penny-lib', epoch=None,
                                             version=None, release=None,
                                             arch=None))

    def test_two_dashes(self):
        """ Even two dashes can happen, make sure they can still form a name. """
        subj = hawkey.Subject("penny-lib-devel")
        nevra = subj.nevra_possibilities_real(self.sack).next()
        self.assertEqual(nevra, hawkey.NEVRA(name='penny-lib-devel', epoch=None,
                                             version=None, release=None,
                                             arch=None))

    def test_wrong_arch(self):
        subj = hawkey.Subject("pilchard-1.2.4-1.ppc64")
        ret = list(subj.nevra_possibilities_real(self.sack))
        self.assertLength(ret, 1)

    def test_globs(self):
        subj = hawkey.Subject("*-1.2.4-1.x86_64")
        ret = list(subj.nevra_possibilities_real(self.sack, allow_globs=True))
        self.assertLength(ret, 5)

    def test_reldep(self):
        subj = hawkey.Subject("P-lib")
        self.assertRaises(StopIteration, subj.nevra_possibilities_real(self.sack).next)
        reldeps = subj.reldep_possibilities_real(self.sack)
        reldep = reldeps.next()
        self.assertEqual(str(reldep), "P-lib")
        self.assertRaises(StopIteration, reldeps.next)

    def test_icase(self):
        subj = hawkey.Subject("penny-lib-DEVEL")
        nevra = subj.nevra_possibilities_real(self.sack, icase=True).next()
        self.assertEqual(nevra.name, "penny-lib-DEVEL")
