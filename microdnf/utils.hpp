/*
Copyright (C) 2020 Red Hat, Inc.

This file is part of microdnf: https://github.com/rpm-software-management/libdnf/

Microdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Microdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with microdnf.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef MICRODNF_UTILS_HPP
#define MICRODNF_UTILS_HPP

#include <string>

namespace microdnf {

bool am_i_root();
bool is_directory(const char * path);
void ensure_dir(const std::string & dname);
std::string normalize_time(time_t timestamp);


// ================================================
// Next utils probably will be movde to libdnf

/* return a path to a valid and safe cachedir - only used when not running
   as root or when --tempcache is set */
std::string get_cache_dir();

/* find the base architecture */
const char * get_base_arch(const char * arch);

std::string detect_arch();
std::string detect_release(const std::string & install_root_path);

}  // namespace microdnf

#endif
