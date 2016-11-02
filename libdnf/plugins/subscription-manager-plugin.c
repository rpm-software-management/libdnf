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

#include "subscription-manager-plugin.h"

#include <rhsm/rhsm.h>

struct _SubscriptionManagerPlugin
{
    PeasExtensionBase parent_instance;
};

static void subscription_manager_plugin_iface_init (DnfActivatableInterface *iface);

G_DEFINE_DYNAMIC_TYPE_EXTENDED (SubscriptionManagerPlugin,
                                subscription_manager_plugin,
                                PEAS_TYPE_EXTENSION_BASE,
                                0,
                                G_IMPLEMENT_INTERFACE (DNF_TYPE_ACTIVATABLE,
                                                       subscription_manager_plugin_iface_init))

static void
subscription_manager_plugin_init (SubscriptionManagerPlugin *plugin)
{
}

static void
subscription_manager_plugin_finalize (GObject *object)
{
    G_OBJECT_CLASS (subscription_manager_plugin_parent_class)->finalize (object);
}

static void
subscription_manager_plugin_activate (DnfActivatable *activatable)
{
}

static void
subscription_manager_plugin_deactivate (DnfActivatable *activatable)
{
}

static gboolean
subscription_manager_plugin_setup (DnfActivatable  *activatable,
                                   DnfContext      *ctx,
                                   GError         **error)
{
    g_autoptr(RHSMContext) rhsm_ctx = rhsm_context_new ();
    g_autofree gchar *repofname = g_build_filename (dnf_context_get_repo_dir (ctx),
                                                    "redhat.repo",
                                                    NULL);
    g_autoptr(GKeyFile) repofile = rhsm_utils_yum_repo_from_context (rhsm_ctx);

    if (!g_key_file_save_to_file (repofile, repofname, error))
        return FALSE;

    return TRUE;
}

static void
subscription_manager_plugin_class_init (SubscriptionManagerPluginClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = subscription_manager_plugin_finalize;
}

static void
subscription_manager_plugin_iface_init (DnfActivatableInterface *iface)
{
    iface->activate = subscription_manager_plugin_activate;
    iface->deactivate = subscription_manager_plugin_deactivate;

    iface->setup = subscription_manager_plugin_setup;
}

static void
subscription_manager_plugin_class_finalize (SubscriptionManagerPluginClass *klass)
{
}

G_MODULE_EXPORT void
peas_register_types (PeasObjectModule *module)
{
    subscription_manager_plugin_register_type (G_TYPE_MODULE (module));

    peas_object_module_register_extension_type (module,
                                                DNF_TYPE_ACTIVATABLE,
                                                SUBSCRIPTION_MANAGER_TYPE_PLUGIN);
}
