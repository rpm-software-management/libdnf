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

import collections
import functools
import gi
import logging
import operator
import time
import warnings

gi.require_version('Dnf', '1.0')
from gi.repository import Dnf

from . import _hawkey

__all__ = [
    # version info
    'VERSION', 'VERSION_MAJOR', 'VERSION_MINOR', 'VERSION_PATCH',
    # submodules
    'test',
    # constants
    'CHKSUM_MD5', 'CHKSUM_SHA1', 'CHKSUM_SHA256', 'CHKSUM_SHA512', 'ICASE',
    'CMDLINE_REPO_NAME', 'SYSTEM_REPO_NAME', 'REASON_DEP', 'REASON_USER',
    'REASON_CLEAN', 'REASON_WEAKDEP', 'FORM_NEVRA', 'FORM_NEVR', 'FORM_NEV',
    'FORM_NA', 'FORM_NAME', 'FORM_ALL', 'MODULE_FORM_NSVCAP', 'MODULE_FORM_NSVCA',
    'MODULE_FORM_NSVAP', 'MODULE_FORM_NSVA', 'MODULE_FORM_NSAP', 'MODULE_FORM_NSA',
    'MODULE_FORM_NSVCP', 'MODULE_FORM_NSVP', 'MODULE_FORM_NSVC', 'MODULE_FORM_NSV',
    'MODULE_FORM_NSP', 'MODULE_FORM_NS', 'MODULE_FORM_NAP', 'MODULE_FORM_NA',
    'MODULE_FORM_NP', 'MODULE_FORM_N'
    # exceptions
    'ArchException', 'Exception', 'QueryException', 'RuntimeException',
    'ValueException',
    # functions
    'chksum_name', 'chksum_type', 'split_nevra', 'convert_reason',
    # gi classes
    'Swdb', 'SwdbItem', 'SwdbReason', 'SwdbPkg', 'SwdbPkgData', 'SwdbTrans',
    'SwdbGroup', 'SwdbEnv',
    # classes
    'Goal', 'NEVRA', 'ModuleForm', 'Package', 'Query', 'Repo', 'Sack', 'Selector', 'Subject']

Swdb = Dnf.Swdb
SwdbItem = Dnf.SwdbItem
SwdbReason = Dnf.SwdbReason
SwdbPkg = Dnf.SwdbPkg
SwdbPkgData = Dnf.SwdbPkgData
SwdbTrans = Dnf.SwdbTrans
SwdbGroup = Dnf.SwdbGroup
SwdbEnv = Dnf.SwdbEnv


