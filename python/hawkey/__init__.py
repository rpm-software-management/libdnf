#
# Copyright (C) 2012-2014 Red Hat, Inc.
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
import functools
import operator

__all__ = [
    # version info
    'VERSION', 'VERSION_MAJOR', 'VERSION_MINOR', 'VERSION_PATCH',
    # submodules
    'test',
    # constants
    'CHKSUM_MD5', 'CHKSUM_SHA1', 'CHKSUM_SHA256', 'CHKSUM_SHA512', 'ICASE',
    'CMDLINE_REPO_NAME', 'SYSTEM_REPO_NAME', 'REASON_DEP', 'REASON_USER',
    'FORM_NEVRA', 'FORM_NEVR', 'FORM_NEV', 'FORM_NA', 'FORM_NAME', 'FORM_ALL',
    # exceptions
    'ArchException', 'Exception', 'QueryException', 'RuntimeException',
    'ValueException',
    # functions
    'chksum_name', 'chksum_type', 'split_nevra',
    # classes
    'Goal', 'NEVRA', 'Package', 'Query', 'Repo', 'Sack', 'Selector', 'Subject']

_QUERY_KEYNAME_MAP = {
    'pkg': _hawkey.PKG,
    'arch': _hawkey.PKG_ARCH,
    'conflicts': _hawkey.PKG_CONFLICTS,
    'description': _hawkey.PKG_DESCRIPTION,
    'downgradable': _hawkey.PKG_DOWNGRADABLE,
    'downgrades': _hawkey.PKG_DOWNGRADES,
    'empty': _hawkey.PKG_EMPTY,
    'enhances': _hawkey.PKG_ENHANCES,
    'epoch': _hawkey.PKG_EPOCH,
    'evr': _hawkey.PKG_EVR,
    'file': _hawkey.PKG_FILE,
    'latest': _hawkey.PKG_LATEST,
    'latest_per_arch': _hawkey.PKG_LATEST_PER_ARCH,
    'location': _hawkey.PKG_LOCATION,
    'name': _hawkey.PKG_NAME,
    'nevra': _hawkey.PKG_NEVRA,
    'obsoletes': _hawkey.PKG_OBSOLETES,
    'provides': _hawkey.PKG_PROVIDES,
    'recommends': _hawkey.PKG_RECOMMENDS,
    'release': _hawkey.PKG_RELEASE,
    'reponame': _hawkey.PKG_REPONAME,
    'requires': _hawkey.PKG_REQUIRES,
    'sourcerpm': _hawkey.PKG_SOURCERPM,
    'suggests': _hawkey.PKG_SUGGESTS,
    'summary': _hawkey.PKG_SUMMARY,
    'supplements': _hawkey.PKG_SUPPLEMENTS,
    'upgradable': _hawkey.PKG_UPGRADABLE,
    'upgrades': _hawkey.PKG_UPGRADES,
    'url': _hawkey.PKG_URL,
    'version': _hawkey.PKG_VERSION,
}

_CMP_MAP = {
    '=': _hawkey.EQ,
    '>': _hawkey.GT,
    '<': _hawkey.LT,
    '!=': _hawkey.NEQ,
    '>=': _hawkey.EQ | _hawkey.GT,
    '<=': _hawkey.EQ | _hawkey.LT,
}

_QUERY_CMP_MAP = {
    'eq': _hawkey.EQ,
    'gt': _hawkey.GT,
    'lt': _hawkey.LT,
    'neq': _hawkey.NEQ,
    'not': _hawkey.NOT,
    'gte': _hawkey.EQ | _hawkey.GT,
    'lte': _hawkey.EQ | _hawkey.LT,
    'substr': _hawkey.SUBSTR,
    'glob': _hawkey.GLOB,
}

VERSION_MAJOR = _hawkey.VERSION_MAJOR
VERSION_MINOR = _hawkey.VERSION_MINOR
VERSION_PATCH = _hawkey.VERSION_PATCH
VERSION = u"%d.%d.%d" % (VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)

SYSTEM_REPO_NAME = _hawkey.SYSTEM_REPO_NAME
CMDLINE_REPO_NAME = _hawkey.CMDLINE_REPO_NAME

FORM_NEVRA = _hawkey.FORM_NEVRA
FORM_NEVR = _hawkey.FORM_NEVR
FORM_NEV = _hawkey.FORM_NEV
FORM_NA = _hawkey.FORM_NA
FORM_NAME = _hawkey.FORM_NAME

