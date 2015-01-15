/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010-2014 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#include "config.h"

#include <gio/gio.h>
#include <libhif.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include "hif-cleanup.h"
#include "hif-context-private.h"
#include "hif-utils.h"

#define HIF_ERROR_INVALID_ARGUMENTS	0
#define HIF_ERROR_NO_SUCH_CMD		1

typedef struct {
	HifContext		*context;
	GOptionContext		*option_context;
	GPtrArray		*cmd_array;
	gboolean		 value_only;
	gchar			**filters;
} HifUtilPrivate;

typedef gboolean (*HifUtilPrivateCb)	(HifUtilPrivate	*cmd,
					 gchar		**values,
					 GError		**error);

typedef struct {
	gchar			*name;
	gchar			*arguments;
	gchar			*description;
	HifUtilPrivateCb	 callback;
} HifUtilItem;

/**
 * hif_cmd_item_free:
 **/
static void
hif_cmd_item_free (HifUtilItem *item)
{
	g_free (item->name);
	g_free (item->arguments);
	g_free (item->description);
	g_free (item);
}

/*
 * hif_sort_command_name_cb:
 */
static gint
hif_sort_command_name_cb (HifUtilItem **item1, HifUtilItem **item2)
{
	return g_strcmp0 ((*item1)->name, (*item2)->name);
}

/**
 * hif_cmd_add:
 **/
static void
hif_cmd_add (GPtrArray *array,
	     const gchar *name,
	     const gchar *arguments,
	     const gchar *description,
	     HifUtilPrivateCb callback)
{
	guint i;
	HifUtilItem *item;
	_cleanup_strv_free_ gchar **names = NULL;

	g_return_if_fail (name != NULL);
	g_return_if_fail (description != NULL);
	g_return_if_fail (callback != NULL);

	/* add each one */
	names = g_strsplit (name, ",", -1);
	for (i = 0; names[i] != NULL; i++) {
		item = g_new0 (HifUtilItem, 1);
		item->name = g_strdup (names[i]);
		if (i == 0) {
			item->description = g_strdup (description);
		} else {
			item->description = g_strdup_printf ("Alias to %s",
							     names[0]);
		}
		item->arguments = g_strdup (arguments);
		item->callback = callback;
		g_ptr_array_add (array, item);
	}
}

/**
 * hif_cmd_get_descriptions:
 **/
static gchar *
hif_cmd_get_descriptions (GPtrArray *array)
{
	guint i;
	guint j;
	guint len;
	const guint max_len = 35;
	HifUtilItem *item;
	GString *string;

	/* print each command */
	string = g_string_new ("");
	for (i = 0; i < array->len; i++) {
		item = g_ptr_array_index (array, i);
		g_string_append (string, "  ");
		g_string_append (string, item->name);
		len = strlen (item->name) + 2;
		if (item->arguments != NULL) {
			g_string_append (string, " ");
			g_string_append (string, item->arguments);
			len += strlen (item->arguments) + 1;
		}
		if (len < max_len) {
			for (j = len; j < max_len + 1; j++)
				g_string_append_c (string, ' ');
			g_string_append (string, item->description);
			g_string_append_c (string, '\n');
		} else {
			g_string_append_c (string, '\n');
			for (j = 0; j < max_len + 1; j++)
				g_string_append_c (string, ' ');
			g_string_append (string, item->description);
			g_string_append_c (string, '\n');
		}
	}

	/* remove trailing newline */
	if (string->len > 0)
		g_string_set_size (string, string->len - 1);

	return g_string_free (string, FALSE);
}

/**
 * hif_cmd_run:
 **/
static gboolean
hif_cmd_run (HifUtilPrivate *priv, const gchar *command, gchar **values, GError **error)
{
	guint i;
	HifUtilItem *item;
	_cleanup_string_free_ GString *string = NULL;

	/* find command */
	for (i = 0; i < priv->cmd_array->len; i++) {
		item = g_ptr_array_index (priv->cmd_array, i);
		if (g_strcmp0 (item->name, command) == 0)
			return item->callback (priv, values, error);
	}

	/* not found */
	string = g_string_new ("");
	g_string_append (string, "Command not found, valid commands are:\n");
	for (i = 0; i < priv->cmd_array->len; i++) {
		item = g_ptr_array_index (priv->cmd_array, i);
		g_string_append_printf (string, " * %s %s\n",
					item->name,
					item->arguments ? item->arguments : "");
	}
	g_set_error_literal (error, HIF_ERROR, HIF_ERROR_NO_SUCH_CMD, string->str);
	return FALSE;
}

/**
 * hif_cmd_install:
 **/
