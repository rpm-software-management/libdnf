/* hif-swdb.h
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

#ifndef HIF_SWDB_H
#define HIF_SWDB_H

#include <glib-object.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <sqlite3.h>
#include "hif-swdb-obj.h"

G_BEGIN_DECLS

#define HIF_TYPE_SWDB (hif_swdb_get_type ())
G_DECLARE_FINAL_TYPE (HifSwdb, hif_swdb, HIF,SWDB, GObject) // structure,function prefix,namespace,object name,inherits

HifSwdb *hif_swdb_new(void);

const gchar* hif_swdb_get_path (HifSwdb *self);

/* change path to swdb - actual swdb is closed first */
void  hif_swdb_set_path (HifSwdb *self, const gchar *path);

/* True when swdb exist */
gboolean hif_swdb_exist(HifSwdb *self);

gint hif_swdb_create_db (HifSwdb *self);

/* Remove old and create new */
gint hif_swdb_reset_db (HifSwdb *self);

gint hif_swdb_add_group_package (HifSwdb *self, gint gid, const gchar *name);

gint hif_swdb_add_group_exclude (HifSwdb *self, gint gid, const gchar *name);

gint hif_swdb_add_environments_exclude (HifSwdb *self, gint eid, const gchar *name);

gint hif_swdb_open(HifSwdb *self);

void hif_swdb_close(HifSwdb *self);

gint hif_swdb_get_package_type (HifSwdb *self, const gchar *type);

gint hif_swdb_get_output_type (HifSwdb *self, const gchar *type);

gint hif_swdb_get_reason_type (HifSwdb *self, const gchar *type);

gint hif_swdb_get_state_type (HifSwdb *self, const gchar *type);

gint hif_swdb_add_package_nevracht(	HifSwdb *self,
				  					const gchar *name,
									const gchar *epoch,
									const gchar *version,
									const gchar *release,
				  					const gchar *arch,
				 					const gchar *checksum_data,
								   	const gchar *checksum_type,
								  	const gchar *type);

gint 	hif_swdb_log_error 		(	HifSwdb *self,
						 			const gint tid,
							  		const gchar *msg);

gint 	hif_swdb_log_output		(	HifSwdb *self,
						 			const gint tid,
									const gchar *msg);

gint 	hif_swdb_trans_beg 	(	HifSwdb *self,
							 	const gchar *timestamp,
							 	const gchar *rpmdb_version,
								const gchar *cmdline,
								const gchar *loginuid,
								const gchar *releasever);

gint 	hif_swdb_trans_end 	(	HifSwdb *self,
							 	const gint tid,
							 	const gchar *end_timestamp,
								const gint return_code);

gint 	hif_swdb_log_package_data(	HifSwdb *self,
									const gint   pid,
                                  	const gchar *from_repo,
                                  	const gchar *from_repo_revision,
                                  	const gchar *from_repo_timestamp,
                                  	const gchar *installed_by,
                                  	const gchar *changed_by,
                                  	const gchar *instalonly,
                                  	const gchar *origin_url );

gint 	hif_swdb_trans_data_beg(	HifSwdb *self,
									const gint 	 tid,
									const gint 	 pid,
									const gchar *reason,
									const gchar *state );

gint 	hif_swdb_trans_data_end	(	HifSwdb *self,
									const gint tid);

gint    hif_swdb_trans_data_pid_end (   HifSwdb *self,
                                        const gint pid,
                                        const gint tid,
                                        const gchar *state);


const gchar *hif_swdb_get_pkg_attr( HifSwdb *self,
									const gint pid,
									const gchar *attribute);

GPtrArray *hif_swdb_load_error (  	HifSwdb *self,
                            	const gint tid);
GPtrArray *hif_swdb_load_output (  HifSwdb *self,
								const gint tid);

const gint 	hif_swdb_get_pid_by_nevracht(	HifSwdb *self,
											const gchar *name,
											const gchar *epoch,
											const gchar *version,
											const gchar *release,
											const gchar *arch,
											const gchar *checksum_data,
											const gchar *checksum_type,
											const gchar *type,
											const gboolean create);

static gchar* _look_for_desc(sqlite3 *db, const gchar *table, const gint id);

GSList *hif_swdb_search (   HifSwdb *self,
							const GSList *patterns);


static gint _pdid_from_pid (	sqlite3 *db,
								const gint pid );
static GSList * _all_pdid_for_pid (	sqlite3 *db,
									const gint pid );

static gint _tid_from_pdid (	sqlite3 *db,
								const gint pdid );

gint 	hif_swdb_log_rpm_data(	HifSwdb *self,
									const gint   pid,
                                  	const gchar *buildtime,
                                    const gchar *buildhost,
                                    const gchar *license,
                                    const gchar *packager,
                                    const gchar *size,
                                    const gchar *sourcerpm,
                                    const gchar *url,
                                    const gchar *vendor,
                                    const gchar *committer,
                                    const gchar *committime);

GPtrArray *hif_swdb_get_packages_by_tid(   HifSwdb *self,
                                    		const gint tid);

const gchar * hif_swdb_trans_cmdline (   HifSwdb *self,
							 	        const gint tid);

GPtrArray *hif_swdb_trans_old(	HifSwdb *self,
                                GPtrArray *tids,
								gint limit,
								const gboolean complete_only);

G_END_DECLS

#endif
