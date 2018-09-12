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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <glib.h>

#include "dnf-db.h"
#include "dnf-package.h"
#include "transaction/Swdb.hpp"

/**
 * dnf_db_ensure_origin_pkg:
 * @db: a #DnfDb instance.
 * @pkg: A package to set
 *
 * Sets the repo origin on a package if not already set.
 *
 * Since: 0.1.0
 */
void
dnf_db_ensure_origin_pkg(DnfDb *db, DnfPackage *pkg)
{
    /* already set */
    if (dnf_package_get_origin(pkg) != NULL)
        return;
    if (!dnf_package_installed(pkg))
        return;

    /* set from the database if available */
    auto tmp = db->getRPMRepo(dnf_package_get_nevra(pkg));
    if (tmp.empty()) {
        g_debug("no origin for %s", dnf_package_get_package_id(pkg));
    } else {
        dnf_package_set_origin(pkg, tmp.c_str());
    }
}

/**
 * dnf_db_ensure_origin_pkglist:
 * @db: a #DnfDb instance.
 * @pkglist: A package list to set
 *
 * Sets the repo origin on several package if not already set.
 *
 * Since: 0.1.0
 */
void
dnf_db_ensure_origin_pkglist(DnfDb *db, GPtrArray *pkglist)
{
    DnfPackage * pkg;
    guint i;
    for (i = 0; i < pkglist->len; i++) {
        pkg = static_cast<DnfPackage *>(g_ptr_array_index(pkglist, i));
        dnf_db_ensure_origin_pkg(db, pkg);
    }
}
