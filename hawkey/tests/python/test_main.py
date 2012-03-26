#! /usr/bin/python

import os
import sys
import unittest

if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.stderr.write("synopsis: %s <module directory> <repo directory>\n" %
                         (sys.argv[0]))
        sys.exit(1)
    sys.path.insert(0, sys.argv[1])
    repo_dir = sys.argv[2]

    import test_sack
    import test_query

    all_suites = unittest.TestSuite()
    all_suites.addTests(test_sack.suite(repo_dir))
    all_suites.addTests(test_query.suite(repo_dir))
    result = unittest.TextTestRunner().run(all_suites)
    sys.exit(0 if result.wasSuccessful() else 1)
