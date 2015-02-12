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

#ifndef __HIF_TRANSACTION_H
#define __HIF_TRANSACTION_H

#include <glib-object.h>
#include <gio/gio.h>

#include <hawkey/goal.h>

#include "hif-context.h"
#include "hif-db.h"
#include "hif-state.h"
#include "hif-repos.h"

#define HIF_TYPE_TRANSACTION		(hif_transaction_get_type())
#define HIF_TRANSACTION(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), HIF_TYPE_TRANSACTION, HifTransaction))
#define HIF_TRANSACTION_CLASS(cls)	(G_TYPE_CHECK_CLASS_CAST((cls), HIF_TYPE_TRANSACTION, HifTransactionClass))
#define HIF_IS_TRANSACTION(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), HIF_TYPE_TRANSACTION))
#define HIF_IS_TRANSACTION_CLASS(cls)	(G_TYPE_CHECK_CLASS_TYPE((cls), HIF_TYPE_TRANSACTION))
#define HIF_TRANSACTION_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), HIF_TYPE_TRANSACTION, HifTransactionClass))

G_BEGIN_DECLS

typedef struct _HifTransactionClass	HifTransactionClass;

struct _HifTransaction
{
	GObject			parent;
};

struct _HifTransactionClass
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

/**
 * HifTransactionFlag:
 * @HIF_TRANSACTION_FLAG_NONE:			No flags
 * @HIF_TRANSACTION_FLAG_ONLY_TRUSTED:		Only install trusted packages
 * @HIF_TRANSACTION_FLAG_ALLOW_REINSTALL:	Allow package reinstallation
 * @HIF_TRANSACTION_FLAG_ALLOW_DOWNGRADE:	Allow package downrades
 * @HIF_TRANSACTION_FLAG_NODOCS:	        Don't install documentation
 * @HIF_TRANSACTION_FLAG_OSTREE_MODE:	        Assume installation on top of an OSTree root
 *
 * The transaction flags.
 **/
typedef enum {
	HIF_TRANSACTION_FLAG_NONE				= 0,
	HIF_TRANSACTION_FLAG_ONLY_TRUSTED		= 1 << 0,
	HIF_TRANSACTION_FLAG_ALLOW_REINSTALL	= 1 << 1,
	HIF_TRANSACTION_FLAG_ALLOW_DOWNGRADE	= 1 << 2,
	HIF_TRANSACTION_FLAG_NODOCS		= 1 << 3,
	HIF_TRANSACTION_FLAG_OSTREE_MODE	= 1 << 4
} HifTransactionFlag;

GType		 hif_transaction_get_type		(void);
HifTransaction	*hif_transaction_new			(HifContext	*context);

/* getters */
guint64		 hif_transaction_get_flags		(HifTransaction	*transaction);
GPtrArray	*hif_transaction_get_remote_pkgs	(HifTransaction	*transaction);
HifDb		*hif_transaction_get_db			(HifTransaction	*transaction);

/* setters */
void		 hif_transaction_set_sources		(HifTransaction	*transaction,
							 GPtrArray	*sources);
void		 hif_transaction_set_uid		(HifTransaction	*transaction,
							 guint		 uid);
void		 hif_transaction_set_flags		(HifTransaction	*transaction,
							 guint64	 flags);

/* object methods */
gboolean	 hif_transaction_depsolve		(HifTransaction	*transaction,
							 HyGoal		 goal,
							 HifState	*state,
							 GError		**error);
gboolean	 hif_transaction_download		(HifTransaction	*transaction,
							 HifState	*state,
							 GError		**error);
gboolean	 hif_transaction_commit			(HifTransaction	*transaction,
							 HyGoal		 goal,
							 HifState	*state,
							 GError		**error);
gboolean	 hif_transaction_ensure_source		(HifTransaction *transaction,
							 HyPackage	 pkg,
							 GError		**error);
gboolean	 hif_transaction_ensure_source_list	(HifTransaction *transaction,
							 HyPackageList	 pkglist,
							 GError		**error);

G_END_DECLS

#endif /* __HIF_TRANSACTION_H */
