/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2013-2014 Richard Hughes <richard@hughsie.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

/**
 * SECTION:hif-sack
 * @short_description: Object representing a remote sack.
 * @include: libhif.h
 * @stability: Unstable
 *
 * Sacks are remote repositories of packages.
 *
 * See also: #HifSource
 */

#include "config.h"

#include <hawkey/errno.h>
#include <hawkey/query.h>
#include <hawkey/packageset.h>

#include "hif-cleanup.h"
#include "libhif.h"
#include "hif-utils.h"

static void
process_excludes (HySack sack,
		  HifSource *src)
{
	gchar **excludes = hif_source_get_exclude_packages (src);
	gchar **iter;
	
	if (!excludes)
		return;

	for (iter = excludes; *iter; iter++) {
		const char *name = *iter;
		HyQuery query;
		HyPackageSet pkgset;

		query = hy_query_create (sack);
		hy_query_filter_latest_per_arch (query, TRUE);
		hy_query_filter (query, HY_PKG_REPONAME, HY_EQ, hif_source_get_id (src));
		hy_query_filter (query, HY_PKG_ARCH, HY_NEQ, "src");
		hy_query_filter (query, HY_PKG_NAME, HY_EQ, name);
		pkgset = hy_query_run_set (query);

		hy_sack_add_excludes (sack, pkgset);

		hy_query_free (query);
		hy_packageset_free (pkgset);
	}
}

/**
 * hif_sack_add_source:
 */
gboolean
hif_sack_add_source (HySack sack,
		     HifSource *src,
		     guint permissible_cache_age,
		     HifSackAddFlags flags,
		     HifState *state,
		     GError **error)
{
	gboolean ret = TRUE;
	GError *error_local = NULL;
	gint rc;
	HifState *state_local;
	int flags_hy = HY_BUILD_CACHE;

	/* set state */
	ret = hif_state_set_steps (state, error,
				   5, /* check repo */
				   95, /* load solv */
				   -1);
	if (!ret)
		return FALSE;

	/* check repo */
	state_local = hif_state_get_child (state);
	ret = hif_source_check (src,
				permissible_cache_age,
				state_local,
				&error_local);
	if (!ret) {
		g_debug ("failed to check, attempting update: %s",
			 error_local->message);
		g_clear_error (&error_local);
		hif_state_reset (state_local);
		ret = hif_source_update (src,
					 HIF_SOURCE_UPDATE_FLAG_FORCE,
					 state_local,
					 &error_local);
		if (!ret) {
			if (!hif_source_get_required (src) &&
			    g_error_matches (error_local,
					     HIF_ERROR,
					     HIF_ERROR_CANNOT_FETCH_SOURCE)) {
				g_warning ("Skipping refresh of %s: %s",
					   hif_source_get_id (src),
					   error_local->message);
				g_error_free (error_local);
				return hif_state_finished (state, error);
			}
			g_propagate_error (error, error_local);
			return FALSE;
		}
	}

	/* checking disabled the repo */
	if (hif_source_get_enabled (src) == HIF_SOURCE_ENABLED_NONE) {
		g_debug ("Skipping %s as repo no longer enabled",
			 hif_source_get_id (src));
		return hif_state_finished (state, error);
	}

	/* done */
	if (!hif_state_done (state, error))
		return FALSE;

	/* only load what's required */
	if ((flags & HIF_SACK_ADD_FLAG_FILELISTS) > 0)
		flags_hy |= HY_LOAD_FILELISTS;
	if ((flags & HIF_SACK_ADD_FLAG_UPDATEINFO) > 0)
		flags_hy |= HY_LOAD_UPDATEINFO;

	/* load solv */
	g_debug ("Loading repo %s", hif_source_get_id (src));
	hif_state_action_start (state, HIF_STATE_ACTION_LOADING_CACHE, NULL);
	rc = hy_sack_load_repo (sack, hif_source_get_repo (src), flags_hy);
	if (rc == HY_E_FAILED)
		rc = hy_get_errno ();
	if (!hif_error_set_from_hawkey (rc, error)) {
		g_prefix_error (error, "Failed to load repo %s: ",
				hif_source_get_id (src));
		return FALSE;
	}

	process_excludes (sack, src);

	/* done */
	return hif_state_done (state, error);
}

/**
 * hif_sack_add_sources:
 */
gboolean
hif_sack_add_sources (HySack sack,
		      GPtrArray *sources,
		      guint permissible_cache_age,
		      HifSackAddFlags flags,
		      HifState *state,
		      GError **error)
{
	gboolean ret;
	guint cnt = 0;
	guint i;
	HifSource *src;
	HifState *state_local;

	/* count the enabled sources */
	for (i = 0; i < sources->len; i++) {
		src = g_ptr_array_index (sources, i);
		if (hif_source_get_enabled (src) != HIF_SOURCE_ENABLED_NONE)
			cnt++;
	}

	/* add each repo */
	hif_state_set_number_steps (state, cnt);
	for (i = 0; i < sources->len; i++) {
		src = g_ptr_array_index (sources, i);
		if (hif_source_get_enabled (src) == HIF_SOURCE_ENABLED_NONE)
			continue;

		/* only allow metadata-only sources if FLAG_UNAVAILABLE is set */
		if (hif_source_get_enabled (src) == HIF_SOURCE_ENABLED_METADATA) {
			if ((flags & HIF_SACK_ADD_FLAG_UNAVAILABLE) == 0)
				continue;
		}

		state_local = hif_state_get_child (state);
		ret = hif_sack_add_source (sack,
					   src,
					   permissible_cache_age,
					   flags,
					   state_local,
					   error);
		if (!ret)
			return FALSE;

		/* done */
		if (!hif_state_done (state, error))
			return FALSE;
	}

	/* success */
	return TRUE;
}
