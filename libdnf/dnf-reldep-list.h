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

#pragma once

#include <glib-object.h>

#include "dnf-reldep.h"
#include "dnf-types.h"

G_BEGIN_DECLS

#define DNF_TYPE_RELDEP_LIST (dnf_reldep_list_get_type())

G_DECLARE_FINAL_TYPE (DnfReldepList, dnf_reldep_list, DNF, RELDEP_LIST, GObject)

DnfReldepList *dnf_reldep_list_new   (DnfSack       *sack);
void           dnf_reldep_list_add   (DnfReldepList *reldep_list,
                                      DnfReldep     *reldep);
gint           dnf_reldep_list_count (DnfReldepList *reldep_list);
DnfReldep     *dnf_reldep_list_index (DnfReldepList *reldep_list,
                                      gint           index);


G_END_DECLS
