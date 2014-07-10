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

struct _HyAdvisoryPkg {
    char *name;
    char *evr;
    char *arch;
    char *filename;
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
