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

/**
 * SECTION:as-source
 * @short_description: Object representing a remote source.
 * @include: libhif.h
 * @stability: Stable
 *
 * Sources are remote repositories of packages.
 *
 * See also: #HifSource
 */

#include "config.h"

#include "hif-source.h"

typedef struct _HifSourcePrivate	HifSourcePrivate;
struct _HifSourcePrivate
{
	gchar			*id;
};

G_DEFINE_TYPE_WITH_PRIVATE (HifSource, hif_source, G_TYPE_OBJECT)

#define GET_PRIVATE(o) (hif_source_get_instance_private (o))

/**
 * hif_source_finalize:
 **/
static void
hif_source_finalize (GObject *object)
{
	HifSource *source = HIF_SOURCE (object);
	HifSourcePrivate *priv = GET_PRIVATE (source);

	g_free (priv->id);

	G_OBJECT_CLASS (hif_source_parent_class)->finalize (object);
}

/**
 * hif_source_init:
 **/
static void
hif_source_init (HifSource *source)
{
}

/**
 * hif_source_class_init:
 **/
static void
hif_source_class_init (HifSourceClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = hif_source_finalize;
}

/**
 * hif_source_get_id:
 * @source: a #HifSource instance.
 *
 * Gets the source ID.
 *
 * Returns: the source ID, e.g. "fedora-updates"
 *
 * Since: 0.1.0
 **/
const gchar *
hif_source_get_id (HifSource *source)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	return priv->id;
}

/**
 * hif_source_set_id:
 * @source: a #HifSource instance.
 * @id: the ID, e.g. "fedora-updates"
 *
 * Sets the source ID.
 *
 * Since: 0.1.0
 **/
void
hif_source_set_id (HifSource *source, const gchar *id)
{
	HifSourcePrivate *priv = GET_PRIVATE (source);
	g_free (priv->id);
	priv->id = g_strdup (id);
}

/**
 * hif_source_new:
 *
 * Creates a new #HifSource.
 *
 * Returns: (transfer full): a #HifSource
 *
 * Since: 0.1.0
 **/
HifSource *
hif_source_new (void)
{
	HifSource *source;
	source = g_object_new (HIF_TYPE_SOURCE, NULL);
	return HIF_SOURCE (source);
}
