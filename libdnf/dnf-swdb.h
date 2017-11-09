/* dnf-swdb.h
 *
 * Copyright (C) 2016 Red Hat, Inc.
 * Author: Eduard Cuba <ecuba@redhat.com>
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

#ifndef DNF_SWDB_H
#define DNF_SWDB_H

#define DNF_SWDB_DEFAULT_PATH           "/var/lib/dnf/history/swdb.sqlite"

#include <glib-object.h>
#include <glib.h>
#include <sqlite3.h>

G_BEGIN_DECLS

#define DNF_TYPE_SWDB dnf_swdb_get_type ()

// structure,function prefix,namespace,object name,inherits
G_DECLARE_FINAL_TYPE (DnfSwdb, dnf_swdb, DNF, SWDB, GObject)

#include "dnf-swdb-pkg.h"
#include "dnf-swdb-itemdata.h"
#include "dnf-swdb-groups.h"
#include "dnf-swdb-trans.h"
#include "dnf-swdb-db.h"
#include "dnf-swdb-types.h"

struct _DnfSwdb
{
    GObject parent_instance;
    gchar   *path;
    sqlite3 *db;
    gboolean ready; //db opened
    gchar *releasever;
};

DnfSwdb        *dnf_swdb_new                    (const gchar    *db_path,
                                                 const gchar    *releasever);
gint            dnf_swdb_get_output_type        (DnfSwdb        *self,
                                                 const gchar    *type);
DnfSwdbReason   _reason_by_iid                  (sqlite3        *db,
                                                 gint            iid);
DnfSwdbReason   dnf_swdb_reason                 (DnfSwdb        *self,
                                                 const gchar    *nevra);
gint            dnf_swdb_get_state_type         (DnfSwdb        *self,
                                                 const gchar    *type);
gint            dnf_swdb_add_package            (DnfSwdb        *self,
                                                 DnfSwdbPkg     *pkg);
gint            dnf_swdb_log_error              (DnfSwdb        *self,
                                                 gint            tid,
                                                 const gchar    *msg);
gint            dnf_swdb_log_output             (DnfSwdb        *self,
                                                 gint           tid,
                                                 const gchar    *msg);
gint            dnf_swdb_trans_beg              (DnfSwdb        *self,
                                                 gint64          timestamp,
                                                 const gchar    *rpmdb_version,
                                                 const gchar    *cmdline,
                                                 const gchar    *loginuid,
                                                 const gchar    *releasever);
gint            dnf_swdb_trans_end              (DnfSwdb        *self,
                                                 gint            tid,
                                                 gint64          end_timestamp,
                                                 const gchar    *end_rpmdb_version,
                                                 gint            return_code);
gint            dnf_swdb_item_data_add          (DnfSwdb        *self,
                                                 DnfSwdbItemData *data);
gint            dnf_swdb_item_data_iid_end     (DnfSwdb        *self,
                                                 gint            iid,
                                                 gint            tid,
                                                 const gchar    *state);
GPtrArray      *dnf_swdb_load_error             (DnfSwdb        *self,
                                                 gint            tid);
GPtrArray      *dnf_swdb_load_output            (DnfSwdb        *self,
                                                 gint            tid);
gchar          *_get_description                (sqlite3        *db,
                                                 gint            id,
                                                 const gchar    *sql);
GArray         *dnf_swdb_search                 (DnfSwdb        *self,
                                                 GPtrArray      *patterns);
gint            _idid_from_iid                  (sqlite3        *db,
                                                 gint            iid);
gchar          *_repo_by_rid                    (sqlite3        *db,
                                                 gint            rid);
GPtrArray      *dnf_swdb_get_packages_by_tid    (DnfSwdb        *self,
                                                 gint            tid);
gchar          *dnf_swdb_trans_cmdline          (DnfSwdb        *self,
                                                 gint            tid);
GPtrArray      *dnf_swdb_trans_old              (DnfSwdb        *self,
                                                 GArray         *tids,
                                                 gint            limit,
                                                 gboolean        complete_only);
DnfSwdbTrans   *dnf_swdb_last                   (DnfSwdb        *self,
                                                 gboolean        complete_only);
DnfSwdbPkg     *dnf_swdb_package                (DnfSwdb        *self,
                                                 const gchar    *nevra);
DnfSwdbItemData *dnf_swdb_item_data             (DnfSwdb        *self,
                                                 const gchar    *nevra);
gchar          *dnf_swdb_repo                   (DnfSwdb        *self,
                                                 const gchar    *nevra);
GPtrArray      *dnf_swdb_checksums              (DnfSwdb        *self,
                                                 GPtrArray      *nevras);
gint            dnf_swdb_set_reason             (DnfSwdb        *self,
                                                 const gchar    *nevra,
                                                 DnfSwdbReason   reason);
gint            dnf_swdb_set_repo               (DnfSwdb        *self,
                                                 const gchar    *nevra,
                                                 const gchar    *repo);
gboolean        dnf_swdb_user_installed         (DnfSwdb        *self,
                                                 const gchar    *nevra);
gchar          *_repo_by_iid                    (sqlite3        *db,
                                                 gint            iid);
GArray         *dnf_swdb_select_user_installed  (DnfSwdb        *self,
                                                 GPtrArray      *nevras);
gint            dnf_swdb_iid_by_nevra           (DnfSwdb        *self,
                                                 const gchar    *nevra);
void            dnf_swdb_trans_with             (DnfSwdb        *self,
                                                 int             tid,
                                                 int             iid);
DnfSwdbPkg     *_get_package_by_iid             (sqlite3        *db,
                                                 gint            iid);
void            dnf_swdb_trans_with_libdnf      (DnfSwdb        *self,
                                                 int             tid);
DnfSwdbReason   dnf_swdb_get_erased_reason      (DnfSwdb        *self,
                                                 gchar          *nevra,
                                                 gint            first_trans,
                                                 gboolean        rollback);

G_END_DECLS

#endif
