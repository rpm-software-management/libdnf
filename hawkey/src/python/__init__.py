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
from sys import version_info as python_version

from . import _hawkey
import collections
import itertools
import operator
import re
import types

__all__ = [
    # version info
    'VERSION', 'VERSION_MAJOR', 'VERSION_MINOR', 'VERSION_PATCH',
    # submodules
    'test',
    # constants
    'CHKSUM_MD5', 'CHKSUM_SHA1', 'CHKSUM_SHA256', 'CMDLINE_REPO_NAME',
    'SYSTEM_REPO_NAME', 'REASON_DEP', 'REASON_USER', 'ICASE',
    'FORM_NEVRA', 'FORM_NEVR', 'FORM_NEV', 'FORM_NA', 'FORM_NAME', 'FORM_ALL',
    # exceptions
    'ArchException', 'Exception', 'QueryException', 'RuntimeException',
    'ValueException',
    # functions
    'chksum_name', 'chksum_type', 'split_nevra',
    # classes
    'Goal', 'NEVRA', 'Package', 'Query', 'Repo', 'Sack', 'Selector', 'Subject']

_QUERY_KEYNAME_MAP = {
    'pkg'	  : _hawkey.PKG,
    'arch'	  : _hawkey.PKG_ARCH,
    'conflicts'   : _hawkey.PKG_CONFLICTS,
    'description' : _hawkey.PKG_DESCRIPTION,
    'downgrades'  : _hawkey.PKG_DOWNGRADES,
    'empty'	  : _hawkey.PKG_EMPTY,
    'epoch'	  : _hawkey.PKG_EPOCH,
    'evr'	  : _hawkey.PKG_EVR,
    'file'	  : _hawkey.PKG_FILE,
    'latest'	  : _hawkey.PKG_LATEST,
    'latest_per_arch': _hawkey.PKG_LATEST_PER_ARCH,
    'location'	  : _hawkey.PKG_LOCATION,
    'name'	  : _hawkey.PKG_NAME,
    'obsoletes'	  : _hawkey.PKG_OBSOLETES,
    'provides'	  : _hawkey.PKG_PROVIDES,
    'release'	  : _hawkey.PKG_RELEASE,
    'reponame'	  : _hawkey.PKG_REPONAME,
    'requires'	  : _hawkey.PKG_REQUIRES,
    'sourcerpm'	  : _hawkey.PKG_SOURCERPM,
    'summary'	  : _hawkey.PKG_SUMMARY,
    'upgrades'	  : _hawkey.PKG_UPGRADES,
    'url'	  : _hawkey.PKG_URL,
    'version'	  : _hawkey.PKG_VERSION,
}

_CMP_MAP = {
    '=' : _hawkey.EQ,
    '>' : _hawkey.GT,
    '<' : _hawkey.LT,
    '!=' : _hawkey.NEQ,
    '>=' : _hawkey.EQ | _hawkey.GT,
    '<=' : _hawkey.EQ | _hawkey.LT,
    }

_QUERY_CMP_MAP = {
    'eq' : _hawkey.EQ,
    'gt' : _hawkey.GT,
    'lt' : _hawkey.LT,
    'neq' : _hawkey.NEQ,
    'gte' : _hawkey.EQ | _hawkey.GT,
    'lte' : _hawkey.EQ | _hawkey.LT,
    'substr'  : _hawkey.SUBSTR,
    'glob' : _hawkey.GLOB,
    }

VERSION_MAJOR = _hawkey.VERSION_MAJOR
VERSION_MINOR = _hawkey.VERSION_MINOR
VERSION_PATCH = _hawkey.VERSION_PATCH
VERSION=u"%d.%d.%d" % (VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)

SYSTEM_REPO_NAME = _hawkey.SYSTEM_REPO_NAME
CMDLINE_REPO_NAME = _hawkey.CMDLINE_REPO_NAME

ICASE = _hawkey.ICASE

CHKSUM_MD5 = _hawkey.CHKSUM_MD5
CHKSUM_SHA1 = _hawkey.CHKSUM_SHA1
CHKSUM_SHA256 = _hawkey.CHKSUM_SHA256

REASON_DEP = _hawkey.REASON_DEP
REASON_USER = _hawkey.REASON_USER

Package = _hawkey.Package
Reldep = _hawkey.Reldep
Repo = _hawkey.Repo
Sack = _hawkey.Sack

Exception = _hawkey.Exception
QueryException = _hawkey.QueryException
ValueException = _hawkey.ValueException
ArchException = _hawkey.ArchException
RuntimeException = _hawkey.RuntimeException

chksum_name = _hawkey.chksum_name
chksum_type = _hawkey.chksum_type

def split_nevra(s):
    t = _hawkey.split_nevra(s)
    return NEVRA(*t)

_NEVRA = collections.namedtuple("_NEVRA",
                                ["name", "epoch", "version", "release", "arch"])

