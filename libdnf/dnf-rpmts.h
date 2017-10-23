/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2013-2015 Richard Hughes <richard@hughsie.com>
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

#ifndef __DNF_RPMTS_H
#define __DNF_RPMTS_H

#include <glib.h>
#include <rpm/rpmts.h>
#include "hy-package.h"

G_BEGIN_DECLS

gboolean         dnf_rpmts_add_install_filename (rpmts           ts,
                                                 const gchar    *filename,
                                                 gboolean        allow_untrusted,
                                                 gboolean        is_update,
                                                 GError         **error);
gboolean         dnf_rpmts_add_remove_pkg       (rpmts           ts,
                                                 DnfPackage *      pkg,
                                                 GError         **error);
gboolean         dnf_rpmts_look_for_problems    (rpmts           ts,
                                                 GError         **error);

G_END_DECLS

#endif /* __DNF_RPMTS_H */
