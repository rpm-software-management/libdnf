/*
 * Copyright (C) 2012-2015 Red Hat, Inc.
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef HY_SACK_INTERNAL_H
#define HY_SACK_INTERNAL_H

#include <stdio.h>
#include <solv/pool.h>
#include <vector>

#include "dnf-sack.h"
#include "hy-query.h"
#include "sack/packageset.hpp"
#include "sack/query.hpp"
#include "module/ModulePackage.hpp"
#include "module/ModulePackageContainer.hpp"

typedef Id  (*dnf_sack_running_kernel_fn_t) (DnfSack    *sack);

/**
 * @brief Store Map with only pkg_solvables to increase query performance
 *
 * @param sack p_sack:...
 * @param pkg_solvables Map with only all pkg_solvables
 * @param pool_nsolvables Number of pool_nsolvables in pool. It used as checksum.
 */
void dnf_sack_set_pkg_solvables(DnfSack *sack, Map *pkg_solvables, int pool_nsolvables);

/**
 * @brief Returns number of pool_nsolvables at time of creation of pkg_solvables. It can be used to
 *        verify Map contant
 *
 * @param sack p_sack:...
 * @return int
 */
int dnf_sack_get_pool_nsolvables(DnfSack *sack);

/**
 * @brief Returns pointer PackageSet with every package solvable in pool
 *
 * @param sack p_sack:...
 * @return Map*
 */
libdnf::PackageSet *dnf_sack_get_pkg_solvables(DnfSack *sack);
libdnf::ModulePackageContainer * dnf_sack_set_module_container(
    DnfSack *sack, libdnf::ModulePackageContainer * newConteiner);
libdnf::ModulePackageContainer * dnf_sack_get_module_container(DnfSack *sack);
void         dnf_sack_make_provides_ready   (DnfSack    *sack);
Id           dnf_sack_running_kernel        (DnfSack    *sack);
void         dnf_sack_recompute_considered_map  (DnfSack * sack, Map ** considered, libdnf::Query::ExcludeFlags flags);
void         dnf_sack_recompute_considered  (DnfSack    *sack);
Id           dnf_sack_last_solvable         (DnfSack    *sack);
const char * dnf_sack_get_arch              (DnfSack    *sack);
void         dnf_sack_set_provides_not_ready(DnfSack    *sack);
void         dnf_sack_set_considered_to_update(DnfSack * sack);
void         dnf_sack_set_running_kernel_fn (DnfSack    *sack,
                                             dnf_sack_running_kernel_fn_t fn);
DnfPackage  *dnf_sack_add_cmdline_package_flags   (DnfSack *sack,
                            const char *fn, const int flags);
std::pair<std::vector<std::vector<std::string>>, libdnf::ModulePackageContainer::ModuleErrorType> dnf_sack_filter_modules_v2(
    DnfSack *sack, libdnf::ModulePackageContainer * moduleContainer, const char ** hotfixRepos,
    const char *install_root, const char * platformModule, bool updateOnly, bool debugSolver);

std::vector<libdnf::ModulePackage *> requiresModuleEnablement(DnfSack * sack, const libdnf::PackageSet * installSet);

/**
 * @brief Return fingerprint of installed RPMs.
 * The format is <count>:<hash>.
 * <count> is a count of installed RPMs.
 * <hash> is a sha1 hash of sorted sha1hdr hashes of installed RPMs.
 *
 * The count can be computed from the command line by running:
 * rpm -qa --qf='%{name}\n' | grep -v '^gpg-pubkey$' | wc -l
 *
 * The hash can be computed from the command line by running:
 * rpm -qa --qf='%{name} %{sha1header}\n' | grep -v '^gpg-pubkey ' \
 * | cut -d ' ' -f 2 | LC_ALL=C sort | tr -d '\n' | sha1sum
 */
std::string dnf_sack_get_rpmdb_version(DnfSack *sack);

#endif // HY_SACK_INTERNAL_H
