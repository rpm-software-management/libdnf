import _hawkey

Repo = _hawkey.Repo

SYSTEM_REPO_NAME = _hawkey.SYSTEM_REPO_NAME
CMDLINE_REPO_NAME = _hawkey.CMDLINE_REPO_NAME

QUERY_KEYNAME_MAP = {
    'name'    : _hawkey.KN_PKG_NAME,
    'summary' : _hawkey.KN_PKG_SUMMARY,
    'repo'    : _hawkey.KN_PKG_REPO,
    'latest'  : _hawkey.KN_PKG_LATEST,
    'updates' : _hawkey.KN_PKG_UPDATES,
    'obsoleting' : _hawkey.KN_PKG_OBSOLETING
}

QUERY_FT_MAP = {
    'eq' : _hawkey.FT_EQ,
    'gt' : _hawkey.FT_GT,
    'lt' : _hawkey.FT_LT,
    'neq' : _hawkey.FT_LT | _hawkey.FT_GT,
    'gte' : _hawkey.FT_EQ | _hawkey.FT_GT,
    'lte' : _hawkey.FT_EQ | _hawkey.FT_LT,
    'substr'  : _hawkey.FT_SUBSTR,
}

Sack = _hawkey.Sack
Goal = _hawkey.Goal

class Package(_hawkey.Package):
    def evr_eq(self, pkg):
        return self.evr_cmp(pkg) == 0

    def evr_gt(self, pkg):
        return self.evr_cmp(pkg) > 0

    def evr_lt(self, pkg):
        return self.evr_cmp(pkg) < 0

class Query(object):
    def __init__(self, sack):
        self._query = _hawkey.Query(sack)
        self.sack = sack
        self.result = None

    def __iter__(self):
        if not self.result:
            self.result = self._query.run()
        for p in self.result:
            yield p

    def _parse_filter_args(self, dct):
        args = []
        for (k, match) in dct.items():
            (keyname, filter_type) = k.split("__", 1)
            if not keyname in QUERY_KEYNAME_MAP:
                raise ValueError("Unrecognized key name: %s" % keyname)
            if not filter_type in QUERY_FT_MAP:
                raise ValueError("Unrecognized filter type: %s" % filter_type)
            args.append((QUERY_KEYNAME_MAP[keyname],
                         QUERY_FT_MAP[filter_type],
                         match))
        return args

    def filter(self, **kwargs):
        map(lambda arg_tuple: self._query.filter(*arg_tuple),
            self._parse_filter_args(kwargs))
        return self

    def provides(self, name, **kwargs):
        raise NotImplementedError("hawkey.Query.provides is not implemented yet")
