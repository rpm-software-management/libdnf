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

#ifndef __HIF_SOURCE_H
#define __HIF_SOURCE_H

#include <glib-object.h>

#define HIF_TYPE_SOURCE			(hif_source_get_type())
#define HIF_SOURCE(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), HIF_TYPE_SOURCE, HifSource))
#define HIF_SOURCE_CLASS(cls)		(G_TYPE_CHECK_CLASS_CAST((cls), HIF_TYPE_SOURCE, HifSourceClass))
#define HIF_IS_SOURCE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), HIF_TYPE_SOURCE))
#define HIF_IS_SOURCE_CLASS(cls)	(G_TYPE_CHECK_CLASS_TYPE((cls), HIF_TYPE_SOURCE))
#define HIF_SOURCE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), HIF_TYPE_SOURCE, HifSourceClass))

G_BEGIN_DECLS

typedef struct _HifSource		HifSource;
typedef struct _HifSourceClass	HifSourceClass;

struct _HifSource
{
	GObject			parent;
};

struct _HifSourceClass
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

GType		 hif_source_get_type		(void);
HifSource	*hif_source_new			(void);

/* getters */
const gchar	*hif_source_get_id		(HifSource	*source);

/* setters */
void		 hif_source_set_id		(HifSource	*source,
						 const gchar	*id);

G_END_DECLS

#endif /* __HIF_SOURCE_H */
