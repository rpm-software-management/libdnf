/* dnf-swdb-pkg.h
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

#ifndef DNF_SWDB_PKG_H
#define DNF_SWDB_PKG_H

#include "dnf-swdb.h"
#include <glib-object.h>

G_BEGIN_DECLS

#define DNF_TYPE_SWDB_PKG       dnf_swdb_pkg_get_type()

G_DECLARE_FINAL_TYPE (DnfSwdbPkg, dnf_swdb_pkg, DNF, SWDB_PKG, GObject)

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
    gchar *state;
    gint pid;
    gchar *ui_from_repo;
    gchar *nvra;
    DnfSwdb *swdb;
};

DnfSwdbPkg          *dnf_swdb_pkg_new               (const gchar   *name,
                                                     const gchar   *epoch,
                                                     const gchar   *version,
                                                     const gchar   *release,
                                                     const gchar   *arch,
                                                     const gchar   *checksum_data,
                                                     const gchar   *checksum_type,
                                                     const gchar   *type);
gchar              *dnf_swdb_pkg_get_ui_from_repo   (DnfSwdbPkg    *self);
gchar              *dnf_swdb_pkg_get_reason         (DnfSwdbPkg    *self);
gint64              dnf_swdb_pkg_compare            (DnfSwdbPkg    *pkg1,
                                                     DnfSwdbPkg    *pkg2);

G_END_DECLS

#endif
