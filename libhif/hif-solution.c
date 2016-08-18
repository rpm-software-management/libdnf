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
 * SECTION:hif-solution
 * @short_description: solution object
 * @include: libhif.h
 *
 * This object represents a suggestion how to fix a failed goal
 */


#include "hif-solution-private.h"
#include "hif-solution.h"

typedef struct
{
    int      action;
    char    *old;
    char    *new;
} HifSolutionPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(HifSolution, hif_solution, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (hif_solution_get_instance_private (o))

/**
 * hif_solution_new:
 *
 * Creates a new #HifSolution.
 *
 * Returns:(transfer full): a #HifSolution
 *
 * Since: 0.7.0
 **/
HifSolution *
hif_solution_new(void)
{
    HifSolution *solution;
    solution = g_object_new(HIF_TYPE_SOLUTION, NULL);
    return HIF_SOLUTION(solution);
}

/**
 * hif_solution_finalize:
 **/
static void
hif_solution_finalize(GObject *object)
{
    HifSolution *solution = HIF_SOLUTION(object);
    HifSolutionPrivate *priv = GET_PRIVATE(solution);

    g_free(priv->old);
    g_free(priv->new);

    G_OBJECT_CLASS(hif_solution_parent_class)->finalize(object);
}

/**
 * hif_solution_init:
 **/
static void
hif_solution_init(HifSolution *solution)
{
}

/**
 * hif_solution_class_init:
 **/
static void
hif_solution_class_init(HifSolutionClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = hif_solution_finalize;
}

/**
 * hif_solution_get_action:
 * @solution: a #HifSolution instance.
 *
 * Returns the solution action.
 *
 * Returns: int
 *
 * Since: 0.7.0
 */
gint
hif_solution_get_action(HifSolution *solution)
{
    HifSolutionPrivate *priv = GET_PRIVATE(solution);
    return priv->action;
}

/**
 * hif_solution_get_old:
 * @solution: a #HifSolution instance.
 *
 * Returns the solution old package description.
 *
 * Returns: string, or %NULL.
 *
 * Since: 0.7.0
 */
const gchar *
hif_solution_get_old(HifSolution *solution)
{
    HifSolutionPrivate *priv = GET_PRIVATE(solution);
    return priv->old;
}

/**
 * hif_solution_get_new:
 * @solution: a #HifSolution instance.
 *
 * Returns the solution new package description.
 *
 * Returns: string, or %NULL.
 *
 * Since: 0.7.0
 */
const gchar *
hif_solution_get_new(HifSolution *solution)
{
    HifSolutionPrivate *priv = GET_PRIVATE(solution);
    return priv->new;
}

/**
 * hif_solution_set:
 * @solution: a #HifSolution instance.
 *
 * Sets solution attributes.
 *
 * Returns: nothing
 *
 * Since: 0.7.0
 */
void
hif_solution_set(HifSolution *solution, int action, const char *old, const char *new)
{
    HifSolutionPrivate *priv = GET_PRIVATE(solution);
    priv->action = action;
    g_free(priv->old);
    priv->old = g_strdup(old);
    g_free(priv->new);
    priv->new = g_strdup(new);
}
