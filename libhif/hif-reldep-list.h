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

#include "hif-reldep.h"
#include "hif-types.h"

G_BEGIN_DECLS

#define HIF_TYPE_RELDEP_LIST (hif_reldep_list_get_type())

G_DECLARE_FINAL_TYPE (HifReldepList, hif_reldep_list, HIF, RELDEP_LIST, GObject)

HifReldepList *hif_reldep_list_new   (HifSack       *sack);
void           hif_reldep_list_add   (HifReldepList *reldep_list,
                                      HifReldep     *reldep);
gint           hif_reldep_list_count (HifReldepList *reldep_list);
HifReldep     *hif_reldep_list_index (HifReldepList *reldep_list,
                                      gint           index);


G_END_DECLS