_QUERY_KEYNAME_MAP = {
    'pkg': _hawkey.PKG,
    'advisory': _hawkey.PKG_ADVISORY,
    'advisory_bug': _hawkey.PKG_ADVISORY_BUG,
    'advisory_cve': _hawkey.PKG_ADVISORY_CVE,
    'advisory_severity': _hawkey.PKG_ADVISORY_SEVERITY,
    'advisory_type': _hawkey.PKG_ADVISORY_TYPE,
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

MODULE_FORM_NSVCAP = _hawkey.MODULE_FORM_NSVCAP
MODULE_FORM_NSVCA = _hawkey.MODULE_FORM_NSVCA
MODULE_FORM_NSVAP = _hawkey.MODULE_FORM_NSVAP
MODULE_FORM_NSVA = _hawkey.MODULE_FORM_NSVA
MODULE_FORM_NSAP = _hawkey.MODULE_FORM_NSAP
MODULE_FORM_NSA = _hawkey.MODULE_FORM_NSA
MODULE_FORM_NSVCP = _hawkey.MODULE_FORM_NSVCP
MODULE_FORM_NSVP = _hawkey.MODULE_FORM_NSVP
MODULE_FORM_NSVC = _hawkey.MODULE_FORM_NSVC
MODULE_FORM_NSV = _hawkey.MODULE_FORM_NSV
MODULE_FORM_NSP = _hawkey.MODULE_FORM_NSP
MODULE_FORM_NS = _hawkey.MODULE_FORM_NS
MODULE_FORM_NAP = _hawkey.MODULE_FORM_NAP
MODULE_FORM_NA = _hawkey.MODULE_FORM_NA
MODULE_FORM_NP = _hawkey.MODULE_FORM_NP
MODULE_FORM_N = _hawkey.MODULE_FORM_N

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
REASON_CLEAN = _hawkey.REASON_CLEAN
REASON_WEAKDEP = _hawkey.REASON_WEAKDEP

ADVISORY_UNKNOWN = _hawkey.ADVISORY_UNKNOWN
ADVISORY_SECURITY = _hawkey.ADVISORY_SECURITY
ADVISORY_BUGFIX = _hawkey.ADVISORY_BUGFIX
ADVISORY_ENHANCEMENT = _hawkey.ADVISORY_ENHANCEMENT
ADVISORY_NEWPACKAGE = _hawkey.ADVISORY_NEWPACKAGE

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

ALLOW_UNINSTALL = _hawkey.ALLOW_UNINSTALL
FORCE_BEST = _hawkey.FORCE_BEST
VERIFY = _hawkey.VERIFY
IGNORE_WEAK_DEPS = _hawkey.IGNORE_WEAK_DEPS

SOLUTION_ALLOW_INSTALL = _hawkey.SOLUTION_ALLOW_INSTALL
SOLUTION_ALLOW_REINSTALL = _hawkey.SOLUTION_ALLOW_REINSTALL
SOLUTION_ALLOW_UPGRADE = _hawkey.SOLUTION_ALLOW_UPGRADE
SOLUTION_ALLOW_DOWNGRADE = _hawkey.SOLUTION_ALLOW_DOWNGRADE
SOLUTION_ALLOW_CHANGE = _hawkey.SOLUTION_ALLOW_CHANGE
SOLUTION_ALLOW_OBSOLETE = _hawkey.SOLUTION_ALLOW_OBSOLETE
SOLUTION_ALLOW_REPLACEMENT = _hawkey.SOLUTION_ALLOW_REPLACEMENT
SOLUTION_ALLOW_REMOVE = _hawkey.SOLUTION_ALLOW_REMOVE
SOLUTION_DO_NOT_INSTALL = _hawkey.SOLUTION_DO_NOT_INSTALL
SOLUTION_DO_NOT_REMOVE = _hawkey.SOLUTION_DO_NOT_REMOVE
SOLUTION_DO_NOT_OBSOLETE = _hawkey.SOLUTION_DO_NOT_OBSOLETE
SOLUTION_DO_NOT_UPGRADE = _hawkey.SOLUTION_DO_NOT_UPGRADE
SOLUTION_BAD_SOLUTION = _hawkey.SOLUTION_BAD_SOLUTION

PY3 = python_version.major >= 3

logger = logging.getLogger('dnf')

def convert_reason(reason):
    if isinstance(reason, Dnf.SwdbReason):
        return reason
    return Dnf.convert_reason_to_id(reason)

def split_nevra(s):
    t = _hawkey.split_nevra(s)
    return NEVRA(*t)


class NEVRA(_hawkey.NEVRA):

    def to_query(self, sack, icase=False):
        _hawkey_query = super(NEVRA, self).to_query(sack, icase=icase)
        return Query(query=_hawkey_query)


class ModuleForm(_hawkey.ModuleForm):

    MODULE_FORM_FIELDS = ["name", "stream", "version", "context", "arch", "profile"]

    def _has_just_name(self):
        return self.name and not self.stream and not self.version and \
               not self.arch and not self.profile

    def __repr__(self):
        values = [getattr(self, i) for i in self.MODULE_FORM_FIELDS]
        items = [(field, value) for field, value in zip(self.MODULE_FORM_FIELDS, values) if value is not None]
        items_str = ", ".join(["{}={}".format(field, value) for field, value in items])
        return "<MODULE_FORM: {}>".format(items_str)

    def __eq__(self, other):
        result = True
        for field in self.MODULE_FORM_FIELDS:
            value_self = getattr(self, field)
            value_other = getattr(other, field)
            result &= value_self == value_other
        return result


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

    def __init__(self, sack):
        super(Goal, self).__init__(sack)
        self.group_members = set()
        self._installs = []

    def get_reason(self, pkg):
        code = super(Goal, self).get_reason(pkg)
        if code == REASON_USER and pkg.name in self.group_members:
            return Dnf.SwdbReason.GROUP
        return Dnf.SwdbReason(code)

    def group_reason(self, pkg, current_reason):
        if current_reason == Dnf.SwdbReason.UNKNOWN and pkg.name in self.group_members:
            return Dnf.SwdbReason.GROUP
        return current_reason

    def push_userinstalled(self, query, history):
        msg = '--> Finding unneeded leftover dependencies' # translate
        logger.debug(msg)
        pkgs = query.installed()

        # get only user installed packages
        user_installed = history.select_user_installed(pkgs)

        for pkg in user_installed:
            self.userinstalled(pkg)

    def available_updates_diff(self, query):
        available_updates = set(query.upgrades().filter(arch__neq="src")
                                .latest().run())
        installable_updates = set(self.list_upgrades())
        installs = set(self.list_installs())
        return (available_updates - installable_updates) - installs

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
        if args:
            self._installs.extend(args)
        if 'select' in kwargs:
            self._installs.extend(kwargs['select'].matches())
        super(Goal, self).install(*args, **kwargs)

    @_auto_selector
    def downgrade_to(self, *args, **kwargs):
        super(Goal, self).downgrade_to(*args, **kwargs)

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
    if not PY3 and isinstance(obj, unicode):
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
        elif not PY3 and isinstance(match, basestring):
            match = _encode(match)
        elif PY3 and isinstance(match, str):
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


def is_glob_pattern(pattern):
    if (not PY3 and isinstance(pattern, basestring)) or \
            (PY3 and isinstance(pattern, str)):
        pattern = [pattern]
    return (isinstance(pattern, list) and any(set(p) & set("*[?") for p in pattern))

def _msg_installed(pkg):
    logger.warning('Package {} is already installed, skipping.'.format(str(pkg))) # translate

class Query(_hawkey.Query):

    def __init__(self, sack=None, query=None):
        super(Query, self).__init__(sack=sack, query=query)

    def __add__(self, operand):
        if not isinstance(operand, list):
            raise TypeError("Only a list can be concatenated to a Query")
        return self.run() + operand

    def difference(self, other):
        new_query = type(self)(query=self)
        return super(Query, new_query).difference(other)

    def intersection(self, other):
        new_query = type(self)(query=self)
        return super(Query, new_query).intersection(other)

    def union(self, other):
        new_query = type(self)(query=self)
        return super(Query, new_query).union(other)

    def _unneeded(self, sack, history, debug_solver=False):
        goal = Goal(sack)
        goal.push_userinstalled(self.installed(), history)
        solved = goal.run()
        if debug_solver:
            goal.write_debugdata('./debugdata-autoremove')
        assert solved
        unneeded = goal.list_unneeded()
        return self.filter(pkg=unneeded)

    def duplicated(self):
        # :api
        installed_name = self.installed()._name_dict()
        duplicated = []
        for name, pkgs in installed_name.items():
            if len(pkgs) > 1:
                for x in range(0, len(pkgs)):
                    dups = False
                    for y in range(x+1, len(pkgs)):
                        if not ((pkgs[x].evr_cmp(pkgs[y]) == 0)
                                and (pkgs[x].arch != pkgs[y].arch)):
                            duplicated.append(pkgs[y])
                            dups = True
                    if dups:
                        duplicated.append(pkgs[x])
        return self.filter(pkg=duplicated)

    def extras(self):
        # :api
        # anything installed but not in a repo is an extra
        avail_dict = self.available()._pkgtup_dict()
        inst_dict = self.installed()._pkgtup_dict()
        extras = []
        for pkgtup, pkgs in inst_dict.items():
            if pkgtup not in avail_dict:
                extras.extend(pkgs)
        return self.filter(pkg=extras)

    def latest(self, limit=1):
        # :api
        if limit == 1:
            return self.filter(latest_per_arch=True)
        else:
            pkgs_na = self._na_dict()
            latest_pkgs = []
            for pkg_list in pkgs_na.values():
                pkg_list.sort(reverse=True)
                if limit > 0:
                    latest_pkgs.extend(pkg_list[0:limit])
                else:
                    latest_pkgs.extend(pkg_list[-limit:])
            return self.filter(pkg=latest_pkgs)

    def _na_dict(self):
        d = {}
        for pkg in self.run():
            key = (pkg.name, pkg.arch)
            d.setdefault(key, []).append(pkg)
        return d

    def _pkgtup_dict(self):
        d = {}
        for pkg in self.run():
            d.setdefault(pkg.pkgtup, []).append(pkg)
        return d

    def _recent(self, recent):
        now = time.time()
        recentlimit = now - (recent*86400)
        recent = [po for po in self if int(po.buildtime) > recentlimit]
        return self.filter(pkg=recent)

    def _nevra(self, *args):
        args_len = len(args)
        if args_len == 3:
            return self.filter(name=args[0], evr=args[1], arch=args[2])
        if args_len == 1:
            nevra = split_nevra(args[0])
        elif args_len == 5:
            nevra = args
        else:
            raise TypeError("nevra() takes 1, 3 or 5 str params")
        return self.filter(
            name=nevra.name, epoch=nevra.epoch, version=nevra.version,
            release=nevra.release, arch=nevra.arch)


class Selector(_hawkey.Selector):

    def set(self, **kwargs):
        for arg_tuple in _parse_filter_args(set(), kwargs):
            super(Selector, self).set(*arg_tuple)
        return self

    def _set_autoglob(self, **kwargs):
        nargs = {}
        for (key, value) in kwargs.items():
            if is_glob_pattern(value):
                nargs[key + "__glob"] = value
            else:
                nargs[key] = value
        return self.set(**nargs)


class Subject(_hawkey.Subject):

    def __init__(self, pkg_spec, ignore_case=False):
        self.icase = ignore_case
        super(Subject, self).__init__(pkg_spec)

    def nevra_possibilities_real(self, *args, **kwargs):
        warnings.simplefilter('always', DeprecationWarning)
        msg = "The function 'nevra_possibilities_real' is deprecated. " \
              "Please use 'get_nevra_possibilities' instead. The function will be removed on 2018-01-01"
        warnings.warn(msg, DeprecationWarning)

        poss = super(Subject, self).nevra_possibilities_real(*args, **kwargs)
        for nevra in poss:
            yield NEVRA(nevra=nevra)

    def module_form_possibilities(self, *args, **kwargs):
        poss = super(Subject, self).module_form_possibilities(*args, **kwargs)
        for module_form in poss:
            yield ModuleForm(module_form=module_form)

    @property
    def _filename_pattern(self):
        return self.pattern.startswith('/') or self.pattern.startswith('*/')

    def _is_arch_specified(self, solution):
        if solution['nevra'] and solution['nevra'].arch:
            return is_glob_pattern(solution['nevra'].arch)
        return False

    def get_nevra_possibilities(self, forms=None):
        # :api
        """
        :param forms: list of hawkey NEVRA forms like [hawkey.FORM_NEVRA, hawkey.FORM_NEVR]
        :return: generator for every possible nevra. Each possible nevra is represented by Class
        NEVRA object (libdnf) that have attributes name, epoch, version, release, arch
        """
        kwargs = dict()
        if forms:
            kwargs['form'] = forms
        for nevra in super(Subject, self).nevra_possibilities(**kwargs):
            yield NEVRA(nevra=nevra)


    def _get_nevra_solution(self, sack, with_nevra=True, with_provides=True, with_filenames=True,
                            forms=None):
        """
        Try to find first real solution for subject if it is NEVRA
        @param sack:
        @param forms:
        @return: dict with keys nevra and query
        """
        kwargs = {}
        if forms:
            kwargs['form'] = forms
        solution = self.get_best_solution(sack, icase=self.icase, with_nevra=with_nevra,
                                          with_provides=with_provides,
                                          with_filenames=with_filenames, **kwargs)
        solution['query'] = Query(query=solution['query'])
        return solution

    def get_best_query(self, sack, with_nevra=True, with_provides=True, with_filenames=True,
                       forms=None):
        # :api

        solution = self._get_nevra_solution(sack, with_nevra=with_nevra,
                                            with_provides=with_provides,
                                            with_filenames=with_filenames,
                                            forms=forms)
        return solution['query']

    def get_best_selector(self, sack, forms=None, obsoletes=True, reponame=None, reports=False):
        # :api
        if reports:
            warnings.simplefilter('always', DeprecationWarning)
            msg = "The attribute 'reports' is deprecated and not used any more. "\
                  "This attribute will be removed on 2018-01-01"
            warnings.warn(msg, DeprecationWarning)

        solution = self._get_nevra_solution(sack, forms=forms)
        if solution['query']:
            q = solution['query']
            q = q.filter(arch__neq="src")
            if obsoletes and solution['nevra'] and solution['nevra'].has_just_name():
                q = q.union(sack.query().filter(obsoletes=q))
            installed_query = q.installed()
            if reponame:
                q = q.filter(reponame=reponame).union(installed_query)
            if q:
                return self._list_or_query_to_selector(sack, q)

        return Selector(sack)

    def _get_best_selectors(self, base, forms=None, obsoletes=True, reponame=None, reports=False,
                            solution=None):
        if solution is None:
            solution = self._get_nevra_solution(base.sack, forms=forms)
        q = solution['query']
        q = q.filter(arch__neq="src")
        if len(q) == 0:
            if reports and not self.icase:
                base._report_icase_hint(self.pattern)
            return []
        q = self._apply_security_filters(q, base)
        if not q:
            return []

        if not self._filename_pattern and is_glob_pattern(self.pattern) \
                or solution['nevra'] and solution['nevra'].name is None:
            with_obsoletes = False

            if obsoletes and solution['nevra'] and solution['nevra'].has_just_name():
                with_obsoletes = True
            installed_query = q.installed()
            if reponame:
                q = q.filter(reponame=reponame)
            available_query = q.available()
            installed_relevant_query = installed_query.filter(
                name=[pkg.name for pkg in available_query])
            if reports:
                self._report_installed(installed_relevant_query)
            q = available_query.union(installed_relevant_query)
            sltrs = []
            for name, pkgs_list in q._name_dict().items():
                if with_obsoletes:
                    pkgs_list = pkgs_list + base.sack.query().filter(
                        obsoletes=pkgs_list).run()
                sltrs.append(self._list_or_query_to_selector(base.sack, pkgs_list))
            return sltrs
        else:
            if obsoletes and solution['nevra'] and solution['nevra'].has_just_name():
                q = q.union(base.sack.query().filter(obsoletes=q))
            installed_query = q.installed()

            if reports:
                self._report_installed(installed_query)
            if reponame:
                q = q.filter(reponame=reponame).union(installed_query)
            if not q:
                return []

            return [self._list_or_query_to_selector(base.sack, q)]

    def _apply_security_filters(self, query, base):
        query = base._merge_update_filters(query, warning=False)
        if not query:
            logger.warning('No security updates for argument "{}"'.format(self.pattern)) # translate
        return query

    @staticmethod
    def _report_installed(iterable_packages):
        for pkg in iterable_packages:
            _msg_installed(pkg)

    @staticmethod
    def _list_or_query_to_selector(sack, list_or_query):
        sltr = Selector(sack)
        return sltr.set(pkg=list_or_query)
