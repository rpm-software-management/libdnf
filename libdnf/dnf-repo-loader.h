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

#ifndef __DNF_REPO_LOADER_H
#define __DNF_REPO_LOADER_H

#include <glib-object.h>
#include "hy-repo.h"
#include "hy-package.h"

#include "dnf-context.h"
#include "dnf-state.h"
#include "dnf-repo.h"

G_BEGIN_DECLS

#define DNF_TYPE_REPO_LOADER (dnf_repo_loader_get_type ())
G_DECLARE_DERIVABLE_TYPE (DnfRepoLoader, dnf_repo_loader, DNF, REPO_LOADER, GObject)

struct _DnfRepoLoaderClass
{
        GObjectClass            parent_class;
        void                    (* changed)     (DnfRepoLoader   *self);
        /*< private >*/
        void (*_dnf_reserved1)  (void);
        void (*_dnf_reserved2)  (void);
        void (*_dnf_reserved3)  (void);
        void (*_dnf_reserved4)  (void);
        void (*_dnf_reserved5)  (void);
        void (*_dnf_reserved6)  (void);
        void (*_dnf_reserved7)  (void);
        void (*_dnf_reserved8)  (void);
};

DnfRepoLoader   *dnf_repo_loader_new                    (DnfContext      *context);

/* object methods */
gboolean         dnf_repo_loader_has_removable_repos    (DnfRepoLoader   *self);
GPtrArray       *dnf_repo_loader_get_repos              (DnfRepoLoader   *self,
                                                         GError         **error);
DnfRepo         *dnf_repo_loader_get_repo_by_id         (DnfRepoLoader   *self,
                                                         const gchar     *id,
                                                         GError         **error);

G_END_DECLS

#endif /* __DNF_REPO_LOADER_H */
