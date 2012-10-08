import _hawkey
import collections
import types

VERSION_MAJOR = _hawkey.VERSION_MAJOR
VERSION_MINOR = _hawkey.VERSION_MINOR
VERSION_PATCH = _hawkey.VERSION_PATCH
VERSION=u"%d.%d.%d" % (VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH)

Repo = _hawkey.Repo

SYSTEM_REPO_NAME = _hawkey.SYSTEM_REPO_NAME
CMDLINE_REPO_NAME = _hawkey.CMDLINE_REPO_NAME

QUERY_KEYNAME_MAP = {
    'pkg'	: _hawkey.PKG,
    'description' : _hawkey.PKG_DESCRIPTION,
    'epoch'	: _hawkey.PKG_EPOCH,
    'name'	: _hawkey.PKG_NAME,
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
    'obsoleting' : _hawkey.PKG_OBSOLETING
}

ICASE = _hawkey.ICASE
QUERY_FT_MAP = {
    'eq' : _hawkey.EQ,
    'gt' : _hawkey.GT,
    'lt' : _hawkey.LT,
    'neq' : _hawkey.NEQ,
    'gte' : _hawkey.EQ | _hawkey.GT,
    'lte' : _hawkey.EQ | _hawkey.LT,
    'substr'  : _hawkey.SUBSTR,
    'glob' : _hawkey.GLOB,
}

CHKSUM_MD5 = _hawkey.CHKSUM_MD5
CHKSUM_SHA1 = _hawkey.CHKSUM_SHA1
CHKSUM_SHA256 = _hawkey.CHKSUM_SHA256

_CHKSUM_TYPES = {
    CHKSUM_MD5: 'md5',
    CHKSUM_SHA1: 'sha1',
    CHKSUM_SHA256: 'sha256'
    }

Exception = _hawkey.Exception
QueryException = _hawkey.QueryException
ValueException = _hawkey.ValueException
ArchException = _hawkey.ArchException
RuntimeException = _hawkey.RuntimeException

Sack = _hawkey.Sack
chksum_name = _hawkey.chksum_name
chksum_type = _hawkey.chksum_type

REASON_DEP = _hawkey.REASON_DEP
REASON_USER = _hawkey.REASON_USER

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

class Goal(_hawkey.Goal):
    @property
    def problems(self):
        return [self.describe_problem(i) for i in range(0, self.count_problems())]

    def run(self, callback=None, **kwargs):
        ret = super(Goal, self).run(**kwargs)
        if callback:
            callback(self)
        return ret

class Package(_hawkey.Package):
    def evr_eq(self, pkg):
        return self.evr_cmp(pkg) == 0

    def evr_gt(self, pkg):
        return self.evr_cmp(pkg) > 0

    def evr_lt(self, pkg):
        return self.evr_cmp(pkg) < 0

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

    def _parse_filter_args(self, flags, dct):
        args = []
        filter_flags = 0
        for flag in flags:
            if not flag in [ICASE]:
                raise ValueError("unrecognized flag: %s" % flag)
            filter_flags |= flag
        for (k, match) in dct.items():
            if type(match) in types.StringTypes:
                match = _encode(match)
            elif isinstance(match, collections.Iterable):
                match = map(_encode, match)
            split = k.split("__", 1)
            if len(split) == 1:
                args.append((QUERY_KEYNAME_MAP[split[0]],
                             QUERY_FT_MAP["eq"]|filter_flags,
                             match))
            elif len(split) == 2:
                (keyname, filter_type) = split
                if not keyname in QUERY_KEYNAME_MAP:
                    raise ValueError("Unrecognized key name: %s" % keyname)
                if not filter_type in QUERY_FT_MAP:
                    raise ValueError("Unrecognized filter type: %s" % filter_type)
                args.append((QUERY_KEYNAME_MAP[keyname],
                             QUERY_FT_MAP[filter_type]|filter_flags,
                             match))
            else:
                raise ValueError("keyword arguments given to filter() need be "
                                 "in <key>__<comparison type>=<value> format")
        return args

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
            self._parse_filter_args(flags, kwargs))
        return self

    def provides(self, name, **kwargs):
        raise NotImplementedError("hawkey.Query.provides is not implemented yet")
