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

class Package(object):
    def __init__(self, package):
        self._package = package # _hawkey object
        self.name = self._package.name
        self.arch = self._package.arch
        self.evr = self._package.evr
        self.reponame = self._package.reponame
        self.location = self._package.location
        self.medianr = self._package.medianr
        self.size = self._package.size

    @property
    def rpmdbid(self):
        return self._package.rpmdbid

    def __cmp__(self, pkg):
        return cmp(self._package, pkg._package)

    def evr_eq(self, pkg):
        return self._package.evr_cmp(pkg._package) == 0

    def evr_gt(self, pkg):
        return self._package.evr_cmp(pkg._package) > 0

    def evr_lt(self, pkg):
        return self._package.evr_cmp(pkg._package) < 0

    def obsoletes_list(self, sack):
        for p in self._package.obsoletes_list():
            yield sack.create_package(p)

class Query(object):
    def __init__(self, sack):
        self._query = _hawkey.Query(sack)
        self.sack = sack
        self.result = None

    def __iter__(self):
        if not self.result:
            self.result = self._query.run()
        for p in self.result:
            yield self.sack.create_package(p)

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

class Sack(_hawkey.Sack):
    def __init__(self, PackageClass=None, package_userdata=None):
        super(Sack, self).__init__()
        self.PackageClass = PackageClass
        self.package_userdata = package_userdata

    def add_cmdline_rpm(self, fn):
        _pkg = super(Sack, self).add_cmdline_rpm(fn)
        return self.create_package(_pkg)

    def create_package(self, pkg):
        """ Create hawkey.Package and wrap it as needed.

            pkg is _hawkey.Package object we will be wrapping.
        """
        if self.PackageClass:
            return self.PackageClass(pkg, self.package_userdata)
        else:
            return Package(pkg)

class Goal(_hawkey.Goal):
    def erase(self, pkg):
        return super(Goal, self).erase(pkg._package)

    def install(self, pkg):
        return super(Goal, self).install(pkg._package)

    def update(self, pkg):
        return super(Goal, self).update(pkg._package)

    def list_erasures(self):
        l = super(Goal, self).list_erasures()
        return map(lambda p: self.sack.create_package(p), l)

    def list_installs(self):
        l = super(Goal, self).list_installs()
        return map(lambda p: self.sack.create_package(p), l)

    def list_upgrades(self):
        l = super(Goal, self).list_upgrades()
        return map(lambda p: self.sack.create_package(p), l)

    def package_upgrades(self, pkg):
        ret = super(Goal, self).package_upgrades(pkg._package)
        return self.sack.create_package(ret)

    def problems(self):
        for i in range(self.count_problems()):
            yield self.describe_problem(i)
