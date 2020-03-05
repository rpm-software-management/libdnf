/*
Copyright (C) 2020 Red Hat, Inc.

This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

Libdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Libdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with libdnf.  If not, see <https://www.gnu.org/licenses/>.
*/


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
