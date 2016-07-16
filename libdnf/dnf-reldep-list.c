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

#include "dnf-reldep-list-private.h"

#include "dnf-reldep-private.h"
#include "dnf-sack-private.h"

/**
 * SECTION: DnfReldepList
 * @short_description: RelDep list
 *
 * #DnfReldepList
 */
struct _DnfReldepList
{
    GObject parent_instance;

    Pool *pool;
    Queue queue;
};

G_DEFINE_TYPE (DnfReldepList, dnf_reldep_list, G_TYPE_OBJECT)

/**
 * dnf_reldep_list_new:
 * @sack: a #DnfSack
 *
 * Returns: an #DnfReldepList
 *
 * Since: 0.7.0
 */
DnfReldepList *
dnf_reldep_list_new (DnfSack *sack)
{
    DnfReldepList *reldep_list = g_object_new (DNF_TYPE_RELDEP_LIST, NULL);
    reldep_list->pool = dnf_sack_get_pool (sack);
    queue_init (&reldep_list->queue);
    return reldep_list;
}

/**
 * dnf_reldep_list_from_queue: (constructor)
 * @pool: Pool
 * @queue: Queue
 *
 * Returns: an #DnfReldepList
 *
 * Since: 0.7.0
 */
DnfReldepList *
dnf_reldep_list_from_queue (Pool *pool, Queue queue)
{
    DnfReldepList *reldep_list = g_object_new (DNF_TYPE_RELDEP_LIST, NULL);
    reldep_list->pool = pool;
    queue_init_clone (&reldep_list->queue, &queue);
    return reldep_list;
}

static void
dnf_reldep_list_finalize (GObject *object)
{
    DnfReldepList *self = (DnfReldepList *)object;

    queue_free (&self->queue);

    G_OBJECT_CLASS (dnf_reldep_list_parent_class)->finalize (object);
}

static void
dnf_reldep_list_class_init (DnfReldepListClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = dnf_reldep_list_finalize;
}

static void
dnf_reldep_list_init (DnfReldepList *self)
{
}

/**
 * dnf_reldep_list_add:
 * @reldep_list: a #DnfReldepList
 * @reldep: a #DnfReldep
 *
 * Returns: Noting.
 *
 * Since: 0.7.0
 */
void
dnf_reldep_list_add (DnfReldepList *reldep_list,
                     DnfReldep     *reldep)
{
    queue_push (&reldep_list->queue, dnf_reldep_get_id (reldep));
}

/**
 * dnf_reldep_list_count:
 * @reldep_list: a #DnfReldepList
 *
 * Returns: Number of RelDeps in queue.
 *
 * Since: 0.7.0
 */
gint
dnf_reldep_list_count (DnfReldepList *reldep_list)
{
    return reldep_list->queue.count;
}

/**
 * dnf_reldep_list_index:
 * @reldep_list: a #DnfReldepList
 * @index: Index
 *
 * Returns: (transfer full): an #DnfReldep
 *
 * Since: 0.7.0
 */
DnfReldep *
dnf_reldep_list_index (DnfReldepList *reldep_list,
                       gint           index)
{
    Id r_id = reldep_list->queue.elements[index];
    return dnf_reldep_from_pool (reldep_list->pool, r_id);
}

/**
 * dnf_reldep_list_extend:
 * @rl1: original #DnfReldepList
 * @rl2: additional #DnfReldepList
 *
 * Extends @rl1 with elements from @rl2.
 *
 * Returns: Nothing.
 *
 * Since: 0.7.0
 */
void
dnf_reldep_list_extend (DnfReldepList *rl1,
                        DnfReldepList *rl2)
{
    queue_insertn (&rl1->queue, 0, rl2->queue.count, rl2->queue.elements);
}