ICASE = _hawkey.ICASE
EQ = _hawkey.EQ
LT = _hawkey.LT
GT = _hawkey.GT

CHKSUM_MD5 = _hawkey.CHKSUM_MD5
CHKSUM_SHA1 = _hawkey.CHKSUM_SHA1
CHKSUM_SHA256 = _hawkey.CHKSUM_SHA256
CHKSUM_SHA512 = _hawkey.CHKSUM_SHA512

REASON_DEP = _hawkey.REASON_DEP
REASON_USER = _hawkey.REASON_USER

ADVISORY_UNKNOWN = _hawkey.ADVISORY_UNKNOWN
ADVISORY_SECURITY = _hawkey.ADVISORY_SECURITY
ADVISORY_BUGFIX = _hawkey.ADVISORY_BUGFIX
ADVISORY_ENHANCEMENT = _hawkey.ADVISORY_ENHANCEMENT

REFERENCE_UNKNOWN = _hawkey.REFERENCE_UNKNOWN
REFERENCE_BUGZILLA = _hawkey.REFERENCE_BUGZILLA
REFERENCE_CVE = _hawkey.REFERENCE_CVE
REFERENCE_VENDOR = _hawkey.REFERENCE_VENDOR

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
detect_arch = _hawkey.detect_arch

ERASE = _hawkey.ERASE
DISTUPGRADE = _hawkey.DISTUPGRADE
DISTUPGRADE_ALL = _hawkey.DISTUPGRADE_ALL
DOWNGRADE = _hawkey.DOWNGRADE
INSTALL = _hawkey.INSTALL
UPGRADE = _hawkey.UPGRADE
UPGRADE_ALL = _hawkey.UPGRADE_ALL


def split_nevra(s):
    t = _hawkey.split_nevra(s)
    return NEVRA(*t)


class NEVRA(_hawkey.NEVRA):

    def to_query(self, sack):
        _hawkey_query = super(NEVRA, self).to_query(sack)
        return Query(query=_hawkey_query)


class Goal(_hawkey.Goal):
    _reserved_kw = set(['package', 'select'])
    _flag_kw = set(['clean_deps', 'check_installed'])
    _goal_actions = {
        ERASE,
        DISTUPGRADE,
        DISTUPGRADE_ALL,
        DOWNGRADE,
        INSTALL,
        UPGRADE,
        UPGRADE_ALL
    }

    @property
    def actions(self):
        return {f for f in self._goal_actions if self._has_actions(f)}

    def _auto_selector(fn):
        def tweaked_fn(self, *args, **kwargs):
            if args or self._reserved_kw & set(kwargs):
                return fn(self, *args, **kwargs)

            # only the flags and unrecognized keywords remained, construct a
            # Selector
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
        split = k.split("__")
        keyname = split[0]
        if len(split) == 1:
            cmp_types = ["eq"]
        else:
            cmp_types = split[1:]
        if not keyname in _QUERY_KEYNAME_MAP:
            raise ValueException("Unrecognized key name: %s" % keyname)
        for cmp_type in cmp_types:
            if not cmp_type in _QUERY_CMP_MAP:
                raise ValueException("Unrecognized filter type: %s" % cmp_type)

        mapped_types = map(_QUERY_CMP_MAP.__getitem__, cmp_types)
        item_flags = functools.reduce(operator.or_, mapped_types, filter_flags)
        args.append((_QUERY_KEYNAME_MAP[keyname],
                     item_flags,
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
        raise NotImplementedError(
            "hawkey.Query.provides is not implemented yet")


class Selector(_hawkey.Selector):

    def set(self, **kwargs):
        for arg_tuple in _parse_filter_args(set(), kwargs):
            super(Selector, self).set(*arg_tuple)
        return self


class Subject(_hawkey.Subject):

    def __init__(self, *args, **kwargs):
        super(Subject, self).__init__(*args, **kwargs)

    def nevra_possibilities(self, *args, **kwargs):
        for nevra in super(Subject, self).nevra_possibilities(*args, **kwargs):
            yield NEVRA(nevra=nevra)

    def nevra_possibilities_real(self, *args, **kwargs):
        poss = super(Subject, self).nevra_possibilities_real(*args, **kwargs)
        for nevra in poss:
            yield NEVRA(nevra=nevra)
