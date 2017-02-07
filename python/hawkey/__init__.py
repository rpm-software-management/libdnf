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
import logging
import operator
import time
import warnings

import libdnf.transaction

from . import _hawkey

__all__ = [
    # version info
    'VERSION', 'VERSION_MAJOR', 'VERSION_MINOR', 'VERSION_PATCH',
    # submodules
    'test',
    # constants
    'CHKSUM_MD5', 'CHKSUM_SHA1', 'CHKSUM_SHA256', 'CHKSUM_SHA384', 'CHKSUM_SHA512',
    'ICASE', 'CMDLINE_REPO_NAME', 'SYSTEM_REPO_NAME', 'REASON_DEP', 'REASON_USER',
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
    'chksum_name', 'chksum_type', 'split_nevra', 'convert_hawkey_reason',
    # classes
    'Goal', 'NEVRA', 'NSVCAP', 'Package', 'Query', 'Repo', 'Sack', 'Selector', 'Subject']

NEVRA = _hawkey.NEVRA
Query = _hawkey.Query
Selector = _hawkey.Selector

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

IGNORE_EXCLUDES = _hawkey.IGNORE_EXCLUDES

CHKSUM_MD5 = _hawkey.CHKSUM_MD5
CHKSUM_SHA1 = _hawkey.CHKSUM_SHA1
CHKSUM_SHA256 = _hawkey.CHKSUM_SHA256
CHKSUM_SHA384 = _hawkey.CHKSUM_SHA384
CHKSUM_SHA512 = _hawkey.CHKSUM_SHA512

REASON_DEP = _hawkey.REASON_DEP
REASON_USER = _hawkey.REASON_USER
REASON_CLEAN = _hawkey.REASON_CLEAN
REASON_WEAKDEP = _hawkey.REASON_WEAKDEP

def convert_hawkey_reason(hawkey_reason):
    if hawkey_reason == REASON_USER:
        return libdnf.transaction.TransactionItemReason_USER
    if hawkey_reason == REASON_DEP:
        return libdnf.transaction.TransactionItemReason_DEPENDENCY
    if hawkey_reason == REASON_CLEAN:
        return libdnf.transaction.TransactionItemReason_CLEAN
    if hawkey_reason == REASON_WEAKDEP:
        return libdnf.transaction.TransactionItemReason_WEAK_DEPENDENCY
    return libdnf.transaction.TransactionItemReason_UNKNOWN

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

PY3 = python_version.major >= 3

logger = logging.getLogger('dnf')

def split_nevra(s):
    t = _hawkey.split_nevra(s)
    return NEVRA(*t)


class NSVCAP(_hawkey.NSVCAP):

    NSVCAP_FIELDS = ["name", "stream", "version", "context", "arch", "profile"]

    def _has_just_name(self):
        return self.name and not self.stream and not self.version and \
               not self.arch and not self.profile

    def __repr__(self):
        values = [getattr(self, i) for i in self.NSVCAP_FIELDS]
        items = [(field, value) for field, value in zip(self.NSVCAP_FIELDS, values) if value is not None]
        items_str = ", ".join(["{}={}".format(field, value) for field, value in items])
        return "<NSVCAP: {}>".format(items_str)

    def __eq__(self, other):
        result = True
        for field in self.NSVCAP_FIELDS:
            value_self = getattr(self, field)
            value_other = getattr(other, field)
            result &= value_self == value_other
        return result


class Goal(_hawkey.Goal):
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

    def get_reason(self, pkg):
        code = super(Goal, self).get_reason(pkg)
        if code == REASON_USER and pkg.name in self.group_members:
            return libdnf.transaction.TransactionItemReason_GROUP
        return convert_hawkey_reason(code)

    def group_reason(self, pkg, current_reason):
        if current_reason == libdnf.transaction.TransactionItemReason_UNKNOWN and pkg.name in self.group_members:
            return libdnf.transaction.TransactionItemReason_GROUP
        return current_reason

    def push_userinstalled(self, query, history):
        msg = '--> Finding unneeded leftover dependencies' # translate
        logger.debug(msg)

        # get only user installed packages
        user_installed = query.userinstalled(history.swdb)
        self.userinstalled(user_installed)


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


def is_glob_pattern(pattern):
    if (not PY3 and isinstance(pattern, basestring)) or \
            (PY3 and isinstance(pattern, str)):
        pattern = [pattern]
    return (isinstance(pattern, list) and any(set(p) & set("*[?") for p in pattern))


class Subject(_hawkey.Subject):

    def __init__(self, pkg_spec, ignore_case=False):
        super(Subject, self).__init__(pkg_spec, ignore_case=ignore_case)

    def nsvcap_possibilities(self, *args, **kwargs):
        poss = super(Subject, self).nsvcap_possibilities(*args, **kwargs)
        for nsvcap in poss:
            yield NSVCAP(nsvcap=nsvcap)

    @property
    def _filename_pattern(self):
        return self.pattern.startswith('/') or self.pattern.startswith('*/')

    def _is_arch_specified(self, solution):
        if solution['nevra'] and solution['nevra'].arch:
            return is_glob_pattern(solution['nevra'].arch)
        return False

    def nevra_possibilities(self, form=None):
        warnings.simplefilter('always', DeprecationWarning)
        msg = "The function 'nevra_possibilities' is deprecated. " \
              "Please use 'get_nevra_possibilities' instead. The function will be removed on " \
              "2018-01-01"
        warnings.warn(msg, DeprecationWarning)
        for nevra in super(Subject, self).get_nevra_possibilities(forms=form):
            yield NEVRA(nevra=nevra)

    def _get_best_selectors(self, base, forms=None, obsoletes=True, reponame=None, reports=False,
                            solution=None):
        if solution is None:
            solution = self.get_best_solution(base.sack, forms=forms, with_src=False)
        q = solution['query']
        if len(q) == 0:
            return []
        q = self._apply_security_filters(q, base)
        if not q:
            # we don't report the exact reason why any selector returned - reasons can be only src
            # found, no package or not in requested repository. We should improve it in libdnf
            # after movement of base.install() or base.distro_sync()
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
                base._report_already_installed(installed_relevant_query)
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
                base._report_already_installed(installed_query)
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
    def _list_or_query_to_selector(sack, list_or_query):
        sltr = Selector(sack)
        return sltr.set(pkg=list_or_query)
