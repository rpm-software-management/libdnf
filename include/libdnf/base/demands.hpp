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


#ifndef LIBDNF_BASE_DEMANDS_HPP
#define LIBDNF_BASE_DEMANDS_HPP

#include <string>
#include <vector>


namespace libdnf {


/// Demands are runtime options that are not instance wide
/// and can differ for each method call.
class Demands {
public:
    // allowerasing
    bool get_allow_erasing() const { return allowErasing; }
    void set_allow_erasing(bool value) { allowErasing = value; }

    // best
    bool get_best() const { return best; }
    void set_best(bool value) { best = value; }

    // cacheonly: 0 = off; 1 = repodata; 2 = repodata + packages
    int get_cache_only_level() const { return cacheOnlyLevel; }
    void set_cache_only_level(int value) { cacheOnlyLevel = value; }

private:
    bool allowErasing = false;
    bool best = false;
    int cacheOnlyLevel = 0;
    std::vector<std::string> fromRepo;
    /*
    available_repos = _BoolDefault(False)
    resolving = _BoolDefault(False)
    root_user = _BoolDefault(False)
    sack_activation = _BoolDefault(False)
    load_system_repo = _BoolDefault(True)
    cacheonly = _BoolDefault(False)
    fresh_metadata = _BoolDefault(True)
    freshest_metadata = _BoolDefault(False)
    changelogs = _BoolDefault(False)

    TODO(dmach): debugsolver
    */
};

}  // namespace libdnf

#endif
