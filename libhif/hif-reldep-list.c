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

#include "hif-reldep-list-private.h"

#include "hif-reldep-private.h"
#include "hif-sack-private.h"

/**
 * SECTION: HifReldepList
 * @short_description: RelDep list
 *
 * #HifReldepList
 */
struct _HifReldepList
{
    GObject parent_instance;

    Pool *pool;
    Queue queue;
};

G_DEFINE_TYPE (HifReldepList, hif_reldep_list, G_TYPE_OBJECT)

/**
 * hif_reldep_list_new:
 * @sack: a #HifSack
 *
 * Returns: an #HifReldepList
 *
 * Since: 0.7.0
 */
HifReldepList *
hif_reldep_list_new (HifSack *sack)
{
    HifReldepList *reldep_list = g_object_new (HIF_TYPE_RELDEP_LIST, NULL);
    reldep_list->pool = hif_sack_get_pool (sack);
    queue_init (&reldep_list->queue);
    return reldep_list;
}

/**
 * hif_reldep_list_from_queue: (constructor)
 * @pool: Pool
 * @queue: Queue
 *
 * Returns: an #HifReldepList
 *
 * Since: 0.7.0
 */
HifReldepList *
hif_reldep_list_from_queue (Pool *pool, Queue queue)
{
    HifReldepList *reldep_list = g_object_new (HIF_TYPE_RELDEP_LIST, NULL);
    reldep_list->pool = pool;
    queue_init_clone (&reldep_list->queue, &queue);
    return reldep_list;
}

static void
hif_reldep_list_finalize (GObject *object)
{
    HifReldepList *self = (HifReldepList *)object;

    queue_free (&self->queue);

    G_OBJECT_CLASS (hif_reldep_list_parent_class)->finalize (object);
}

static void
hif_reldep_list_class_init (HifReldepListClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = hif_reldep_list_finalize;
}

static void
hif_reldep_list_init (HifReldepList *self)
{
}

/**
 * hif_reldep_list_add:
 * @reldep_list: a #HifReldepList
 * @reldep: a #HifReldep
 *
 * Returns: Noting.
 *
 * Since: 0.7.0
 */
void
hif_reldep_list_add (HifReldepList *reldep_list,
                     HifReldep     *reldep)
{
    queue_push (&reldep_list->queue, hif_reldep_get_id (reldep));
}

/**
 * hif_reldep_list_count:
 * @reldep_list: a #HifReldepList
 *
 * Returns: Number of RelDeps in queue.
 *
 * Since: 0.7.0
 */
gint
hif_reldep_list_count (HifReldepList *reldep_list)
{
    return reldep_list->queue.count;
}

/**
 * hif_reldep_list_index:
 * @reldep_list: a #HifReldepList
 * @index: Index
 *
 * Returns: (transfer full): an #HifReldep
 *
 * Since: 0.7.0
 */
HifReldep *
hif_reldep_list_index (HifReldepList *reldep_list,
                       gint           index)
{
    Id r_id = reldep_list->queue.elements[index];
    return hif_reldep_from_pool (reldep_list->pool, r_id);
}

/**
 * hif_reldep_list_extend:
 * @rl1: original #HifReldepList
 * @rl2: additional #HifReldepList
 *
 * Extends @rl1 with elements from @rl2.
 *
 * Returns: Nothing.
 *
 * Since: 0.7.0
 */
void
hif_reldep_list_extend (HifReldepList *rl1,
                        HifReldepList *rl2)
{
    queue_insertn (&rl1->queue, 0, rl2->queue.count, rl2->queue.elements);
}
