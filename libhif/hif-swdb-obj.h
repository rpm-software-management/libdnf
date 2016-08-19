/* hif-swdb-obj.h
 *
 * Copyright (C) 2016 Red Hat, Inc.
 * Author: Eduard Cuba <xcubae00@stud.fit.vutbr.cz>
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

#ifndef HIF_SWDB_OBJ_H
#define HIF_SWDB_OBJ_H

#include <glib-object.h>

G_BEGIN_DECLS

//Package holder for swdb history
#define HIF_TYPE_SWDB_PKG ( hif_swdb_pkg_get_type())
G_DECLARE_FINAL_TYPE ( HifSwdbPkg, hif_swdb_pkg, HIF, SWDB_PKG, GObject)
struct _HifSwdbPkg
{
	GObject parent_instance;
    const gchar *name;
	const gchar *epoch;
	const gchar *version;
	const gchar *release;
	const gchar *arch;
	const gchar *checksum_data;
	const gchar *checksum_type;
	const gchar *type;
};

HifSwdbPkg* hif_swdb_pkg_new(   const gchar* name,
                                const gchar* epoch,
                                const gchar* version,
                                const gchar* release,
                                const gchar* arch,
                                const gchar* checksum_data,
                                const gchar* checksum_type,
                                const gchar* type);

//holder for history transaction

#define HIF_TYPE_SWDB_TRANS ( hif_swdb_trans_get_type())
G_DECLARE_FINAL_TYPE ( HifSwdbTrans, hif_swdb_trans, HIF, SWDB_TRANS, GObject)

struct _HifSwdbTrans
{
	GObject parent_instance;
	HifSwdb *swdb;
	gint tid;
	const gchar *beg_timestamp;
	const gchar *end_timestamp;
	const gchar *rpmdb_version;
	const gchar *cmdline;
	const gchar *loginuid;
	const gchar *releasever;
	gint return_code;
	gboolean altered_lt_rpmdb;
	gboolean altered_gt_rpmdb;
	gboolean is_output;
	gboolean is_error;
};

HifSwdbTrans* hif_swdb_trans_new(	const gint tid,
									const gchar *beg_timestamp,
									const gchar *end_timestamp,
									const gchar *rpmdb_version,
									const gchar *cmdline,
									const gchar *loginuid,
									const gchar *releasever,
									const gint return_code);

//holder for history transaction data

#define HIF_TYPE_SWDB_TRANSDATA ( hif_swdb_transdata_get_type())
G_DECLARE_FINAL_TYPE ( HifSwdbTransData, hif_swdb_transdata, HIF, SWDB_TRANSDATA, GObject)
struct _HifSwdbTransData
{
	GObject parent_instance;
	gint tdid;
	gint tid;
	gint pdid;
	gint gid;
	gint done;
	gint ORIGINAL_TD_ID;
	gchar *reason;
	gchar *state;
};

HifSwdbTransData* hif_swdb_transdata_new(	gint tdid,
											gint tid,
											gint pdid,
											gint gid,
											gint done,
											gint ORIGINAL_TD_ID,
											gchar *reason,
											gchar *state);

G_END_DECLS

#endif