class NEVRA(_NEVRA):
    __slots__ = ()

    def evr_cmp(self, other, sack):
        return sack.evr_cmp(self.evr, other.evr)

    @property
    def evr(self):
        fmt= "%(epoch)d:%(version)s-%(release)s"
        if self.epoch is None:
            fmt = "%(version)s-%(release)s"
        return fmt % self._asdict()

    def to_query(self, sack):
        return Query(sack).filter(
            name=self.name, epoch=self.epoch, version=self.version,
            release=self.release, arch=self.arch)

class Goal(_hawkey.Goal):
    _reserved_kw	= set(['package', 'select'])
    _flag_kw		= set(['clean_deps', 'check_installed'])

    def _auto_selector(fn):
        def tweaked_fn(self, *args, **kwargs):
            if args or self._reserved_kw & set(kwargs):
                return fn(self, *args, **kwargs)

            # only the flags and unrecognized keywords remained, construct a Selector
            new_kwargs = {}
            for flag in self._flag_kw & set(kwargs):
                new_kwargs[flag] = kwargs.pop(flag)
            new_kwargs['select'] = Selector(self.sack).set(**kwargs)
            return fn(self, **new_kwargs)
        return tweaked_fn

    @property
    def problems(self):
        return [self.describe_problem(i) for i in range(0, self.count_problems())]

    def run(self, callback=None, **kwargs):
        ret = super(Goal, self).run(**kwargs)
        if callback:
            callback(self)
        return ret

    @_auto_selector
    def erase(self, *args, **kwargs):
        super(Goal, self).erase(*args, **kwargs)

    @_auto_selector
    def install(self, *args, **kwargs):
        super(Goal, self).install(*args, **kwargs)

    @_auto_selector
    def upgrade(self, *args, **kwargs):
        super(Goal, self).upgrade(*args, **kwargs)

def _encode(obj):
    """ Identity, except when obj is unicode then return a UTF-8 string.

        This assumes UTF-8 is good enough for libsolv and always will be. Else
        we'll have to deal with some encoding configuration.

        Since we use this to match string queries, we have to enforce 'strict'
        and potentially face exceptions rather than bizarre results. (Except
        that as long as we stick to UTF-8 it never fails.)
    """
    if python_version.major < 3 and isinstance(obj, unicode):
        return obj.encode('utf8', 'strict')
    return obj

def _parse_filter_args(flags, dct):
    args = []
    filter_flags = 0
    for flag in flags:
        if not flag in [ICASE]:
            raise ValueException("unrecognized flag: %s" % flag)
        filter_flags |= flag
    for (k, match) in dct.items():
        if isinstance(match, Query):
            pass
        elif python_version.major < 3 and isinstance(match, basestring):
            match = _encode(match)
        elif python_version.major >= 3 and isinstance(match, str):
            match = _encode(match)
        elif isinstance(match, collections.Iterable):
            match = list(map(_encode, match))
        split = k.split("__", 1)
        if len(split) == 1:
            keyname=split[0]
            cmp_type = "eq"
        elif len(split) == 2:
            (keyname, cmp_type) = split
        else:
            raise ValueError("keyword arguments given to filter() need be "
                             "in <key>__<comparison type>=<value> format")
        if not keyname in _QUERY_KEYNAME_MAP:
            raise ValueException("Unrecognized key name: %s" % keyname)
        if not cmp_type in _QUERY_CMP_MAP:
            raise ValueException("Unrecognized filter type: %s" % cmp_type)
        args.append((_QUERY_KEYNAME_MAP[keyname],
                     _QUERY_CMP_MAP[cmp_type]|filter_flags,
                     match))
    return args

class Query(_hawkey.Query):
    def __init__(self, sack=None, query=None):
        super(Query, self).__init__(sack=sack, query=query)
        self._result = None

    def __add__(self, operand):
        if not isinstance(operand, list):
            raise TypeError("Only a list can be concatenated to a Query")
        return self.run() + operand

    def __iter__(self):
        """ Iterate over (cached) query result. """
        return iter(self.run())

    def __getitem__(self, idx):
        return self.run()[idx]

    def __len__(self):
        return len(self.run())

    def count(self):
        return len(self)

    @property
    def result(self):
        assert((self._result is None) or self.evaluated)
        if not self._result and self.evaluated:
            self._result = super(Query, self).run()
        return self._result

    def run(self):
        """ Execute the query and cache the result.

            This does not force re-evaluation unless neccessary, run() checks
            that.
        """
        self._result = super(Query, self).run()
        return self._result

    def filter(self, *lst, **kwargs):
        new_query = type(self)(query=self)
        return new_query.filterm(*lst, **kwargs)

    def filterm(self, *lst, **kwargs):
        self._result = None
        flags = set(lst)
        for arg_tuple in _parse_filter_args(flags, kwargs):
            super(Query, self).filter(*arg_tuple)
        return self

    def provides(self, name, **kwargs):
        raise NotImplementedError("hawkey.Query.provides is not implemented yet")

