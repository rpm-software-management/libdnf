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

#include "hif-reldep-private.h"

#include <solv/pool.h>

#include "hif-sack-private.h"
#include "hy-iutil.h"

/**
 * SECTION: hif-reldep
 * @short_description: RelDep
 *
 * #HifReldep
 */

struct _HifReldep
{
    GObject parent_instance;

    Pool *pool;
    Id r_id;
};

G_DEFINE_TYPE (HifReldep, hif_reldep, G_TYPE_OBJECT)

static gint
cmptype2relflags2 (HifComparisonKind cmp_type)
{
    gint flags = 0;
    if (cmp_type & HIF_COMPARISON_EQ)
        flags |= REL_EQ;
    if (cmp_type & HIF_COMPARISON_LT)
        flags |= REL_LT;
    if (cmp_type & HIF_COMPARISON_GT)
        flags |= REL_GT;
    g_assert (flags);
    return flags;
}

/**
 * hif_reldep_from_pool: (constructor)
 * @pool: Pool
 * @r_id: RelDep ID
 *
 * Returns: an #HifReldep
 *
 * Since: 0.7.0
 */
HifReldep *
hif_reldep_from_pool (Pool *pool,
                      Id    r_id)
{
    HifReldep *reldep = g_object_new (HIF_TYPE_RELDEP, NULL);
    reldep->pool = pool;
    reldep->r_id = r_id;
    return reldep;
}

/**
 * hif_reldep_new:
 * @sack: a #HifSack
 * @name: Name
 * @cmp_type: Comparison type
 * @evr: (nullable): EVR
 *
 * Returns: an #HifReldep, or %NULL
 *
 * Since: 0.7.0
 */
HifReldep *
hif_reldep_new (HifSack           *sack,
                const gchar       *name,
                HifComparisonKind  cmp_type,
                const gchar       *evr)
{
    Pool *pool = hif_sack_get_pool (sack);
    Id id = pool_str2id (pool, name, 0);

    if (id == STRID_NULL || id == STRID_EMPTY)
        return NULL;

    if (evr) {
        g_assert (cmp_type);
        Id ievr = pool_str2id (pool, evr, 1);
        int flags = cmptype2relflags2 (cmp_type);
        id = pool_rel2id (pool, id, ievr, flags, 1);
    }

    return hif_reldep_from_pool (pool, id);
}

static void
hif_reldep_finalize (GObject *object)
{
    G_OBJECT_CLASS (hif_reldep_parent_class)->finalize (object);
}

static void
hif_reldep_class_init (HifReldepClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = hif_reldep_finalize;
}

static void
hif_reldep_init (HifReldep *self)
{
}

/**
 * hif_reldep_to_string:
 * @reldep: a #HifReldep
 *
 * Returns: a string
 *
 * Since: 0.7.0
 */
const gchar *
hif_reldep_to_string (HifReldep *reldep)
{
    return pool_dep2str(reldep->pool, reldep->r_id);
}

Id
hif_reldep_get_id (HifReldep *reldep)
{
    return reldep->r_id;
}
