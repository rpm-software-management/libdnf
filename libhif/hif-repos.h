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

#if !defined (__LIBHIF_H) && !defined (HIF_COMPILATION)
#error "Only <libhif.h> can be included directly."
#endif

#ifndef __HIF_REPOS_H
#define __HIF_REPOS_H

#include <glib-object.h>
#include "hy-repo.h"
#include "hy-package.h"

#include "hif-context.h"
#include "hif-state.h"
#include "hif-source.h"

G_BEGIN_DECLS

#define HIF_TYPE_REPOS (hif_repos_get_type ())
G_DECLARE_DERIVABLE_TYPE (HifRepos, hif_repos, HIF, REPOS, GObject)

struct _HifReposClass
{
        GObjectClass            parent_class;
        void                    (* changed)     (HifRepos       *repos);
        /*< private >*/
        void (*_hif_reserved1)  (void);
        void (*_hif_reserved2)  (void);
        void (*_hif_reserved3)  (void);
        void (*_hif_reserved4)  (void);
        void (*_hif_reserved5)  (void);
        void (*_hif_reserved6)  (void);
        void (*_hif_reserved7)  (void);
        void (*_hif_reserved8)  (void);
};

HifRepos        *hif_repos_new                  (HifContext     *context);

/* object methods */
gboolean         hif_repos_has_removable        (HifRepos       *self);
GPtrArray       *hif_repos_get_sources          (HifRepos       *self,
                                                 GError         **error);
HifSource       *hif_repos_get_source_by_id     (HifRepos       *self,
                                                 const gchar    *id,
                                                 GError         **error);

G_END_DECLS

#endif /* __HIF_REPOS_H */