static gboolean
hif_cmd_install (HifUtilPrivate *priv, gchar **values, GError **error)
{
	guint i;

	if (g_strv_length (values) < 1) {
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_INVALID_ARGUMENTS,
				     "Not enough arguments, "
				     "expected package or group name");
		return FALSE;
	}

	/* install each package */
	if (!hif_context_setup (priv->context, NULL, error))
		return FALSE;
	for (i = 0; values[i] != NULL; i++) {
		if (!hif_context_install (priv->context, values[i], error))
			return FALSE;
	}
	return hif_context_run (priv->context, NULL, error);
}

/**
 * hif_cmd_remove:
 **/
static gboolean
hif_cmd_remove (HifUtilPrivate *priv, gchar **values, GError **error)
{
	guint i;

	if (g_strv_length (values) < 1) {
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_INVALID_ARGUMENTS,
				     "Not enough arguments, "
				     "expected package or group name");
		return FALSE;
	}

	/* remove each package */
	if (!hif_context_setup (priv->context, NULL, error))
		return FALSE;
	for (i = 0; values[i] != NULL; i++) {
		if (!hif_context_remove (priv->context, values[i], error))
			return FALSE;
	}
	return hif_context_run (priv->context, NULL, error);
}

/**
 * hif_cmd_refresh_source:
 **/
static gboolean
hif_cmd_refresh_source (HifUtilPrivate *priv, HifSource *src, HifState *state, GError **error)
{
	HifState *state_local;
	gboolean ret;
	guint cache_age;
	_cleanup_error_free_ GError *error_local = NULL;

	/* set steps */
	if (!hif_state_set_steps (state, error, 50, 50, -1))
		return FALSE;

	/* check source */
	state_local = hif_state_get_child (state);
	g_print ("Checking %s\n", hif_source_get_id (src));
	cache_age = hif_context_get_cache_age (priv->context);
	ret = hif_source_check (src, cache_age, state_local, &error_local);
	if (ret)
		return hif_state_finished (state, error);

	/* done */
	if (!hif_state_done (state, error))
		return FALSE;

	/* print error to console and continue */
	g_print ("Failed to check %s: %s\n",
		 hif_source_get_id (src),
		 error_local->message);
	g_clear_error (&error_local);
	if (!hif_state_finished (state_local, error))
		return FALSE;

	/* actually update source */
	state_local = hif_state_get_child (state);
	g_print ("Updating %s\n", hif_source_get_id (src));
	if (!hif_source_update (src, HIF_SOURCE_UPDATE_FLAG_IMPORT_PUBKEY,
				state_local, &error_local)) {
		if (g_error_matches (error_local,
				     HIF_ERROR,
				     HIF_ERROR_CANNOT_FETCH_SOURCE)) {
			g_print ("Skipping repo: %s\n", error_local->message);
			return hif_state_finished (state, error);
		}
		g_propagate_error (error, error_local);
		return FALSE;
	}

	/* done */
	return hif_state_done (state, error);
}

/**
 * hif_cmd_refresh:
 **/
static gboolean
hif_cmd_refresh (HifUtilPrivate *priv, gchar **values, GError **error)
{
	GPtrArray *sources;
	HifSource *src;
	HifState *state;
	HifState *state_local;
	guint i;

	if (!hif_context_setup (priv->context, NULL, error))
		return FALSE;

	/* check and refresh each source in turn */
	sources = hif_context_get_sources (priv->context);
	state = hif_context_get_state (priv->context);
	hif_state_set_number_steps (state, sources->len);
	for (i = 0; i < sources->len; i++) {
		src = g_ptr_array_index (sources, i);
		if (hif_source_get_enabled (src) == HIF_SOURCE_ENABLED_NONE)
			continue;
		state_local = hif_state_get_child (state);
		if (!hif_cmd_refresh_source (priv, src, state_local, error))
			return FALSE;
		if (!hif_state_done (state, error))
			return FALSE;
	}
	return TRUE;
}

/**
 * hif_cmd_clean:
 **/
static gboolean
hif_cmd_clean (HifUtilPrivate *priv, gchar **values, GError **error)
{
	GPtrArray *sources;
	HifSource *src;
	HifState *state;
	guint i;

	if (!hif_context_setup (priv->context, NULL, error))
		return FALSE;

	/* clean each source in turn */
	sources = hif_context_get_sources (priv->context);
	state = hif_context_get_state (priv->context);
	hif_state_set_number_steps (state, sources->len);
	for (i = 0; i < sources->len; i++) {
		src = g_ptr_array_index (sources, i);
		if (!hif_source_clean (src, error))
			return FALSE;
		if (!hif_state_done (state, error))
			return FALSE;
	}
	return TRUE;
}

/**
 * hif_cmd_update:
 **/
