/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2012-2013 Red Hat, Inc.
 * Copyright (C) 2015 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or(at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

/**
 * SECTION:dnf-packageset
 * @short_description: SHORT DESCRIPTION
 * @include: libdnf.h
 * @stability: Unstable
 *
 * Long description.
 *
 * See also: #DnfContext
 */


#include "dnf-sack-private.hpp"
#include "sack/packageset.hpp"

/**
 * dnf_packageset_new:
 * @sack: A #DnfSack
 *
 * Creates a new #DnfPackageSet.
 *
 * Returns:(transfer full): a #DnfPackageSet *
 *
 * Since: 0.7.0
 **/
DnfPackageSet *
dnf_packageset_new(DnfSack *sack)
{
    return new libdnf::PackageSet(sack);
}

/**
 * dnf_packageset_from_bitmap: (skip):
 * @pset: a #DnfPackageSet instance.
 *
 * Creates a set from a bitmap.
 *
 * Returns: a new #DnfPackageSet
 *
 * Since: 0.7.0
 */
DnfPackageSet *
dnf_packageset_from_bitmap(DnfSack *sack, Map *map)
{
    return new libdnf::PackageSet(sack, map);
}

/**
 * dnf_packageset_get_map: (skip):
 * @pset: a #DnfPackageSet instance.
 *
 * Gets the map.
 *
 * Returns: A #Map, or %NULL
 *
 * Since: 0.7.0
 */
Map *
dnf_packageset_get_map(DnfPackageSet *pset)
{
    return pset->getMap();
}

/**
 * dnf_packageset_clone:
 * @pset: a #DnfPackageSet instance.
 *
 * Clones the packageset.
 *
 * Returns: a new #DnfPackageSet
 *
 * Since: 0.7.0
 */
DnfPackageSet *
dnf_packageset_clone(DnfPackageSet *pset)
{
    return new libdnf::PackageSet(*pset);
}

/**
 * dnf_packageset_add:
 * @pset: a #DnfPackageSet instance.
 * @pkg: a #DnfPackage
 *
 * Adds a new package to the set.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
void
dnf_packageset_add(DnfPackageSet *pset, DnfPackage *pkg)
{
    pset->set(pkg);
}

/**
 * dnf_packageset_count:
 * @pset: a #DnfPackageSet instance.
 *
 * Returns the size of the set.
 *
 * Returns: size_t, or 0 for empty
 *
 * Since: 0.7.0
 */
size_t
dnf_packageset_count(DnfPackageSet *pset)
{
    return pset->size();
}

/**
 * dnf_packageset_has:
 * @pset: a #DnfPackageSet instance.
 * @pkg: a #DnfPackage.
 *
 * Finds out if a package is in the set.
 *
 * Returns: %TRUE if it exists
 *
 * Since: 0.7.0
 */
int
dnf_packageset_has(DnfPackageSet *pset, DnfPackage *pkg)
{
    return pset->has(pkg);
}

void
dnf_packageset_free(DnfPackageSet *pset)
{
    delete pset;
}
