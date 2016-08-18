/*
 * Copyright (C) 2016 Red Hat, Inc.
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

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define HIF_TYPE_SOLUTION (hif_solution_get_type ())
G_DECLARE_FINAL_TYPE (HifSolution, hif_solution, HIF, SOLUTION, GObject)

struct _HifSolutionClass
{
        GObjectClass            parent_class;
};

gint         hif_solution_get_action     (HifSolution *solution);
const gchar *hif_solution_get_old        (HifSolution *solution);
const gchar *hif_solution_get_new        (HifSolution *solution);
void         hif_solution_set            (HifSolution *solution,
                                          int action,
                                          const char *old,
                                          const char *new);

G_END_DECLS
