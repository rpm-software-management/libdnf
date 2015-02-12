/*
 * Copyright (C) 2012-2013 Red Hat, Inc.
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

#include "assert.h"

// libsolv
#include <solv/pool.h>
#include <solv/repo.h>
#include <solv/queue.h>
#include <solv/util.h>

// GLib
#include <glib.h>

// hawkey
#include "packagelist.h"
#include "package_internal.h"
#include "sack_internal.h"

HyPackageList
hy_packagelist_create(void)
{
    return (HyPackageList)g_ptr_array_new_with_free_func ((GDestroyNotify)hy_package_free);
}

void
hy_packagelist_free(HyPackageList plist)
{
    GPtrArray *a = (GPtrArray*)plist;
    g_ptr_array_unref(a);
}

int
hy_packagelist_count(HyPackageList plist)
{
    GPtrArray *a = (GPtrArray*)plist;
    return a->len;
}

/**
 * Returns the package at position 'index'.
 *
 * Returns NULL if the packagelist doesn't have enough elements for index.
 *
 * Borrows the caller packagelist's reference, caller shouldn't call
 * hy_package_free().
 */
HyPackage
hy_packagelist_get(HyPackageList plist, int index)
{
    GPtrArray *a = (GPtrArray*)plist;
    if (index < a->len)
	return a->pdata[index];
    return NULL;
}

/**
 * Returns the package at position 'index'.
 *
 * Gives caller a new reference to the package that can survive the
 * packagelist. The returned package has to be freed via hy_package_free().
 */
HyPackage
hy_packagelist_get_clone(HyPackageList plist, int index)
{
    return hy_package_link(hy_packagelist_get(plist, index));
}

int
hy_packagelist_has(HyPackageList plist, HyPackage pkg)
{
    GPtrArray *a = (GPtrArray*)plist;
    for (int i = 0; i < a->len; ++i)
	if (hy_package_identical(pkg, a->pdata[i]))
	    return 1;
    return 0;
}

/**
 * Adds pkg at the end of plist.
 *
 * Assumes ownership of pkg and will free it during hy_packagelist_free().
 */
void hy_packagelist_push(HyPackageList plist, HyPackage pkg)
{
    GPtrArray *a = (GPtrArray*)plist;
    g_ptr_array_add(a, pkg);
}
