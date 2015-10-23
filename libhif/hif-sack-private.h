/*
 * Copyright (C) 2012-2015 Red Hat, Inc.
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

#ifndef HY_SACK_INTERNAL_H
#define HY_SACK_INTERNAL_H

#include <stdio.h>
#include <solv/pool.h>

#include "hif-sack.h"

typedef Id  (*hif_sack_running_kernel_fn_t) (HifSack    *sack);

void         hif_sack_make_provides_ready   (HifSack    *sack);
Id           hif_sack_running_kernel        (HifSack    *sack);
int          hif_sack_knows                 (HifSack    *sack,
                                             const char *name,
                                             const char *version,
                                             int         flags);
void         hif_sack_recompute_considered  (HifSack    *sack);
Pool        *hif_sack_get_pool              (HifSack    *sack);
Id           hif_sack_last_solvable         (HifSack    *sack);

Queue       *hif_sack_get_installonly       (HifSack    *sack);
void         hif_sack_set_running_kernel_fn (HifSack    *sack,
                                             hif_sack_running_kernel_fn_t fn);

#endif // HY_SACK_INTERNAL_H
