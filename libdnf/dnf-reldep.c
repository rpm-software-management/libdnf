/*
 * Copyright © 2016  Igor Gnatenko <ignatenko@redhat.com>
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

#include "dnf-reldep-private.h"

#include <solv/pool.h>

#include "dnf-sack-private.h"
#include "hy-iutil.h"

/**
 * SECTION: dnf-reldep
 * @short_description: RelDep
 *
 * #DnfReldep
 */

struct _DnfReldep
{
    GObject parent_instance;

    Pool *pool;
    Id r_id;
};

G_DEFINE_TYPE (DnfReldep, dnf_reldep, G_TYPE_OBJECT)

static gint
cmptype2relflags2 (DnfComparisonKind cmp_type)
{
    gint flags = 0;
    if (cmp_type & DNF_COMPARISON_EQ)
        flags |= REL_EQ;
    if (cmp_type & DNF_COMPARISON_LT)
        flags |= REL_LT;
    if (cmp_type & DNF_COMPARISON_GT)
        flags |= REL_GT;
    g_assert (flags);
    return flags;
}

/**
 * dnf_reldep_from_pool: (constructor)
 * @pool: Pool
 * @r_id: RelDep ID
 *
 * Returns: an #DnfReldep
 *
 * Since: 0.7.0
 */
DnfReldep *
dnf_reldep_from_pool (Pool *pool,
                      Id    r_id)
{
    DnfReldep *reldep = g_object_new (DNF_TYPE_RELDEP, NULL);
    reldep->pool = pool;
    reldep->r_id = r_id;
    return reldep;
}

/**
 * dnf_reldep_new:
 * @sack: a #DnfSack
 * @name: Name
 * @cmp_type: Comparison type
 * @evr: (nullable): EVR
 *
 * Returns: an #DnfReldep, or %NULL
 *
 * Since: 0.7.0
 */
DnfReldep *
dnf_reldep_new (DnfSack           *sack,
                const gchar       *name,
                DnfComparisonKind  cmp_type,
                const gchar       *evr)
{
    Pool *pool = dnf_sack_get_pool (sack);
    Id id = pool_str2id (pool, name, 0);

    if (id == STRID_NULL || id == STRID_EMPTY)
        return NULL;

    if (evr) {
        g_assert (cmp_type);
        Id ievr = pool_str2id (pool, evr, 1);
        int flags = cmptype2relflags2 (cmp_type);
        id = pool_rel2id (pool, id, ievr, flags, 1);
    }

    return dnf_reldep_from_pool (pool, id);
}

static void
dnf_reldep_finalize (GObject *object)
{
    G_OBJECT_CLASS (dnf_reldep_parent_class)->finalize (object);
}

static void
dnf_reldep_class_init (DnfReldepClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = dnf_reldep_finalize;
}

static void
dnf_reldep_init (DnfReldep *self)
{
}

/**
 * dnf_reldep_to_string:
 * @reldep: a #DnfReldep
 *
 * Returns: a string
 *
 * Since: 0.7.0
 */
const gchar *
dnf_reldep_to_string (DnfReldep *reldep)
{
    return pool_dep2str(reldep->pool, reldep->r_id);
}

Id
dnf_reldep_get_id (DnfReldep *reldep)
{
    return reldep->r_id;
}
