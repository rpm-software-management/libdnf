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

typedef struct
{
    int      action;
    char    *old;
    char    *new;
} DnfSolutionPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(DnfSolution, dnf_solution, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (dnf_solution_get_instance_private (o))

/**
 * dnf_solution_new:
 *
 * Creates a new #DnfSolution.
 *
 * Returns:(transfer full): a #DnfSolution
 *
 * Since: 0.7.0
 **/
DnfSolution *
dnf_solution_new(void)
{
    DnfSolution *solution;
    solution = g_object_new(DNF_TYPE_SOLUTION, NULL);
    return DNF_SOLUTION(solution);
}

/**
 * dnf_solution_finalize:
 **/
static void
dnf_solution_finalize(GObject *object)
{
    DnfSolution *solution = DNF_SOLUTION(object);
    DnfSolutionPrivate *priv = GET_PRIVATE(solution);

    g_free(priv->old);
    g_free(priv->new);

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
 * Returns: DnfGoalSolutionActions
 *
 * Since: 0.7.0
 */
DnfGoalSolutionActions
dnf_solution_get_action(DnfSolution *solution)
{
    DnfSolutionPrivate *priv = GET_PRIVATE(solution);
    return priv->action;
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
    DnfSolutionPrivate *priv = GET_PRIVATE(solution);
    return priv->old;
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
    DnfSolutionPrivate *priv = GET_PRIVATE(solution);
    return priv->new;
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
dnf_solution_set(DnfSolution *solution, int action, const char *old, const char *new)
{
    DnfSolutionPrivate *priv = GET_PRIVATE(solution);
    priv->action = action;
    g_free(priv->old);
    priv->old = g_strdup(old);
    g_free(priv->new);
    priv->new = g_strdup(new);
}
