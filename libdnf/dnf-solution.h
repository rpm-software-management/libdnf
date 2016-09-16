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

#define DNF_TYPE_SOLUTION (dnf_solution_get_type ())
G_DECLARE_FINAL_TYPE (DnfSolution, dnf_solution, DNF, SOLUTION, GObject)

typedef enum {
    DNF_SOLUTION_ACTION_ALLOW_INSTALL     = 1 << 1,
    DNF_SOLUTION_ACTION_ALLOW_REINSTALL   = 1 << 2,
    DNF_SOLUTION_ACTION_ALLOW_UPGRADE     = 1 << 3,
    DNF_SOLUTION_ACTION_ALLOW_DOWNGRADE   = 1 << 4,
    DNF_SOLUTION_ACTION_ALLOW_CHANGE      = 1 << 5,
    DNF_SOLUTION_ACTION_ALLOW_OBSOLETE    = 1 << 6,
    DNF_SOLUTION_ACTION_ALLOW_REPLACEMENT = 1 << 7,
    DNF_SOLUTION_ACTION_ALLOW_REMOVE      = 1 << 8,
    DNF_SOLUTION_ACTION_DO_NOT_INSTALL    = 1 << 9,
    DNF_SOLUTION_ACTION_DO_NOT_REMOVE     = 1 << 10,
    DNF_SOLUTION_ACTION_DO_NOT_OBSOLETE   = 1 << 11,
    DNF_SOLUTION_ACTION_DO_NOT_UPGRADE    = 1 << 12,
    DNF_SOLUTION_ACTION_BAD_SOLUTION      = 1 << 13
} DnfSolutionAction;

DnfSolutionAction dnf_solution_get_action (DnfSolution *solution);
const gchar *dnf_solution_get_old        (DnfSolution *solution);
const gchar *dnf_solution_get_new        (DnfSolution *solution);
void         dnf_solution_set            (DnfSolution *solution,
                                          DnfSolutionAction action,
                                          const gchar *old,
                                          const gchar *new);

G_END_DECLS