class Selector(_hawkey.Selector):
    def set(self, **kwargs):
        for arg_tuple in _parse_filter_args(set(), kwargs):
            super(Selector, self).set(*arg_tuple)
        return self

FORM_NEVRA	= re.compile("""(?P<name>[.\-S]+)-\
(?P<epoch>E)?(?P<version>[.S]+)-\
(?P<release>[.S]+)\.(?P<arch>S)$""")
FORM_NEVR	= re.compile("""(?P<name>[.\-S]+)-\
(?P<epoch>E)?(?P<version>[.S]+)-(?P<release>[.S]+)$""")
FORM_NEV	= re.compile("""(?P<name>[.\-S]+)-\
(?P<epoch>E)?(?P<version>[.S]+)$""")
FORM_NA		= re.compile("""(?P<name>[.\-S]+)\.(?P<arch>S)$""")
FORM_NAME	= re.compile("""(?P<name>[.\-S]+)$""")

# most specific to least
FORMS_MOST_SPEC	= [FORM_NEVRA, FORM_NEVR, FORM_NEV, FORM_NA, FORM_NAME]
# what the user most probably means
FORMS_REAL	= [FORM_NA, FORM_NAME, FORM_NEVRA, FORM_NEV, FORM_NEVR]

def _is_glob_pattern(pattern):
    return set(pattern) & set("*[?")

class _Token(object):
    def __init__(self, content, epoch=False):
        self.content = content
        self.epoch = epoch

    def __repr__(self):
        if self.epoch:
            return "<%s:>" % self.content
        elif self.content == '.':
            return "<.>"
        elif self.content == '-':
            return "<->"
        else:
            return self.content

    def __str__(self):
        if self.epoch:
            return "%s:" % self.content
        return self.content

    def abbr(self):
        if self.epoch:
            return "E"
        elif self.content in ('.', '-'):
            return self.content
        else:
            return "S"

class Subject(object):
    def __init__(self, pattern):
        self.pat = pattern
        self._tokenize()

    @property
    def _abbr(self):
        return "".join(map(operator.methodcaller('abbr'), self.tokens))

    @staticmethod
    def _listify_form(form):
        if type(form) is type(FORM_NEVRA):
            return [form]
        else:
            return form[:]

    @staticmethod
    def _is_int(s):
        return Subject._to_int(s) is not None

    @staticmethod
    def _throw_tokens(pattern):
        current = ""
        for c in pattern:
            if c in (".", "-"):
                yield _Token(current)
                yield _Token(c)
                current = ""
                continue
            if c == ":" and Subject._is_int(current):
                yield _Token(current, epoch=True)
                current = ""
                continue
            current += c
        yield _Token(current)

    @staticmethod
    def _to_int(s):
        try:
            return int(s)
        except (ValueError, TypeError):
            return None

    def _backmap(self, abbr, match, group):
        start = match.start(group)
        end = match.end(group)
        merged = "".join(map(str, self.tokens[start:end]))
        if group == 'epoch':
            return self._to_int(merged[:-1])
        return merged

    def _tokenize(self):
        self.tokens = list(self._throw_tokens(self.pat))

    @property
    def pattern(self):
        return self.pat

    default_nevra=NEVRA(name=None, epoch=None, version=None, release=None,
                        arch=None)

    def nevra_possibilities(self, form=FORMS_MOST_SPEC):
        abbr = self._abbr
        forms = self._listify_form(form)
        for pat in forms:
            match = pat.match(abbr)
            if match is None:
                continue
            yield self.default_nevra._replace(**
                {key:self._backmap(abbr, match, key)
                 for key in match.groupdict()})

    def nevra_possibilities_real(self, sack, allow_globs=False, icase=False,
                                 form=FORMS_REAL):
        def should_check_arch(val):
            if val is None:
                return False
            if allow_globs and _is_glob_pattern(val):
                # filtering by a globbed arch is not supported by sack._knows()
                return False
            return True

        def filter_version(version):
            if version is None:
                return None
            if allow_globs and _is_glob_pattern(version):
                # filtering by a globbed version is not supported by sack._knows()
                return None
            return version

        existing_arches = sack.list_arches()
        existing_arches.append('src')
        for nevra in self.nevra_possibilities(form=form):
            if nevra.name is not None:
                name = nevra.name
                # do not enable globbing unless necessary, it gets expensive
                glob = bool(allow_globs and _is_glob_pattern(name))
                version = filter_version(nevra.version)
                if not sack._knows(name, version, name_only=True,
                                   icase=icase, glob=glob):
                    continue

            if should_check_arch(nevra.arch):
                if nevra.arch not in existing_arches:
                    continue
            yield nevra

    def reldep_possibilities_real(self, sack, icase=False):
        abbr = self._abbr
        if FORM_NAME.match(abbr):
            if sack._knows(self.pat, icase=icase):
                yield Reldep(sack, self.pat)
