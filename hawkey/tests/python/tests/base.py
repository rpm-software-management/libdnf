import hawkey
import os.path
import unittest

class TestCase(unittest.TestCase):
    repo_dir = os.path.normpath(os.path.join(__file__, "../../../repos"))

    def assertLength(self, collection, length):
        return self.assertEqual(len(collection), length)

def by_name(sack, name):
    return hawkey.Query(sack).filter(name=name)[0]

def by_name_repo(sack, name, repo):
    return hawkey.Query(sack).filter(name=name, reponame=repo)[0]
