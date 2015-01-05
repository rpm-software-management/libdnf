/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2013-2014 Richard Hughes <richard@hughsie.com>
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
#include <hawkey/repo.h>
#include <hawkey/package.h>

#include "hif-context.h"
#include "hif-state.h"
#include "hif-source.h"

#define HIF_TYPE_REPOS			(hif_repos_get_type())
#define HIF_REPOS(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), HIF_TYPE_REPOS, HifRepos))
#define HIF_REPOS_CLASS(cls)		(G_TYPE_CHECK_CLASS_CAST((cls), HIF_TYPE_REPOS, HifReposClass))
#define HIF_IS_REPOS(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), HIF_TYPE_REPOS))
#define HIF_IS_REPOS_CLASS(cls)	(G_TYPE_CHECK_CLASS_TYPE((cls), HIF_TYPE_REPOS))
#define HIF_REPOS_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), HIF_TYPE_REPOS, HifReposClass))

G_BEGIN_DECLS

typedef struct _HifReposClass	HifReposClass;

struct _HifRepos
{
	GObject			parent;
};

struct _HifReposClass
{
	GObjectClass		parent_class;
	void			(* changed)	(HifRepos	*repos);
	/*< private >*/
	void (*_hif_reserved1)	(void);
	void (*_hif_reserved2)	(void);
	void (*_hif_reserved3)	(void);
	void (*_hif_reserved4)	(void);
	void (*_hif_reserved5)	(void);
	void (*_hif_reserved6)	(void);
	void (*_hif_reserved7)	(void);
	void (*_hif_reserved8)	(void);
};

GType		 hif_repos_get_type		(void);
HifRepos	*hif_repos_new			(HifContext	*context);

/* object methods */
gboolean	 hif_repos_has_removable	(HifRepos	*self);
GPtrArray	*hif_repos_get_sources		(HifRepos	*self,
						 GError		**error);
HifSource	*hif_repos_get_source_by_id	(HifRepos	*self,
						 const gchar	*id,
						 GError		**error);

G_END_DECLS

#endif /* __HIF_REPOS_H */
