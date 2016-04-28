/*
 * Copyright (C) 2014 Red Hat, Inc.
 * Copyright (C) 2015 Richard Hughes <richard@hughsie.com>
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

/**
 * SECTION:hif-advisorypkg
 * @short_description: Update advisory package
 * @include: libhif.h
 * @stability: Unstable
 *
 * This object represents an package listed as an advisory.
 *
 * See also: #HifAdvisory
 */


#include <solv/evr.h>
#include <solv/util.h>

#include "hif-advisorypkg-private.h"

typedef struct
{
    char    *name;
    char    *evr;
    char    *arch;
    char    *filename;
} HifAdvisoryPkgPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(HifAdvisoryPkg, hif_advisorypkg, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (hif_advisorypkg_get_instance_private (o))

/**
 * hif_advisorypkg_finalize:
 **/
static void
hif_advisorypkg_finalize(GObject *object)
{
    HifAdvisoryPkg *advisorypkg = HIF_ADVISORY_PKG(object);
    HifAdvisoryPkgPrivate *priv = GET_PRIVATE(advisorypkg);

    g_free(priv->name);
    g_free(priv->evr);
    g_free(priv->arch);
    g_free(priv->filename);

    G_OBJECT_CLASS(hif_advisorypkg_parent_class)->finalize(object);
}

/**
 * hif_advisorypkg_init:
 **/
static void
hif_advisorypkg_init(HifAdvisoryPkg *advisorypkg)
{
}

/**
 * hif_advisorypkg_class_init:
 **/
static void
hif_advisorypkg_class_init(HifAdvisoryPkgClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = hif_advisorypkg_finalize;
}

/**
 * hif_advisorypkg_set_name:
 * @advisorypkg: a #HifAdvisoryPkg instance.
 * @value: new value
 *
 * Sets the advisory package name.
*
 * Since: 0.7.0
 */
void
hif_advisorypkg_set_name(HifAdvisoryPkg *advisorypkg, const char *value)
{
    HifAdvisoryPkgPrivate *priv = GET_PRIVATE(advisorypkg);
    g_free(priv->name);
    priv->name = g_strdup(value);
}

/**
 * hif_advisorypkg_set_evr:
 * @advisorypkg: a #HifAdvisoryPkg instance.
 * @value: new value
 *
 * Sets the advisory package EVR.
*
 * Since: 0.7.0
 */
void
hif_advisorypkg_set_evr(HifAdvisoryPkg *advisorypkg, const char *value)
{
    HifAdvisoryPkgPrivate *priv = GET_PRIVATE(advisorypkg);
    g_free(priv->evr);
    priv->evr = g_strdup(value);
}

/**
 * hif_advisorypkg_set_arch:
 * @advisorypkg: a #HifAdvisoryPkg instance.
 * @value: new value
 *
 * Sets the advisory package architecture.
*
 * Since: 0.7.0
 */
void
hif_advisorypkg_set_arch(HifAdvisoryPkg *advisorypkg, const char *value)
{
    HifAdvisoryPkgPrivate *priv = GET_PRIVATE(advisorypkg);
    g_free(priv->arch);
    priv->arch = g_strdup(value);
}

/**
 * hif_advisorypkg_set_filename:
 * @advisorypkg: a #HifAdvisoryPkg instance.
 * @value: new value
 *
 * Sets the advisory package filename.
*
 * Since: 0.7.0
 */
void
hif_advisorypkg_set_filename(HifAdvisoryPkg *advisorypkg, const char *value)
{
    HifAdvisoryPkgPrivate *priv = GET_PRIVATE(advisorypkg);
    g_free(priv->filename);
    priv->filename = g_strdup(value);
}

/**
 * hif_advisorypkg_compare:
 * @left: a #HifAdvisoryPkg instance.
 * @right: another #HifAdvisoryPkg instance.
 *
 * Compares one advisory agains another.
 *
 * Returns: 0 if they are the same
 *
 * Since: 0.7.0
 */
int
hif_advisorypkg_compare(HifAdvisoryPkg *left, HifAdvisoryPkg *right)
{
    HifAdvisoryPkgPrivate *lpriv = GET_PRIVATE(left);
    HifAdvisoryPkgPrivate *rpriv = GET_PRIVATE(right);
    return
        g_strcmp0(lpriv->name, rpriv->name) ||
        g_strcmp0(lpriv->evr, rpriv->evr) ||
        g_strcmp0(lpriv->arch, rpriv->arch) ||
        g_strcmp0(lpriv->filename, rpriv->filename);
}

/**
 * hif_advisorypkg_compare_solvable:
 * @advisorypkg: a #HifAdvisoryPkg instance.
 * @pool: package pool
 * @solvable: solvable
 *
 * Compares advisorypkg against solvable
 *
 * Returns: 0 if they are the same
 *
 * Since: 0.7.0
 */
int
hif_advisorypkg_compare_solvable(HifAdvisoryPkg *advisorypkg, Pool *pool, Solvable *s)
{
    HifAdvisoryPkgPrivate *priv = GET_PRIVATE(advisorypkg);

    const char *n = pool_id2str(pool, s->name);
    const char *e = pool_id2str(pool, s->evr);
    const char *a = pool_id2str(pool, s->arch);

    return
        g_strcmp0(priv->name, n) ||
        pool_evrcmp_str(pool, priv->evr, e, EVRCMP_COMPARE) ||
        g_strcmp0(priv->arch, a);
}

/**
 * hif_advisorypkg_get_name:
 * @advisorypkg: a #HifAdvisoryPkg instance.
 *
 * Returns the advisory package name.
 *
 * Returns: string, or %NULL.
 *
 * Since: 0.7.0
 */
const char *
hif_advisorypkg_get_name(HifAdvisoryPkg *advisorypkg)
{
    HifAdvisoryPkgPrivate *priv = GET_PRIVATE(advisorypkg);
    return priv->name;
}

/**
 * hif_advisorypkg_get_evr:
 * @advisorypkg: a #HifAdvisoryPkg instance.
 *
 * Returns the advisory package EVR.
 *
 * Returns: string, or %NULL.
 *
 * Since: 0.7.0
 */
const char *
hif_advisorypkg_get_evr(HifAdvisoryPkg *advisorypkg)
{
    HifAdvisoryPkgPrivate *priv = GET_PRIVATE(advisorypkg);
    return priv->evr;
}

/**
 * hif_advisorypkg_get_arch:
 * @advisorypkg: a #HifAdvisoryPkg instance.
 *
 * Returns the advisory package architecture.
 *
 * Returns: string, or %NULL.
 *
 * Since: 0.7.0
 */
const char *
hif_advisorypkg_get_arch(HifAdvisoryPkg *advisorypkg)
{
    HifAdvisoryPkgPrivate *priv = GET_PRIVATE(advisorypkg);
    return priv->arch;
}

/**
 * hif_advisorypkg_get_filename:
 * @advisorypkg: a #HifAdvisoryPkg instance.
 *
 * Returns the advisory package filename.
 *
 * Returns: string, or %NULL.
 *
 * Since: 0.7.0
 */
const char *
hif_advisorypkg_get_filename(HifAdvisoryPkg *advisorypkg)
{
    HifAdvisoryPkgPrivate *priv = GET_PRIVATE(advisorypkg);
    return priv->filename;
}

/**
 * hif_advisorypkg_new:
 *
 * Creates a new #HifAdvisoryPkg.
 *
 * Returns:(transfer full): a #HifAdvisoryPkg
 *
 * Since: 0.7.0
 **/
HifAdvisoryPkg *
hif_advisorypkg_new(void)
{
    HifAdvisoryPkg *advisorypkg;
    advisorypkg = g_object_new(HIF_TYPE_ADVISORY_PKG, NULL);
    return HIF_ADVISORY_PKG(advisorypkg);
}
