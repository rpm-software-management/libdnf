/*
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef _LIBDNF_CONFIG_CONST_HPP
#define _LIBDNF_CONFIG_CONST_HPP

#include <string>
#include <vector>

namespace libdnf {

constexpr const char * PERSISTDIR = "/var/lib/dnf";
constexpr const char * SYSTEM_CACHEDIR = "/var/cache/dnf";

constexpr const char * CONF_FILENAME = "/etc/dnf/dnf.conf";
// drop-in configuration directories
constexpr const char * CONF_DIR = "/etc/dnf/libdnf5.conf.d";
constexpr const char * DISTRIBUTION_CONF_DIR = "/usr/share/dnf5/libdnf.conf.d/";

// directories with repository configuration overides
constexpr const char * REPOS_OVERRIDE_DIR = "/etc/dnf/repos.override.d";
constexpr const char * DISTRIBUTION_REPOS_OVERRIDE_DIR = "/usr/share/dnf5/repos.override.d";

// More important varsdirs must be on the end of vector
const std::vector<std::string> VARS_DIRS{"/etc/yum/vars", "/etc/dnf/vars"};

const std::vector<std::string> GROUP_PACKAGE_TYPES{"mandatory", "default", "conditional"};
const std::vector<std::string> INSTALLONLYPKGS{"kernel", "kernel-PAE",
                 "installonlypkg(kernel)",
                 "installonlypkg(kernel-module)",
                 "installonlypkg(vm)",
                 "multiversion(kernel)"};

constexpr const char * BUGTRACKER="https://bugzilla.redhat.com/enter_bug.cgi?product=Fedora&component=dnf";

}

#endif
