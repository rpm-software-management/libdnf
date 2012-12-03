import base
import hawkey

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
        result = list(subj.possibilities())
        self.assertLength(result, 1)
        self.assertEqual(result[0], hawkey.NEVRA(name='four-of-fish', epoch=8,
                                                 version='3.6.9',
                                                 release='11.fc100',
                                                 arch='x86_64'))

        subj = hawkey.Subject(INP_FOF_NOEPOCH, form=hawkey.FORM_NEVRA)
        result = list(subj.possibilities())
        self.assertEqual(result[0], hawkey.NEVRA(name='four-of-fish', epoch=None,
                                                 version='3.6.9',
                                                 release='11.fc100',
                                                 arch='x86_64'))

    def test_nevr(self):
        subj = hawkey.Subject(INP_FOF, form=hawkey.FORM_NEVR)
        expect = hawkey.NEVRA(name='four-of-fish', epoch=8, version='3.6.9',
                              release='11.fc100.x86_64', arch=None)
        self.assertEqual(list(subj.possibilities())[0], expect)

    def test_nevr_fail(self):
        subj = hawkey.Subject("four-of", form=hawkey.FORM_NEVR)
        self.assertLength(list(subj.possibilities()), 0)

    def test_nev(self):
        subj = hawkey.Subject(INP_FOF_NEV, form=hawkey.FORM_NEV)
        possibilities = list(subj.possibilities())
        self.assertLength(possibilities, 1)
        self.assertEqual(possibilities[0],
                         hawkey.NEVRA(name='four-of-fish', epoch=8,
                                      version='3.6.9', release=None, arch=None))

    def test_na(self):
        subj = hawkey.Subject(INP_FOF_NA, form=hawkey.FORM_NA)
        possibilities = list(subj.possibilities())
        self.assertLength(possibilities, 1)
        self.assertEqual(possibilities[0],
                         hawkey.NEVRA(name='four-of-fish-3.6.9', epoch=None,
                                      version=None, release=None, arch='i686'))

    def test_combined(self):
        subj = hawkey.Subject(INP_FOF)
        nevras = subj.possibilities()
        # the epoch in INP_FOF nicely limits the possibilities:
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
        nevras = subj.possibilities()
        self.assertEqual(nevras.next(), hawkey.NEVRA(name='four-of-fish',
                                                     epoch=None, version='3.6.9',
                                                     release='11.fc100',
                                                     arch='x86_64'))
        self.assertEqual(nevras.next(), hawkey.NEVRA(name='four-of-fish',
                                                     epoch=None, version='3.6.9',
                                                     release='11.fc100.x86_64',
                                                     arch=None))
        self.assertEqual(nevras.next(), hawkey.NEVRA(name='four-of-fish-3.6.9',
                                                     epoch=None,
                                                     version='11.fc100.x86_64',
                                                     release=None, arch=None))
        self.assertEqual(nevras.next(), hawkey.NEVRA(
                name='four-of-fish-3.6.9-11.fc100', epoch=None, version=None,
                release=None, arch='x86_64'))
        self.assertEqual(nevras.next(), hawkey.NEVRA(
                name='four-of-fish-3.6.9-11.fc100.x86_64', epoch=None,
                version=None, release=None, arch=None))
        self.assertRaises(StopIteration, nevras.next)

class SubjectRealPossibilitiesTest(base.TestCase):
    def setUp(self):
        self.sack = hawkey.test.TestSack(repo_dir=self.repo_dir)
        self.sack.load_system_repo()
        self.sack.load_test_repo("main", "main.repo")
        self.sack.load_test_repo("updates", "updates.repo")

    def test_none(self):
        subj = hawkey.Subject(INP_FOF_NOEPOCH)
        ret = list(subj.real_possibilities(self.sack))
        self.assertLength(ret, 0)

    def test_nevra(self):
        subj = hawkey.Subject("pilchard-1.2.4-1.x86_64")
        possibilities = subj.real_possibilities(self.sack)
        nevra = possibilities.next()
        self.assertEqual(nevra, hawkey.NEVRA(name='pilchard', epoch=None,
                                             version='1.2.4', release='1',
                                             arch='x86_64'))
        nevra = possibilities.next()
        self.assertEqual(nevra, hawkey.NEVRA(name='pilchard', epoch=None,
                                             version='1.2.4', release='1.x86_64',
                                             arch=None))
        self.assertRaises(StopIteration, possibilities.next)

    def test_wrong_arch(self):
        subj = hawkey.Subject("pilchard-1.2.4-1.ppc64")
        ret = list(subj.real_possibilities(self.sack))
        self.assertLength(ret, 1)
