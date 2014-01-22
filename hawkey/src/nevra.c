/*
 * Copyright (C) 2013 Red Hat, Inc.
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <solv/util.h>
#include "query.h"
#include "nevra.h"
#include "nevra_internal.h"
#include "sack.h"
#include "types.h"


static void
hy_nevra_clear(HyNevra nevra)
{
    nevra->name = NULL;
    nevra->epoch = -1L;
    nevra->version = NULL;
    nevra->release = NULL;
    nevra->arch = NULL;
}

HyNevra
hy_nevra_create()
{
    HyNevra nevra = solv_calloc(1, sizeof(*nevra));
    hy_nevra_clear(nevra);
    return nevra;
}

static inline int
string_cmp(const char *s1, const char *s2)
{
    if (s1 == NULL)
	return -1;
    else if (s2 == NULL)
	return 1;
    return strcmp(s1, s2);
}

int
hy_nevra_cmp(HyNevra nevra1, HyNevra nevra2)
{
    int ret;
    char *nevra1_strs[] = { nevra1->name, nevra1->version, nevra1->release, nevra1->arch };
    char *nevra2_strs[] = { nevra2->name, nevra2->version, nevra2->release, nevra2->arch };
    if (nevra1->epoch < nevra2->epoch)
	return -1;
    else if (nevra1->epoch > nevra2->epoch)
	return 1;
    else
	return 0;
    for (int i = 0; i < 4; ++i) {
	ret = string_cmp(nevra1_strs[i], nevra2_strs[i]);
	if (ret)
	    return ret;
    }
    return 0;
}

void
hy_nevra_free(HyNevra nevra)
{
    solv_free(nevra->name);
    solv_free(nevra->version);
    solv_free(nevra->release);
    solv_free(nevra->arch);
    solv_free(nevra);
}

HyNevra
hy_nevra_clone(HyNevra nevra)
{
    HyNevra clone = hy_nevra_create();
    clone->name = solv_strdup(nevra->name);
    clone->epoch = nevra->epoch;
    clone->version = solv_strdup(nevra->version);
    clone->release = solv_strdup(nevra->release);
    clone->arch = solv_strdup(nevra->arch);
    return clone;
}

static inline char **
get_string(HyNevra nevra, int which)
{
    switch (which) {
    case HY_NEVRA_NAME:
	return &(nevra->name);
    case HY_NEVRA_VERSION:
	return &(nevra->version);
    case HY_NEVRA_RELEASE:
	return &(nevra->release);
    case HY_NEVRA_ARCH:
	return &(nevra->arch);
    default:
	return NULL;
    }
}

const char *
hy_nevra_get_string(HyNevra nevra, int which)
{
    return *get_string(nevra, which);
}

void
hy_nevra_set_string(HyNevra nevra, int which, const char* str_val)
{
    char** attr = get_string(nevra, which);
    solv_free(*attr);
    *attr = solv_strdup(str_val);
}

int
hy_nevra_get_epoch(HyNevra nevra)
{
    return nevra->epoch;
}

int
hy_nevra_set_epoch(HyNevra nevra, int epoch)
{
    return nevra->epoch = epoch;
}

HyQuery
hy_nevra_to_query(HyNevra nevra, HySack sack)
{
    HyQuery query = hy_query_create(sack);
    hy_query_filter(query, HY_PKG_NAME, HY_EQ, nevra->name);
    hy_query_filter_num(query, HY_PKG_EPOCH, HY_EQ, nevra->epoch);
    hy_query_filter(query, HY_PKG_VERSION, HY_EQ, nevra->version);
    hy_query_filter(query, HY_PKG_RELEASE, HY_EQ, nevra->release);
    hy_query_filter(query, HY_PKG_ARCH, HY_EQ, nevra->arch);
    return query;
}

int
hy_nevra_evr_cmp(HyNevra nevra1, HyNevra nevra2, HySack sack)
{
    char *self_evr = hy_nevra_get_evr(nevra1);
    char *other_evr = hy_nevra_get_evr(nevra2);
    int cmp = hy_sack_evr_cmp(sack, self_evr, other_evr);
    solv_free(self_evr);
    solv_free(other_evr);
    return cmp;
}

#define VER_REL_LENGTH(nevra) (strlen(nevra->version) + strlen(nevra->release))

char *hy_nevra_get_evr(HyNevra nevra)
{
    char *str = NULL;
    if (nevra->epoch == -1) {
	str = solv_malloc2(sizeof(char), 2 + VER_REL_LENGTH(nevra));
	sprintf(str, "%s-%s", nevra->version, nevra->release);
    } else {
	str = solv_malloc2(sizeof(char), 18 + VER_REL_LENGTH(nevra));
	sprintf(str, "%d:%s-%s", nevra->epoch, nevra->version, nevra->release);
    }
    return str;
}

#undef VER_REL_LENGTH
