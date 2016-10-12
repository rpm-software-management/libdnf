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

#include "dnf-context.h"

G_BEGIN_DECLS

#define DNF_TYPE_ACTIVATABLE dnf_activatable_get_type ()

G_DECLARE_INTERFACE (DnfActivatable, dnf_activatable, DNF, ACTIVATABLE, GObject)

struct _DnfActivatableInterface
{
    GTypeInterface parent_iface;

    void (*activate)   (DnfActivatable  *activatable);
    void (*deactivate) (DnfActivatable  *activatable);

    gboolean (*setup)  (DnfActivatable  *activatable,
                        DnfContext      *ctx,
                        GError         **error);
};

void     dnf_activatable_activate   (DnfActivatable  *activatable);
void     dnf_activatable_deactivate (DnfActivatable  *activatable);
gboolean dnf_activatable_setup      (DnfActivatable  *activatable,
                                     DnfContext      *ctx,
                                     GError         **error);

G_END_DECLS
