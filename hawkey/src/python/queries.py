import hawkey
import itertools

def _construct_result(sack, patterns, ignore_case,
                      include_repo=None, exclude_repo=None,
                      updates_only=False, latest_only=False):
    if ignore_case:
        print("warning: hawkey can not match case-insensitive yet, ignored.")
    queries = []
    for p in patterns:
        q = hawkey.Query(sack)
        # autodetect glob patterns
        if set(p) & set("*[?"):
            q.filter(name__glob=p)
        else:
            q.filter(name__eq=p)
        if include_repo:
            q.filter(repo__eq=include_repo)
        if exclude_repo:
            q.filter(repo__neq=exclude_repo)
        q.filter(updates__eq=updates_only)
        q.filter(latest__eq=latest_only)
        queries.append(q)
    return itertools.chain.from_iterable(queries)

def installed_by_name(sack, patterns, ignore_case=False):
    return _construct_result(sack, patterns, ignore_case,
                             include_repo=hawkey.SYSTEM_REPO_NAME)

def available_by_name(sack, patterns, ignore_case=False, latest_only=False):
    return _construct_result(sack, patterns, ignore_case,
                             exclude_repo=hawkey.SYSTEM_REPO_NAME,
                             latest_only=latest_only)

def by_name(sack, patterns, ignore_case=False):
    return _construct_result(sack, patterns, ignore_case)

def latest_per_arch(sack, patterns, ignore_case=False, include_repo=None,
                    exclude_repo=None):
    matching = _construct_result(sack, patterns, ignore_case,
                                 include_repo, exclude_repo)
    latest = {} # (name, arch) -> pkg mapping
    for pkg in matching:
        key = (pkg.name, pkg.arch)
        if key in latest and latest[key].evr_gt(pkg):
            continue
        latest[key] = pkg
    return latest

def latest_installed_per_arch(sack, patterns, ignore_case=False):
    return latest_per_arch(sack, patterns, ignore_case,
                           include_repo=hawkey.SYSTEM_REPO_NAME)

def latest_available_per_arch(sack, patterns, ignore_case=False):
    return latest_per_arch(sack, patterns, ignore_case,
                           exclude_repo=hawkey.SYSTEM_REPO_NAME)

def updates_by_name(sack, patterns, ignore_case=False):
    return _construct_result(sack, patterns, ignore_case,
                             updates_only=True)

def per_arch_dict(pkg_list):
    d = {}
    for pkg in pkg_list:
        key = (pkg.name, pkg.arch)
        d.setdefault(key, []).append(pkg)
    return d

def per_pkgtup_dict(pkg_list):
    d = {}
    for pkg in pkg_list:
        d.setdefault(pkg.pkgtup, []).append(pkg)
    return d
