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

#include "dnf-activatable.h"

#include <libpeas/peas.h>

G_BEGIN_DECLS

#define SUBSCRIPTION_MANAGER_TYPE_PLUGIN subscription_manager_plugin_get_type ()

G_DECLARE_FINAL_TYPE (SubscriptionManagerPlugin, subscription_manager_plugin, SUBSCRIPTION_MANAGER, PLUGIN, PeasExtensionBase)

G_MODULE_EXPORT void peas_register_types (PeasObjectModule *module);

G_END_DECLS
