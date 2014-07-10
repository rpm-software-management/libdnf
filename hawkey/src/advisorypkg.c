/*
 * Copyright (C) 2014 Red Hat, Inc.
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

// libsolv
#include <solv/util.h>

// hawkey
#include "advisorypkg_internal.h"

#define PACKAGE_BLOCK 15

struct _HyAdvisoryPkg {
    char *name;
    char *evr;
    char *arch;
    char *filename;
};

struct _HyAdvisoryPkgList {
    HyAdvisoryPkg *pkgs;
    int npkgs;
};

/* internal */
HyAdvisoryPkg
advisorypkg_create()
{
    HyAdvisoryPkg pkg = solv_calloc(1, sizeof(*pkg));
    return pkg;
}

static char **
get_string(HyAdvisoryPkg advisorypkg, int which)
{
    switch (which) {
    case HY_ADVISORYPKG_NAME:
	return &(advisorypkg->name);
    case HY_ADVISORYPKG_EVR:
	return &(advisorypkg->evr);
    case HY_ADVISORYPKG_ARCH:
	return &(advisorypkg->arch);
    case HY_ADVISORYPKG_FILENAME:
	return &(advisorypkg->filename);
    default:
	return NULL;
    }
}

void
advisorypkg_set_string(HyAdvisoryPkg advisorypkg, int which, const char* str_val)
{
    char** attr = get_string(advisorypkg, which);
    solv_free(*attr);
    *attr = solv_strdup(str_val);
}

HyAdvisoryPkg
advisorypkg_clone(HyAdvisoryPkg advisorypkg)
{
    HyAdvisoryPkg clone = advisorypkg_create();
    advisorypkg_set_string(clone, HY_ADVISORYPKG_NAME,
	    hy_advisorypkg_get_string(advisorypkg, HY_ADVISORYPKG_NAME));
    advisorypkg_set_string(clone, HY_ADVISORYPKG_EVR,
	    hy_advisorypkg_get_string(advisorypkg, HY_ADVISORYPKG_EVR));
    advisorypkg_set_string(clone, HY_ADVISORYPKG_ARCH,
	    hy_advisorypkg_get_string(advisorypkg, HY_ADVISORYPKG_ARCH));
    advisorypkg_set_string(clone, HY_ADVISORYPKG_FILENAME,
	    hy_advisorypkg_get_string(advisorypkg, HY_ADVISORYPKG_FILENAME));
    return clone;
}

int
advisorypkg_identical(HyAdvisoryPkg left, HyAdvisoryPkg right)
{
    return
	strcmp(left->name, right->name) &&
	strcmp(left->evr, right->evr) &&
	strcmp(left->arch, right->arch) &&
	strcmp(left->filename, right->filename);
}

HyAdvisoryPkgList
advisorypkglist_create()
{
    HyAdvisoryPkgList pkglist = solv_calloc(1, sizeof(*pkglist));
    pkglist->npkgs = 0;
    pkglist->pkgs = solv_extend(
	0, pkglist->npkgs, 0, sizeof(HyAdvisoryPkg), PACKAGE_BLOCK);
    return pkglist;
}

void
advisorypkglist_add(HyAdvisoryPkgList pkglist, HyAdvisoryPkg advisorypkg)
{
    pkglist->pkgs = solv_extend(
	    pkglist->pkgs, pkglist->npkgs, 1, sizeof(advisorypkg), PACKAGE_BLOCK);
    pkglist->pkgs[pkglist->npkgs++] = advisorypkg_clone(advisorypkg);
}

/* public */
void
hy_advisorypkg_free(HyAdvisoryPkg advisorypkg)
{
    solv_free(advisorypkg->name);
    solv_free(advisorypkg->evr);
    solv_free(advisorypkg->arch);
    solv_free(advisorypkg->filename);
    solv_free(advisorypkg);
}

const char *
hy_advisorypkg_get_string(HyAdvisoryPkg advisorypkg, int which)
{
    return *get_string(advisorypkg, which);
}

void
hy_advisorypkglist_free(HyAdvisoryPkgList pkglist)
{
    for(int i = 0; i < hy_advisorypkglist_count(pkglist); i++) {
	hy_advisorypkg_free(pkglist->pkgs[i]);
    }
    solv_free(pkglist->pkgs);
    solv_free(pkglist);
}

int
hy_advisorypkglist_count(HyAdvisoryPkgList pkglist)
{
    return pkglist->npkgs;
}

HyAdvisoryPkg
hy_advisorypkglist_get_clone(HyAdvisoryPkgList pkglist, int index)
{
    return advisorypkg_clone(pkglist->pkgs[index]);
}
