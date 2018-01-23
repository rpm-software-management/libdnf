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

#include "dnf-reldep-list.h"
#include "dnf-sack-private.hpp"
#include "repo/solvable/Dependency.hpp"
#include "repo/solvable/DependencyContainer.hpp"

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
    return new DependencyContainer(sack);
}

/**
 * dnf_reldep_list_add:
 * @reldep_list: a #DnfReldepList
 * @reldep: a #DnfReldep
 *
 * Returns: Nothing.
 *
 * Since: 0.7.0
 */
void
dnf_reldep_list_add (DnfReldepList *reldep_list,
                     DnfReldep     *reldep)
{
    reldep_list->add(reldep);
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
    return reldep_list->count();
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
    return reldep_list->getPtr(index);
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
    rl1->extend(rl2);
}
