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

#ifndef __HIF_SACK_H
#define __HIF_SACK_H

#include <glib-object.h>
#include <hawkey/sack.h>

#include "hif-source.h"
#include "hif-state.h"

/**
 * HifSackAddFlags:
 * @HIF_SACK_ADD_FLAG_NONE:			Add the primary
 * @HIF_SACK_ADD_FLAG_FILELISTS:		Add the filelists
 * @HIF_SACK_ADD_FLAG_UPDATEINFO:		Add the updateinfo
 * @HIF_SACK_ADD_FLAG_REMOTE:			Use remote sources
 *
 * The error code.
 **/
typedef enum {
	HIF_SACK_ADD_FLAG_NONE			= 0,
	HIF_SACK_ADD_FLAG_FILELISTS		= 1,
	HIF_SACK_ADD_FLAG_UPDATEINFO		= 2,
	HIF_SACK_ADD_FLAG_REMOTE		= 4,
	/*< private >*/
	HIF_SACK_ADD_FLAG_LAST
} HifSackAddFlags;

/* object methods */
gboolean	 hif_sack_add_source		(HySack		 sack,
						 HifSource	*src,
						 guint		 permissible_cache_age,
						 HifSackAddFlags flags,
						 HifState	*state,
						 GError		**error);
gboolean	 hif_sack_add_sources		(HySack		 sack,
						 GPtrArray	*sources,
						 guint		 permissible_cache_age,
						 HifSackAddFlags flags,
						 HifState	*state,
						 GError		**error);

G_END_DECLS

#endif /* __HIF_SACK_H */
