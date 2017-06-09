/* dnf-swdb-trans.h
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

#ifndef DNF_SWDB_TRANS_H
#define DNF_SWDB_TRANS_H

#include <glib-object.h>

G_BEGIN_DECLS

#define DNF_TYPE_SWDB_TRANS     dnf_swdb_trans_get_type()
G_DECLARE_FINAL_TYPE (DnfSwdbTrans, dnf_swdb_trans, DNF, SWDB_TRANS, GObject)

#include "dnf-swdb.h"

struct _DnfSwdbTrans
{
    GObject parent_instance;
    DnfSwdb *swdb;
    gint tid;
    gchar *beg_timestamp;
    gchar *end_timestamp;
    gchar *beg_rpmdb_version;
    gchar *end_rpmdb_version;
    gchar *cmdline;
    gchar *loginuid;
    gchar *releasever;
    gint return_code;
    gboolean altered_lt_rpmdb;
    gboolean altered_gt_rpmdb;
    gboolean is_output;
    gboolean is_error;
    GArray *merged_tids;
};

DnfSwdbTrans   *dnf_swdb_trans_new              (gint           tid,
                                                 const gchar   *beg_timestamp,
                                                 const gchar   *end_timestamp,
                                                 const gchar   *beg_rpmdb_version,
                                                 const gchar   *end_rpmdb_version,
                                                 const gchar   *cmdline,
                                                 const gchar   *loginuid,
                                                 const gchar   *releasever,
                                                 gint           return_code);
gboolean        dnf_swdb_trans_compare_rpmdbv   (DnfSwdbTrans  *self,
                                                 const gchar   *rpmdbv);
GPtrArray      *dnf_swdb_trans_data             (DnfSwdbTrans  *self);
GPtrArray      *dnf_swdb_trans_output           (DnfSwdbTrans  *self);
GPtrArray      *dnf_swdb_trans_error            (DnfSwdbTrans  *self);
void            dnf_swdb_trans_merge            (DnfSwdbTrans  *self,
                                                 DnfSwdbTrans  *other);
GPtrArray      *dnf_swdb_trans_packages         (DnfSwdbTrans  *self);

G_END_DECLS

#endif
