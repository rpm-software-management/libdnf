/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2010-2015 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 *(at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include <gio/gio.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include "libdnf.h"
#include "dnf-utils.h"

#define DNF_ERROR_INVALID_ARGUMENTS     0
#define DNF_ERROR_NO_SUCH_CMD           1

typedef struct {
    DnfContext      *context;
    GOptionContext  *option_context;
    GPtrArray       *cmd_array;
    gboolean         value_only;
    gchar          **filters;
} DnfUtilPrivate;

typedef gboolean(*DnfUtilPrivateCb) (DnfUtilPrivate    *cmd,
                                     gchar        **values,
                                     GError        **error);

typedef struct {
    gchar               *name;
    gchar               *arguments;
    gchar               *description;
    DnfUtilPrivateCb     callback;
} DnfUtilItem;

/**
 * dnf_cmd_item_free:
 **/
static void
dnf_cmd_item_free(DnfUtilItem *item)
{
    g_free(item->name);
    g_free(item->arguments);
    g_free(item->description);
    g_free(item);
}

/*
 * dnf_sort_command_name_cb:
 */
static gint
dnf_sort_command_name_cb(DnfUtilItem **item1, DnfUtilItem **item2)
{
    return g_strcmp0((*item1)->name,(*item2)->name);
}

/**
 * dnf_cmd_add:
 **/
static void
dnf_cmd_add(GPtrArray *array,
            const gchar *name,
            const gchar *arguments,
            const gchar *description,
            DnfUtilPrivateCb callback)
{
    guint i;
    DnfUtilItem *item;
    g_auto(GStrv) names = NULL;

    g_return_if_fail(name != NULL);
    g_return_if_fail(description != NULL);
    g_return_if_fail(callback != NULL);

    /* add each one */
    names = g_strsplit(name, ",", -1);
    for (i = 0; names[i] != NULL; i++) {
        item = g_new0(DnfUtilItem, 1);
        item->name = g_strdup(names[i]);
        if (i == 0) {
            item->description = g_strdup(description);
        } else {
            item->description = g_strdup_printf("Alias to %s",
                                 names[0]);
        }
        item->arguments = g_strdup(arguments);
        item->callback = callback;
        g_ptr_array_add(array, item);
    }
}

/**
 * dnf_cmd_get_descriptions:
 **/
static gchar *
dnf_cmd_get_descriptions(GPtrArray *array)
{
    guint i;
    guint j;
    guint len;
    const guint max_len = 35;
    DnfUtilItem *item;
    GString *string;

    /* print each command */
    string = g_string_new("");
    for (i = 0; i < array->len; i++) {
        item = g_ptr_array_index(array, i);
        g_string_append(string, "  ");
        g_string_append(string, item->name);
        len = strlen(item->name) + 2;
        if (item->arguments != NULL) {
            g_string_append(string, " ");
            g_string_append(string, item->arguments);
            len += strlen(item->arguments) + 1;
        }
        if (len < max_len) {
            for (j = len; j < max_len + 1; j++)
                g_string_append_c(string, ' ');
            g_string_append(string, item->description);
            g_string_append_c(string, '\n');
        } else {
            g_string_append_c(string, '\n');
            for (j = 0; j < max_len + 1; j++)
                g_string_append_c(string, ' ');
            g_string_append(string, item->description);
            g_string_append_c(string, '\n');
        }
    }

    /* remove trailing newline */
    if (string->len > 0)
        g_string_set_size(string, string->len - 1);

    return g_string_free(string, FALSE);
}

/**
 * dnf_cmd_run:
 **/
static gboolean
dnf_cmd_run(DnfUtilPrivate *priv, const gchar *command, gchar **values, GError **error)
{
    guint i;
    DnfUtilItem *item;
    g_autoptr(GString) string = NULL;

    /* find command */
    for (i = 0; i < priv->cmd_array->len; i++) {
        item = g_ptr_array_index(priv->cmd_array, i);
        if (g_strcmp0(item->name, command) == 0)
            return item->callback(priv, values, error);
    }

    /* not found */
    string = g_string_new("");
    g_string_append(string, "Command not found, valid commands are:\n");
    for (i = 0; i < priv->cmd_array->len; i++) {
        item = g_ptr_array_index(priv->cmd_array, i);
        g_string_append_printf(string, " * %s %s\n",
                               item->name,
                               item->arguments ? item->arguments : "");
    }
    g_set_error_literal(error, DNF_ERROR, DNF_ERROR_NO_SUCH_CMD, string->str);
    return FALSE;
}

/**
 * dnf_cmd_install:
 **/
static gboolean
dnf_cmd_install(DnfUtilPrivate *priv, gchar **values, GError **error)
{
    guint i;

    if (g_strv_length(values) < 1) {
        g_set_error_literal(error,
                            DNF_ERROR,
                            DNF_ERROR_INVALID_ARGUMENTS,
                            "Not enough arguments, "
                            "expected package or group name");
        return FALSE;
    }

    /* install each package */
    if (!dnf_context_setup(priv->context, NULL, error))
        return FALSE;
    for (i = 0; values[i] != NULL; i++) {
        if (!dnf_context_install(priv->context, values[i], error))
            return FALSE;
    }
    return dnf_context_run(priv->context, NULL, error);
}

/**
 * dnf_cmd_reinstall:
 **/
static gboolean
dnf_cmd_reinstall(DnfUtilPrivate *priv, gchar **values, GError **error)
{
    guint i;
    DnfTransaction *transaction;

    if (g_strv_length(values) < 1) {
        g_set_error_literal(error,
                            DNF_ERROR,
                            DNF_ERROR_INVALID_ARGUMENTS,
                            "Not enough arguments, "
                            "expected package or group name");
        return FALSE;
    }

    /* install each package */
    if (!dnf_context_setup(priv->context, NULL, error))
        return FALSE;

    /* relax duplicate checking */
    transaction = dnf_context_get_transaction(priv->context);
    dnf_transaction_set_flags(transaction, DNF_TRANSACTION_FLAG_ALLOW_REINSTALL);

    /* reinstall each package */
    for (i = 0; values[i] != NULL; i++) {
        if (!dnf_context_remove(priv->context, values[i], error))
            return FALSE;
        if (!dnf_context_install(priv->context, values[i], error))
            return FALSE;
    }
    return dnf_context_run(priv->context, NULL, error);
}

/**
 * dnf_cmd_remove:
 **/
static gboolean
dnf_cmd_remove(DnfUtilPrivate *priv, gchar **values, GError **error)
{
    guint i;

    if (g_strv_length(values) < 1) {
        g_set_error_literal(error,
                            DNF_ERROR,
                            DNF_ERROR_INVALID_ARGUMENTS,
                            "Not enough arguments, "
                            "expected package or group name");
        return FALSE;
    }

    /* remove each package */
    if (!dnf_context_setup(priv->context, NULL, error))
        return FALSE;
    for (i = 0; values[i] != NULL; i++) {
        if (!dnf_context_remove(priv->context, values[i], error))
            return FALSE;
    }
    return dnf_context_run(priv->context, NULL, error);
}

/**
 * dnf_cmd_refresh_source:
 **/
static gboolean
dnf_cmd_refresh_source(DnfUtilPrivate *priv, DnfRepo *src, DnfState *state, GError **error)
{
    DnfState *state_local;
    gboolean ret;
    guint cache_age;
    g_autoptr(GError) error_local = NULL;

    /* set steps */
    if (!dnf_state_set_steps(state, error, 50, 50, -1))
        return FALSE;

    /* check source */
    state_local = dnf_state_get_child(state);
    g_print("Checking %s\n", dnf_repo_get_id(src));
    cache_age = dnf_context_get_cache_age(priv->context);
    ret = dnf_repo_check(src, cache_age, state_local, &error_local);
    if (ret)
        return dnf_state_finished(state, error);

    /* done */
    if (!dnf_state_done(state, error))
        return FALSE;

    /* print error to console and continue */
    g_print("Failed to check %s: %s\n",
         dnf_repo_get_id(src),
         error_local->message);
    g_clear_error(&error_local);
    if (!dnf_state_finished(state_local, error))
        return FALSE;

    /* actually update source */
    state_local = dnf_state_get_child(state);
    g_print("Updating %s\n", dnf_repo_get_id(src));
    if (!dnf_repo_update(src, DNF_REPO_UPDATE_FLAG_IMPORT_PUBKEY,
                state_local, &error_local)) {
        if (g_error_matches(error_local,
                            DNF_ERROR,
                            DNF_ERROR_CANNOT_FETCH_SOURCE)) {
            g_print("Skipping repo: %s\n", error_local->message);
            return dnf_state_finished(state, error);
        }
        g_propagate_error(error, error_local);
        return FALSE;
    }

    /* done */
    return dnf_state_done(state, error);
}

/**
 * dnf_cmd_refresh:
 **/
static gboolean
dnf_cmd_refresh(DnfUtilPrivate *priv, gchar **values, GError **error)
{
    GPtrArray *sources;
    DnfRepo *src;
    DnfState *state;
    DnfState *state_local;
    guint i;

    if (!dnf_context_setup(priv->context, NULL, error))
        return FALSE;

    /* check and refresh each source in turn */
    sources = dnf_context_get_repos(priv->context);
    state = dnf_context_get_state(priv->context);
    dnf_state_set_number_steps(state, sources->len);
    for (i = 0; i < sources->len; i++) {
        src = g_ptr_array_index(sources, i);
        if (dnf_repo_get_enabled(src) == DNF_REPO_ENABLED_NONE)
            continue;
        state_local = dnf_state_get_child(state);
        if (!dnf_cmd_refresh_source(priv, src, state_local, error))
            return FALSE;
        if (!dnf_state_done(state, error))
            return FALSE;
    }
    return TRUE;
}

/**
 * dnf_cmd_clean:
 **/
static gboolean
dnf_cmd_clean(DnfUtilPrivate *priv, gchar **values, GError **error)
{
    GPtrArray *sources;
    DnfRepo *src;
    DnfState *state;
    guint i;

    if (!dnf_context_setup(priv->context, NULL, error))
        return FALSE;

    /* clean each source in turn */
    sources = dnf_context_get_repos(priv->context);
    state = dnf_context_get_state(priv->context);
    dnf_state_set_number_steps(state, sources->len);
    for (i = 0; i < sources->len; i++) {
        src = g_ptr_array_index(sources, i);
        if (!dnf_repo_clean(src, error))
            return FALSE;
        if (!dnf_state_done(state, error))
            return FALSE;
    }
    return TRUE;
}

/**
 * dnf_cmd_update:
 **/
static gboolean
dnf_cmd_update(DnfUtilPrivate *priv, gchar **values, GError **error)
{
    guint i;

    if (g_strv_length(values) < 1) {
        g_set_error_literal(error,
                            DNF_ERROR,
                            DNF_ERROR_INVALID_ARGUMENTS,
                            "Not enough arguments, "
                            "expected package or group name");
        return FALSE;
    }

    /* update each package */
    if (!dnf_context_setup(priv->context, NULL, error))
        return FALSE;
    for (i = 0; values[i] != NULL; i++) {
        if (!dnf_context_update(priv->context, values[i], error))
            return FALSE;
    }
    return dnf_context_run(priv->context, NULL, error);
}

/**
 * dnf_cmd_ignore_cb:
 **/
static void
dnf_cmd_ignore_cb(const gchar *log_domain, GLogLevelFlags log_level,
                  const gchar *message, gpointer user_data)
{
}

/**
 * main:
 **/
int
main(int argc, char *argv[])
{
    DnfUtilPrivate *priv;
    gboolean ret;
    gboolean verbose = FALSE;
    gboolean version = FALSE;
    gboolean opt_disable_yumdb = FALSE;
    guint retval = 1;
    g_autofree gchar *opt_root = NULL;
    g_autofree gchar *opt_reposdir = NULL;
    g_autofree gchar *opt_cachedir = NULL;

    const GOptionEntry options[] = {
        { "reposdir", 0, 0, G_OPTION_ARG_STRING, &opt_reposdir,
            "Directory for yum repository files", NULL },
        { "root", 0, 0, G_OPTION_ARG_STRING, &opt_root,
            "Installation root", NULL },
        { "cachedir", 0, 0, G_OPTION_ARG_STRING, &opt_cachedir,
            "Directory for caches", NULL },
        { "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
            "Show extra debugging information", NULL },
        { "disable-yumdb", 'v', 0, G_OPTION_ARG_NONE, &opt_disable_yumdb,
            "Disable writing updates to the yumdb", NULL },
        { "version", '\0', 0, G_OPTION_ARG_NONE, &version,
            "Show version", NULL },
        { NULL}
    };
    g_autoptr(GError) error = NULL;
    g_autofree gchar *cmd_descriptions = NULL;
    g_autofree gchar *filter = NULL;

    setlocale(LC_ALL, "");

    /* create helper object */
    priv = g_new0(DnfUtilPrivate, 1);

    /* add commands */
    priv->cmd_array = g_ptr_array_new_with_free_func((GDestroyNotify) dnf_cmd_item_free);
    dnf_cmd_add(priv->cmd_array,
                "install", "[pkgname]",
                "Install a package or group name",
                dnf_cmd_install);
    dnf_cmd_add(priv->cmd_array,
                "reinstall", "[pkgname]",
                "Reinstall a package or group name",
                dnf_cmd_reinstall);
    dnf_cmd_add(priv->cmd_array,
                "remove", "[pkgname]",
                "Remove a package or group name",
                dnf_cmd_remove);
    dnf_cmd_add(priv->cmd_array,
                "update", "[pkgname]",
                "Update a package or group name",
                dnf_cmd_update);
    dnf_cmd_add(priv->cmd_array,
                "refresh", "[force]",
                "Refresh all the metadata",
                dnf_cmd_refresh);
    dnf_cmd_add(priv->cmd_array,
                "clean", NULL,
                "Clean all the metadata",
                dnf_cmd_clean);

    /* sort by command name */
    g_ptr_array_sort(priv->cmd_array,
             (GCompareFunc) dnf_sort_command_name_cb);

    /* get a list of the commands */
    priv->option_context = g_option_context_new(NULL);
    cmd_descriptions = dnf_cmd_get_descriptions(priv->cmd_array);
    g_option_context_set_summary(priv->option_context, cmd_descriptions);

    g_option_context_add_main_entries(priv->option_context, options, NULL);
    ret = g_option_context_parse(priv->option_context, &argc, &argv, &error);
    if (!ret) {
        g_print("Failed to parse arguments: %s\n", error->message);
        goto out;
    }

    if (!opt_reposdir)
        opt_reposdir = g_strdup("/etc/yum.repos.d");

    /* add filter if specified */
    priv->context = dnf_context_new();
    if (opt_root) {
        dnf_context_set_install_root(priv->context, opt_root);
        if (!opt_cachedir) {
            g_print("Must specify --cachedir when using --root\n");
            goto out;
        }
    } else if (!opt_cachedir)
        opt_cachedir = g_strdup("/var/cache/PackageKit"); 

    {
        g_autofree char *metadatadir =
            g_build_filename(opt_cachedir, "metadata", NULL);
        g_autofree char *solvdir =
            g_build_filename(opt_cachedir, "hawkey", NULL);
        g_autofree char *lockdir =
            g_build_filename(opt_cachedir, "lock", NULL);
        dnf_context_set_cache_dir(priv->context, metadatadir);
        dnf_context_set_solv_dir(priv->context, solvdir);
        dnf_context_set_lock_dir(priv->context, lockdir);
    }

    dnf_context_set_repo_dir(priv->context, opt_reposdir);

    dnf_context_set_check_disk_space(priv->context, TRUE);
    dnf_context_set_check_transaction(priv->context, TRUE);
    dnf_context_set_keep_cache(priv->context, FALSE);
    dnf_context_set_cache_age(priv->context, 24 * 60 * 60);
    dnf_context_set_yumdb_enabled(priv->context, !opt_disable_yumdb);

    /* set verbose? */
    if (verbose) {
        g_setenv("G_MESSAGES_DEBUG", "all", FALSE);
    } else {
        g_log_set_handler(G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
                          dnf_cmd_ignore_cb, NULL);
    }

    /* get version */
    if (version) {
        g_print("Version:\t%s\n", PACKAGE_VERSION);
        goto out;
    }

    /* run the specified command */
    ret = dnf_cmd_run(priv, argv[1],(gchar**) &argv[2], &error);
    if (!ret) {
        if (g_error_matches(error, DNF_ERROR, DNF_ERROR_NO_SUCH_CMD)) {
            g_autofree gchar *tmp = NULL;
            tmp = g_option_context_get_help(priv->option_context, TRUE, NULL);
            g_print("%s", tmp);
        } else {
            g_print("%s\n", error->message);
        }
        goto out;
    }

    /* success */
    retval = 0;
out:
    if (priv != NULL) {
        if (priv->context != NULL)
            g_object_unref(priv->context);
        if (priv->cmd_array != NULL)
            g_ptr_array_unref(priv->cmd_array);
        g_strfreev(priv->filters);
        g_option_context_free(priv->option_context);
        g_free(priv);
    }
    return retval;
}

