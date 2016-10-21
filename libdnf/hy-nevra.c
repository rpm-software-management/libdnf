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
#include "hy-query.h"
#include "hy-nevra.h"
#include "hy-nevra-private.h"
#include "dnf-sack.h"
#include "hy-types.h"


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
    HyNevra nevra = g_malloc0(sizeof(*nevra));
    hy_nevra_clear(nevra);
    return nevra;
}

static inline int
string_cmp(const char *s1, const char *s2)
{
    if (s1 == NULL) {
        if (s2 == NULL)
            return 0;
        return -1;
    } else if (s2 == NULL)
        return 1;
    return strcmp(s1, s2);
}

int
hy_nevra_cmp(HyNevra nevra1, HyNevra nevra2)
{
    int ret;
    ret = string_cmp(nevra1->name, nevra2->name);
    if (ret != 0)
        return ret;
    ret = hy_nevra_evr_cmp(nevra1, nevra2, NULL);
    if (ret != 0)
        return ret;
    return string_cmp(nevra1->arch, nevra2->arch);
}

void
hy_nevra_free(HyNevra nevra)
{
    g_free(nevra->name);
    g_free(nevra->version);
    g_free(nevra->release);
    g_free(nevra->arch);
    g_free(nevra);
}

HyNevra
hy_nevra_clone(HyNevra nevra)
{
    HyNevra clone = hy_nevra_create();
    clone->name = g_strdup(nevra->name);
    clone->epoch = nevra->epoch;
    clone->version = g_strdup(nevra->version);
    clone->release = g_strdup(nevra->release);
    clone->arch = g_strdup(nevra->arch);
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
    g_free(*attr);
    *attr = g_strdup(str_val);
}

int
hy_nevra_get_epoch(HyNevra nevra)
{
    return nevra->epoch;
}

void
hy_nevra_set_epoch(HyNevra nevra, int epoch)
{
    nevra->epoch = epoch;
}

HyQuery
hy_nevra_to_query(HyNevra nevra, DnfSack *sack)
{
    HyQuery query = hy_query_create(sack);
    if (nevra->name != NULL)
        hy_query_filter(query, HY_PKG_NAME, HY_EQ, nevra->name);
    if (nevra->epoch != -1)
        hy_query_filter_num(query, HY_PKG_EPOCH, HY_EQ, nevra->epoch);
    if (nevra->version != NULL)
        hy_query_filter(query, HY_PKG_VERSION, HY_EQ, nevra->version);
    if (nevra->release != NULL)
        hy_query_filter(query, HY_PKG_RELEASE, HY_EQ, nevra->release);
    if (nevra->arch != NULL)
        hy_query_filter(query, HY_PKG_ARCH, HY_EQ, nevra->arch);
    return query;
}

int
hy_nevra_evr_cmp(HyNevra nevra1, HyNevra nevra2, DnfSack *sack)
{
    char *self_evr = hy_nevra_get_evr(nevra1);
    char *other_evr = hy_nevra_get_evr(nevra2);
    int cmp = dnf_sack_evr_cmp(sack, self_evr, other_evr);
    g_free(self_evr);
    g_free(other_evr);
    return cmp;
}

char *hy_nevra_get_evr(HyNevra nevra)
{
    if (nevra->epoch == -1)
        return g_strdup_printf ("%s-%s", nevra->version, nevra->release);
    return g_strdup_printf ("%d:%s-%s", nevra->epoch, nevra->version, nevra->release);
}

gboolean
hy_nevra_has_just_name(HyNevra nevra)
{
  return nevra->name != NULL &&
    nevra->epoch == -1 &&
    nevra->version == NULL &&
    nevra->release == NULL;
}
