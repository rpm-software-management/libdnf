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
#define DNF_TYPE_SWDB_PKG dnf_swdb_pkg_get_type()
G_DECLARE_FINAL_TYPE ( DnfSwdbPkg, dnf_swdb_pkg, DNF, SWDB_PKG, GObject)
struct _DnfSwdbPkg
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
	gboolean done;
	gchar 	*state;
	gint 	pid;
	gchar * ui_from_repo;
	gchar * nvra;
	DnfSwdb *swdb;
};

HifSwdbPkg* hif_swdb_pkg_new(   const gchar* name,
                                const gchar* epoch,
                                const gchar* version,
                                const gchar* release,
                                const gchar* arch,
                                const gchar* checksum_data,
                                const gchar* checksum_type,
                                const gchar* type);
const gchar* hif_swdb_pkg_get_ui_from_repo	( HifSwdbPkg *self);


//Package Data holder for swdb history
#define DNF_TYPE_SWDB_PKGDATA dnf_swdb_pkgdata_get_type()
G_DECLARE_FINAL_TYPE ( DnfSwdbPkgData, dnf_swdb_pkgdata, DNF, SWDB_PKGDATA, GObject)
struct _DnfSwdbPkgData
{
	GObject parent_instance;
	gchar *from_repo;
    gchar *from_repo_revision;
  	gchar *from_repo_timestamp;
  	gchar *installed_by;
  	gchar *changed_by;
  	gchar *installonly;
  	gchar *origin_url;
	gint 	pdid;
	gint 	pid;
};

HifSwdbPkgData* hif_swdb_pkgdata_new( 	const gchar* from_repo_revision,
                                        const gchar* from_repo_timestamp,
                                        const gchar* installed_by,
                                        const gchar* changed_by,
                                        const gchar* installonly,
                                        const gchar* origin_url,
										const gchar* from_repo);

//holder for history transaction

#define DNF_TYPE_SWDB_TRANS dnf_swdb_trans_get_type()
G_DECLARE_FINAL_TYPE ( DnfSwdbTrans, dnf_swdb_trans, DNF, SWDB_TRANS, GObject)

struct _HifSwdbTrans
{
	GObject parent_instance;
	HifSwdb *swdb;
	gint tid;
	const gchar *beg_timestamp;
	const gchar *end_timestamp;
	const gchar *beg_rpmdb_version;
	const gchar *end_rpmdb_version;
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
									const gchar *beg_rpmdb_version,
									const gchar *end_rpmdb_version,
									const gchar *cmdline,
									const gchar *loginuid,
									const gchar *releasever,
									const gint return_code);

//holder for history transaction data

#define DNF_TYPE_SWDB_TRANSDATA dnf_swdb_transdata_get_type()
G_DECLARE_FINAL_TYPE ( DnfSwdbTransData, dnf_swdb_transdata, DNF, SWDB_TRANSDATA, GObject)
struct _DnfSwdbTransData
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

//holder for group

#define DNF_TYPE_SWDB_GROUP dnf_swdb_group_get_type()
G_DECLARE_FINAL_TYPE ( DnfSwdbGroup, dnf_swdb_group, DNF, SWDB_GROUP, GObject)
struct _DnfSwdbGroup
{
	GObject parent_instance;
	gint gid;
	const gchar* name_id;
	gchar* name;
	gchar* ui_name;
	gint is_installed;
	gint pkg_types;
	gint grp_types;
	HifSwdb *swdb;
};

HifSwdbGroup* hif_swdb_group_new(
									const gchar* name_id,
									gchar* name,
									gchar* ui_name,
									gint is_installed,
									gint pkg_types,
									gint grp_types,
									HifSwdb *swdb);

GPtrArray * hif_swdb_group_get_exclude(HifSwdbGroup *self);
GPtrArray * hif_swdb_group_get_full_list(HifSwdbGroup *self);
gint hif_swdb_group_update_full_list(   HifSwdbGroup *group,
                                        GPtrArray *full_list);
gint hif_swdb_group_add_package (       HifSwdbGroup *group,
                                        GPtrArray *packages);

gint hif_swdb_group_add_exclude (       HifSwdbGroup *group,
                                        GPtrArray *exclude);

//holder for environment

#define DNF_TYPE_SWDB_ENV dnf_swdb_env_get_type()
G_DECLARE_FINAL_TYPE ( DnfSwdbEnv, dnf_swdb_env, DNF, SWDB_ENV, GObject)
struct _DnfSwdbEnv
{
	GObject parent_instance;
	gint eid;
	const gchar* name_id;
	gchar* name;
	gchar* ui_name;
	gint pkg_types;
	gint grp_types;
	HifSwdb *swdb;
};

HifSwdbEnv* hif_swdb_env_new(		const gchar* name_id,
									gchar* name,
									gchar* ui_name,
									gint pkg_types,
									gint grp_types,
									HifSwdb *swdb);

GPtrArray *hif_swdb_env_get_grp_list    (HifSwdbEnv* env);

GPtrArray *hif_swdb_env_get_exclude    (HifSwdbEnv* self);

gboolean hif_swdb_env_is_installed  (HifSwdbEnv *env );

G_END_DECLS

#endif
