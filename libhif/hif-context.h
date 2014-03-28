/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2014 Richard Hughes <richard@hughsie.com>
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

#ifndef __HIF_CONTEXT_H
#define __HIF_CONTEXT_H

#include <glib-object.h>

#define HIF_TYPE_CONTEXT		(hif_context_get_type())
#define HIF_CONTEXT(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), HIF_TYPE_CONTEXT, HifContext))
#define HIF_CONTEXT_CLASS(cls)		(G_TYPE_CHECK_CLASS_CAST((cls), HIF_TYPE_CONTEXT, HifContextClass))
#define HIF_IS_CONTEXT(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), HIF_TYPE_CONTEXT))
#define HIF_IS_CONTEXT_CLASS(cls)	(G_TYPE_CHECK_CLASS_TYPE((cls), HIF_TYPE_CONTEXT))
#define HIF_CONTEXT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), HIF_TYPE_CONTEXT, HifContextClass))

G_BEGIN_DECLS

typedef struct _HifContext		HifContext;
typedef struct _HifContextClass		HifContextClass;

struct _HifContext
{
	GObject			parent;
};

struct _HifContextClass
{
	GObjectClass		parent_class;
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

GType		 hif_context_get_type		(void);
HifContext	*hif_context_new		(void);

/* getters */
const gchar	*hif_context_get_repo_dir	(HifContext	*context);
const gchar	*hif_context_get_base_arch	(HifContext	*context);
const gchar	*hif_context_get_release_ver	(HifContext	*context);
const gchar	*hif_context_get_cache_dir	(HifContext	*context);

/* setters */
void		 hif_context_set_repo_dir	(HifContext	*context,
						 const gchar	*repo_dir);
void		 hif_context_set_base_arch	(HifContext	*context,
						 const gchar	*base_arch);
void		 hif_context_set_release_ver	(HifContext	*context,
						 const gchar	*release_ver);
void		 hif_context_set_cache_dir	(HifContext	*context,
						 const gchar	*cache_dir);

/* object methods */

G_END_DECLS

#endif /* __HIF_CONTEXT_H */