static gboolean
hif_cmd_update (HifUtilPrivate *priv, gchar **values, GError **error)
{
	guint i;

	if (g_strv_length (values) < 1) {
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_INVALID_ARGUMENTS,
				     "Not enough arguments, "
				     "expected package or group name");
		return FALSE;
	}

	/* update each package */
	if (!hif_context_setup (priv->context, NULL, error))
		return FALSE;
	for (i = 0; values[i] != NULL; i++) {
		if (!hif_context_update (priv->context, values[i], error))
			return FALSE;
	}
	return hif_context_run (priv->context, NULL, error);
}

/**
 * hif_cmd_ignore_cb:
 **/
static void
hif_cmd_ignore_cb (const gchar *log_domain, GLogLevelFlags log_level,
		   const gchar *message, gpointer user_data)
{
}

/**
 * main:
 **/
int
main (int argc, char *argv[])
{
	HifUtilPrivate *priv;
	gboolean ret;
	gboolean verbose = FALSE;
	gboolean version = FALSE;
	guint retval = 1;
	const GOptionEntry options[] = {
		{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose,
			"Show extra debugging information", NULL },
		{ "version", '\0', 0, G_OPTION_ARG_NONE, &version,
			"Show version", NULL },
		{ NULL}
	};
	_cleanup_error_free_ GError *error = NULL;
	_cleanup_free_ gchar *cmd_descriptions = NULL;
	_cleanup_free_ gchar *filter = NULL;

	setlocale (LC_ALL, "");

	/* create helper object */
	priv = g_new0 (HifUtilPrivate, 1);

	/* add commands */
	priv->cmd_array = g_ptr_array_new_with_free_func ((GDestroyNotify) hif_cmd_item_free);
	hif_cmd_add (priv->cmd_array,
		     "install", "[pkgname]",
		     "Install a package or group name",
		     hif_cmd_install);
	hif_cmd_add (priv->cmd_array,
		     "remove", "[pkgname]",
		     "Remove a package or group name",
		     hif_cmd_remove);
	hif_cmd_add (priv->cmd_array,
		     "update", "[pkgname]",
		     "Update a package or group name",
		     hif_cmd_update);
	hif_cmd_add (priv->cmd_array,
		     "refresh", "[force]",
		     "Refresh all the metadata",
		     hif_cmd_refresh);
	hif_cmd_add (priv->cmd_array,
		     "clean", NULL,
		     "Clean all the metadata",
		     hif_cmd_clean);

	/* sort by command name */
	g_ptr_array_sort (priv->cmd_array,
			  (GCompareFunc) hif_sort_command_name_cb);

	/* get a list of the commands */
	priv->option_context = g_option_context_new (NULL);
	cmd_descriptions = hif_cmd_get_descriptions (priv->cmd_array);
	g_option_context_set_summary (priv->option_context, cmd_descriptions);

	g_option_context_add_main_entries (priv->option_context, options, NULL);
	ret = g_option_context_parse (priv->option_context, &argc, &argv, &error);
	if (!ret) {
		g_print ("Failed to parse arguments: %s\n", error->message);
		goto out;
	}

	/* add filter if specified */
	priv->context = hif_context_new ();
	hif_context_set_repo_dir (priv->context, "/etc/yum.repos.d");
	hif_context_set_cache_dir (priv->context, "/var/cache/PackageKit/metadata/");
	hif_context_set_solv_dir (priv->context, "/var/cache/PackageKit/hawkey/");
	hif_context_set_check_disk_space (priv->context, TRUE);
	hif_context_set_check_transaction (priv->context, TRUE);
	hif_context_set_keep_cache (priv->context, FALSE);
	hif_context_set_cache_age (priv->context, 24 * 60 * 60);

	/* set verbose? */
	if (verbose) {
		g_setenv ("G_MESSAGES_DEBUG", "all", FALSE);
	} else {
		g_log_set_handler (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,
				   hif_cmd_ignore_cb, NULL);
	}

	/* get version */
	if (version) {
		g_print ("Version:\t%s\n", PACKAGE_VERSION);
		goto out;
	}

	/* run the specified command */
	ret = hif_cmd_run (priv, argv[1], (gchar**) &argv[2], &error);
	if (!ret) {
		if (g_error_matches (error, HIF_ERROR, HIF_ERROR_NO_SUCH_CMD)) {
			gchar *tmp;
			tmp = g_option_context_get_help (priv->option_context, TRUE, NULL);
			g_print ("%s", tmp);
			g_free (tmp);
		} else {
			g_print ("%s\n", error->message);
		}
		goto out;
	}

	/* success */
	retval = 0;
out:
	if (priv != NULL) {
		if (priv->context != NULL)
			g_object_unref (priv->context);
		if (priv->cmd_array != NULL)
			g_ptr_array_unref (priv->cmd_array);
		g_strfreev (priv->filters);
		g_option_context_free (priv->option_context);
		g_free (priv);
	}
	return retval;
}

