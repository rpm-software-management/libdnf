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

#ifndef LIBDNF_RELDEPLIST_H
#define LIBDNF_RELDEPLIST_H

#include "dnf-reldep.h"
#include "dnf-types.h"

#ifdef __cplusplus
extern "C" {
#endif

DnfReldepList *dnf_reldep_list_new(DnfSack *sack);
DnfReldep *dnf_reldep_list_index(DnfReldepList *reldep_list, gint index);
gint dnf_reldep_list_count(DnfReldepList *reldep_list);
void dnf_reldep_list_add(DnfReldepList *reldep_list, DnfReldep *reldep);
void dnf_reldep_list_extend(DnfReldepList *rl1, DnfReldepList *rl2);

#ifdef __cplusplus
}
#endif

#endif // LIBDNF_RELDEPLIST_H
