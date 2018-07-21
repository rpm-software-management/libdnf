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
#include "sack/packageset.hpp"
#include "module/ModulePackage.hpp"

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

void         dnf_sack_make_provides_ready   (DnfSack    *sack);
Id           dnf_sack_running_kernel        (DnfSack    *sack);
void         dnf_sack_recompute_considered  (DnfSack    *sack);
Id           dnf_sack_last_solvable         (DnfSack    *sack);

Queue       *dnf_sack_get_installonly       (DnfSack    *sack);
void         dnf_sack_set_running_kernel_fn (DnfSack    *sack,
                                             dnf_sack_running_kernel_fn_t fn);
DnfPackage  *dnf_sack_add_cmdline_package_flags   (DnfSack *sack,
                            const char *fn, const int flags);
std::vector<std::shared_ptr<ModulePackage>> requiresModuleEnablement(DnfSack * sack, const libdnf::PackageSet * installSet);

#endif // HY_SACK_INTERNAL_H
