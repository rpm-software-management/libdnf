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

/**
 * SECTION:dnf-solution
 * @short_description: solution object
 * @include: libhif.h
 *
 * This object represents a suggestion how to fix a failed goal
 */


#include "dnf-solution-private.h"
#include "dnf-solution.h"

struct _DnfSolution
{
    GObject            parent_instance;

    DnfSolutionAction  action;
    gchar             *old;
    gchar             *new;
};

G_DEFINE_TYPE(DnfSolution, dnf_solution, G_TYPE_OBJECT)

/*
 * dnf_solution_new:
 *
 * Creates a new #DnfSolution.
 *
 * Returns: a #DnfSolution
 *
 * Since: 0.7.0
 **/
DnfSolution *
dnf_solution_new(void)
{
    return g_object_new(DNF_TYPE_SOLUTION, NULL);
}

/**
 * dnf_solution_finalize:
 **/
static void
dnf_solution_finalize(GObject *object)
{
    DnfSolution *self = (DnfSolution *)object;

    g_free(self->old);
    g_free(self->new);

    G_OBJECT_CLASS(dnf_solution_parent_class)->finalize(object);
}

/**
 * dnf_solution_init:
 **/
static void
dnf_solution_init(DnfSolution *solution)
{
}

/**
 * dnf_solution_class_init:
 **/
static void
dnf_solution_class_init(DnfSolutionClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = dnf_solution_finalize;
}

/**
 * dnf_solution_get_action:
 * @solution: a #DnfSolution instance.
 *
 * Returns the solution action.
 *
 * Returns: an #DnfSolutionAction
 *
 * Since: 0.7.0
 */
DnfSolutionAction
dnf_solution_get_action(DnfSolution *solution)
{
    return solution->action;
}

/**
 * dnf_solution_get_old:
 * @solution: a #DnfSolution instance.
 *
 * Returns the solution old package description.
 *
 * Returns: string, or %NULL.
 *
 * Since: 0.7.0
 */
const gchar *
dnf_solution_get_old(DnfSolution *solution)
{
    return solution->old;
}

/**
 * dnf_solution_get_new:
 * @solution: a #DnfSolution instance.
 *
 * Returns the solution new package description.
 *
 * Returns: string, or %NULL.
 *
 * Since: 0.7.0
 */
const gchar *
dnf_solution_get_new(DnfSolution *solution)
{
    return solution->new;
}

/**
 * dnf_solution_set:
 * @solution: a #DnfSolution instance.
 *
 * Sets solution attributes.
 *
 * Returns: nothing
 *
 * Since: 0.7.0
 */
void
dnf_solution_set(DnfSolution *solution, DnfSolutionAction action,
                 const gchar *old, const gchar *new)
{
    solution->action = action;
    g_free(solution->old);
    solution->old = g_strdup(old);
    g_free(solution->new);
    solution->new = g_strdup(new);
}
