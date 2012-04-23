import os.path
import unittest

class TestCase(unittest.TestCase):
    repo_dir = os.path.normpath(os.path.join(__file__, "../../../repos"))
