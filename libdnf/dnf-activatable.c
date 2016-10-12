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

#include "dnf-activatable.h"

/**
 * SECTION:dnf-activatable
 * @short_description: Interface for activatable plugins.
 * @see_also: #DnfContext.
 */

G_DEFINE_INTERFACE (DnfActivatable, dnf_activatable, G_TYPE_OBJECT)

static void dnf_activatable_default_init (DnfActivatableInterface *iface)
{
}

/**
 * dnf_activatable_activate:
 * @activatable: a #DnfActivatable
 *
 * Activates the extension. This is the starting point where
 * the implementation does everything it needs to do. Avoid
 * doing it earlier than this call.
 *
 * This function is called after the extension is loaded and
 * the signals are connected. If you want to do anything before
 * that, the _init function should be used instead.
 */
void
dnf_activatable_activate (DnfActivatable *activatable)
{
    g_return_if_fail (DNF_IS_ACTIVATABLE (activatable));

    DnfActivatableInterface *iface = DNF_ACTIVATABLE_GET_IFACE (activatable);
    g_return_if_fail (iface->activate != NULL);
    iface->activate (activatable);
}

/**
 * dnf_activatable_deactivate:
 * @activatable: a #DnfActivatable
 *
 * Deactivates the extension.
 */
void
dnf_activatable_deactivate (DnfActivatable *activatable)
{
    g_return_if_fail (DNF_IS_ACTIVATABLE (activatable));

    DnfActivatableInterface *iface = DNF_ACTIVATABLE_GET_IFACE (activatable);
    g_return_if_fail (iface->deactivate != NULL);
    iface->deactivate (activatable);
}

/**
 * dnf_activatable_setup:
 * @activatable: a #DnfActivatable
 * @ctx: a #DnfContext
 * @error: a #GError
 *
 * Setup the extension. Called when dnf_context_setup () is executed.
 *
 * Returns: %TRUE if callback is not implemented in plugin or ran successfully,
 *          %FALSE otherwise.
 */
gboolean
dnf_activatable_setup (DnfActivatable  *activatable,
                       DnfContext      *ctx,
                       GError         **error)
{
    g_return_val_if_fail (DNF_IS_ACTIVATABLE (activatable), TRUE);

    DnfActivatableInterface *iface = DNF_ACTIVATABLE_GET_IFACE (activatable);
    g_return_val_if_fail (iface->setup != NULL, TRUE);
    return iface->setup (activatable, ctx, error);
}
