#ifndef LIBDNF_REPO_REPOS_HPP
#define LIBDNF_REPO_REPOS_HPP

#include "repo.hpp"


// TODO(dmach): include @System and @Commandline repos in Repos


namespace libdnf::repo {


// class definitions to mute clang-tidy complaints
// to be removed / implemented
class RepoConf;


/// @replaces dnf:dnf/repodict.py:class:RepoDict
class Repos {
public:
    /// @replaces dnf:dnf/repodict.py:method:RepoDict.add(self, repo)
    /// @replaces dnf:dnf/repodict.py:method:RepoDict.add_new_repo(self, repoid, conf, baseurl=(), **kwargs)
    Repo & new_repo(RepoConf conf);

    // TODO(dmach): Query() -> get(id), filter(...)
};


}  // namespace libdnf::repo

/*
dnf:dnf/repodict.py:method:RepoDict.all(self)
dnf:dnf/repodict.py:method:RepoDict.clear()
dnf:dnf/repodict.py:method:RepoDict.copy()
dnf:dnf/repodict.py:method:RepoDict.enable_debug_repos(self)
dnf:dnf/repodict.py:method:RepoDict.enable_source_repos(self)
dnf:dnf/repodict.py:method:RepoDict.fromkeys(iterable, value=None, /)
dnf:dnf/repodict.py:method:RepoDict.get(self, key, default=None, /)
dnf:dnf/repodict.py:method:RepoDict.get_matching(self, key)
dnf:dnf/repodict.py:method:RepoDict.items(self)
dnf:dnf/repodict.py:method:RepoDict.iter_enabled(self)
dnf:dnf/repodict.py:method:RepoDict.keys(self)
dnf:dnf/repodict.py:method:RepoDict.pop()
dnf:dnf/repodict.py:method:RepoDict.popitem()
dnf:dnf/repodict.py:method:RepoDict.setdefault(self, key, default=None, /)
dnf:dnf/repodict.py:method:RepoDict.update()
dnf:dnf/repodict.py:method:RepoDict.values(self)
*/

#endif
