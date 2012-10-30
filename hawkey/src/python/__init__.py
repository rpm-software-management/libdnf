import _hawkey
import collections
import types

__all__ = [
    # version info
    'VERSION', 'VERSION_MAJOR', 'VERSION_MINOR', 'VERSION_PATCH',
    # submodules
    'test',
    # constants
    'CHKSUM_MD5', 'CHKSUM_SHA1', 'CHKSUM_SHA256', 'CMDLINE_REPO_NAME',
    'SYSTEM_REPO_NAME', 'REASON_DEP', 'REASON_USER', 'ICASE',
    # exceptions
    'ArchException', 'Exception', 'QueryException', 'RuntimeException',
    'ValueException',
    # functions
    'chksum_name', 'chksum_type', 'split_nevra',
    # classes
    'Goal', 'NEVRA', 'Package', 'Query', 'Repo', 'Sack', 'Selector']

_QUERY_KEYNAME_MAP = {
    'pkg'	: _hawkey.PKG,
    'description' : _hawkey.PKG_DESCRIPTION,
    'epoch'	: _hawkey.PKG_EPOCH,
    'name'	: _hawkey.PKG_NAME,
    'obsoletes'	: _hawkey.PKG_OBSOLETES,
    'url'	: _hawkey.PKG_URL,
    'arch'	: _hawkey.PKG_ARCH,
    'evr'	: _hawkey.PKG_EVR,
    'version'	: _hawkey.PKG_VERSION,
    'release'	: _hawkey.PKG_RELEASE,
    'sourcerpm'	: _hawkey.PKG_SOURCERPM,
    'summary'	: _hawkey.PKG_SUMMARY,
    'file'	: _hawkey.PKG_FILE,
    'reponame'	: _hawkey.PKG_REPONAME,
    'latest'	: _hawkey.PKG_LATEST,
    'downgrades' : _hawkey.PKG_DOWNGRADES,
    'upgrades'	: _hawkey.PKG_UPGRADES,
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
            for flag in self._flag_kw & kwargs.viewkeys():
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
    if type(obj) is unicode:
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
        elif type(match) in types.StringTypes:
            match = _encode(match)
        elif isinstance(match, collections.Iterable):
            match = map(_encode, match)
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
        self.result = None

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

    def run(self):
        """ Execute the query and cache the result. """
        if self.result is None:
            self.result = super(Query, self).run()
        return self.result

    def count(self):
        return len(self)

    def filter(self, *lst, **kwargs):
        new_query = Query(query=self)
        return new_query.filterm(*lst, **kwargs)

    def filterm(self, *lst, **kwargs):
        self.result = None
        flags = set(lst)
        map(lambda arg_tuple: super(Query, self).filter(*arg_tuple),
            _parse_filter_args(flags, kwargs))
        return self

    def provides(self, name, **kwargs):
        raise NotImplementedError("hawkey.Query.provides is not implemented yet")

class Selector(_hawkey.Selector):
    def set(self, **kwargs):
        map(lambda arg_tuple: super(Selector, self).set(*arg_tuple),
            _parse_filter_args(set(), kwargs))
        return self
