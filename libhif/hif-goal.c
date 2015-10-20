/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2013-2014 Richard Hughes <richard@hughsie.com>
 *
 * Most of this code was taken from Zif, libzif/zif-transaction.c
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
 * SECTION:hif-goal
 * @short_description: Helper methods for dealing with hawkey goals.
 * @include: libhif.h
 * @stability: Unstable
 *
 * These methods make it easier to deal with hawkey goals.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include "hy-packagelist.h"
#include "hy-util.h"

#include "hif-cleanup.h"
#include "hif-goal.h"
#include "hif-package.h"
#include "hif-utils.h"

/**
 * hif_goal_depsolve:
 */
gboolean
hif_goal_depsolve (HyGoal goal, GError **error)
{
	gchar *tmp;
	gint cnt;
	gint j;
	gint rc;
	_cleanup_string_free_ GString *string = NULL;

	rc = hy_goal_run_flags (goal, HY_ALLOW_UNINSTALL);
	if (rc) {
		string = g_string_new ("Could not depsolve transaction; ");
		cnt = hy_goal_count_problems (goal);
		if (cnt == 1)
			g_string_append_printf (string, "%i problem detected:\n", cnt);
		else
			g_string_append_printf (string, "%i problems detected:\n", cnt);
		for (j = 0; j < cnt; j++) {
			tmp = hy_goal_describe_problem (goal, j);
			g_string_append_printf (string, "%i. %s\n", j, tmp);
			g_free (tmp);
		}
		g_string_truncate (string, string->len - 1);
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_PACKAGE_CONFLICTS,
				     string->str);
		return FALSE;
	}

	/* anything to do? */
	if (hy_goal_req_length (goal) == 0) {
		g_set_error_literal (error,
				     HIF_ERROR,
				     HIF_ERROR_NO_PACKAGES_TO_UPDATE,
				     "The transaction was empty");
		return FALSE;
	}
	return TRUE;
}

/**
 * hif_goal_get_packages:
 */
GPtrArray *
hif_goal_get_packages (HyGoal goal, ...)
{
	GPtrArray *array;
	HyPackage pkg;
	gint info_tmp;
	guint i;
	guint j;
	va_list args;

	/* process the valist */
	va_start (args, goal);
	array = g_ptr_array_new_with_free_func ((GDestroyNotify) hy_package_free);
	for (j = 0;; j++) {
		HyPackageList pkglist = NULL;
		info_tmp = va_arg (args, gint);
		if (info_tmp == -1)
			break;
		switch (info_tmp) {
		case HIF_PACKAGE_INFO_REMOVE:
			pkglist = hy_goal_list_erasures (goal);
			FOR_PACKAGELIST(pkg, pkglist, i) {
				hif_package_set_action (pkg, HIF_STATE_ACTION_REMOVE);
				g_ptr_array_add (array, hy_package_link (pkg));
			}
			break;
		case HIF_PACKAGE_INFO_INSTALL:
			pkglist = hy_goal_list_installs (goal);
			FOR_PACKAGELIST(pkg, pkglist, i) {
				hif_package_set_action (pkg, HIF_STATE_ACTION_INSTALL);
				g_ptr_array_add (array, hy_package_link (pkg));
			}
			break;
		case HIF_PACKAGE_INFO_OBSOLETE:
			pkglist = hy_goal_list_obsoleted (goal);
			FOR_PACKAGELIST(pkg, pkglist, i) {
				hif_package_set_action (pkg, HIF_STATE_ACTION_OBSOLETE);
				g_ptr_array_add (array, hy_package_link (pkg));
			}
			break;
		case HIF_PACKAGE_INFO_REINSTALL:
			pkglist = hy_goal_list_reinstalls (goal);
			FOR_PACKAGELIST(pkg, pkglist, i) {
				hif_package_set_action (pkg, HIF_STATE_ACTION_REINSTALL);
				g_ptr_array_add (array, hy_package_link (pkg));
			}
			break;
		case HIF_PACKAGE_INFO_UPDATE:
			pkglist = hy_goal_list_upgrades (goal);
			FOR_PACKAGELIST(pkg, pkglist, i) {
				hif_package_set_action (pkg, HIF_STATE_ACTION_UPDATE);
				g_ptr_array_add (array, hy_package_link (pkg));
			}
			break;
		case HIF_PACKAGE_INFO_DOWNGRADE:
			pkglist = hy_goal_list_downgrades (goal);
			FOR_PACKAGELIST(pkg, pkglist, i) {
				hif_package_set_action (pkg, HIF_STATE_ACTION_DOWNGRADE);
				g_ptr_array_add (array, hy_package_link (pkg));
			}
			break;
		default:
			g_assert_not_reached ();
		}
		hy_packagelist_free (pkglist);
	}
	va_end (args);
	return array;
}
