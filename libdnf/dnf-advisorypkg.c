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
 * SECTION:dnf-advisorypkg
 * @short_description: Update advisory package
 * @include: libdnf.h
 * @stability: Unstable
 *
 * This object represents an package listed as an advisory.
 *
 * See also: #DnfAdvisory
 */


#include <solv/evr.h>
#include <solv/util.h>

#include "dnf-advisorypkg-private.h"

typedef struct
{
    char    *name;
    char    *evr;
    char    *arch;
    char    *filename;
} DnfAdvisoryPkgPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(DnfAdvisoryPkg, dnf_advisorypkg, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (dnf_advisorypkg_get_instance_private (o))

/**
 * dnf_advisorypkg_finalize:
 **/
static void
dnf_advisorypkg_finalize(GObject *object)
{
    DnfAdvisoryPkg *advisorypkg = DNF_ADVISORY_PKG(object);
    DnfAdvisoryPkgPrivate *priv = GET_PRIVATE(advisorypkg);

    g_free(priv->name);
    g_free(priv->evr);
    g_free(priv->arch);
    g_free(priv->filename);

    G_OBJECT_CLASS(dnf_advisorypkg_parent_class)->finalize(object);
}

/**
 * dnf_advisorypkg_init:
 **/
static void
dnf_advisorypkg_init(DnfAdvisoryPkg *advisorypkg)
{
}

/**
 * dnf_advisorypkg_class_init:
 **/
static void
dnf_advisorypkg_class_init(DnfAdvisoryPkgClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = dnf_advisorypkg_finalize;
}

/**
 * dnf_advisorypkg_set_name:
 * @advisorypkg: a #DnfAdvisoryPkg instance.
 * @value: new value
 *
 * Sets the advisory package name.
*
 * Since: 0.7.0
 */
void
dnf_advisorypkg_set_name(DnfAdvisoryPkg *advisorypkg, const char *value)
{
    DnfAdvisoryPkgPrivate *priv = GET_PRIVATE(advisorypkg);
    g_free(priv->name);
    priv->name = g_strdup(value);
}

/**
 * dnf_advisorypkg_set_evr:
 * @advisorypkg: a #DnfAdvisoryPkg instance.
 * @value: new value
 *
 * Sets the advisory package EVR.
*
 * Since: 0.7.0
 */
void
dnf_advisorypkg_set_evr(DnfAdvisoryPkg *advisorypkg, const char *value)
{
    DnfAdvisoryPkgPrivate *priv = GET_PRIVATE(advisorypkg);
    g_free(priv->evr);
    priv->evr = g_strdup(value);
}

/**
 * dnf_advisorypkg_set_arch:
 * @advisorypkg: a #DnfAdvisoryPkg instance.
 * @value: new value
 *
 * Sets the advisory package architecture.
*
 * Since: 0.7.0
 */
void
dnf_advisorypkg_set_arch(DnfAdvisoryPkg *advisorypkg, const char *value)
{
    DnfAdvisoryPkgPrivate *priv = GET_PRIVATE(advisorypkg);
    g_free(priv->arch);
    priv->arch = g_strdup(value);
}

/**
 * dnf_advisorypkg_set_filename:
 * @advisorypkg: a #DnfAdvisoryPkg instance.
 * @value: new value
 *
 * Sets the advisory package filename.
*
 * Since: 0.7.0
 */
void
dnf_advisorypkg_set_filename(DnfAdvisoryPkg *advisorypkg, const char *value)
{
    DnfAdvisoryPkgPrivate *priv = GET_PRIVATE(advisorypkg);
    g_free(priv->filename);
    priv->filename = g_strdup(value);
}

/**
 * dnf_advisorypkg_compare:
 * @left: a #DnfAdvisoryPkg instance.
 * @right: another #DnfAdvisoryPkg instance.
 *
 * Compares one advisory agains another.
 *
 * Returns: 0 if they are the same
 *
 * Since: 0.7.0
 */
int
dnf_advisorypkg_compare(DnfAdvisoryPkg *left, DnfAdvisoryPkg *right)
{
    DnfAdvisoryPkgPrivate *lpriv = GET_PRIVATE(left);
    DnfAdvisoryPkgPrivate *rpriv = GET_PRIVATE(right);
    return
        g_strcmp0(lpriv->name, rpriv->name) ||
        g_strcmp0(lpriv->evr, rpriv->evr) ||
        g_strcmp0(lpriv->arch, rpriv->arch) ||
        g_strcmp0(lpriv->filename, rpriv->filename);
}

/**
 * dnf_advisorypkg_compare_solvable:
 * @advisorypkg: a #DnfAdvisoryPkg instance.
 * @pool: package pool
 * @solvable: solvable
 *
 * Compares advisorypkg against solvable
 *
 * Returns: 0 if they are the same
 *
 * Since: 0.7.0
 */
gboolean
dnf_advisorypkg_compare_solvable(DnfAdvisoryPkg *advisorypkg, Pool *pool, Solvable *s)
{
    DnfAdvisoryPkgPrivate *priv = GET_PRIVATE(advisorypkg);

    const char *n = pool_id2str(pool, s->name);
    const char *e = pool_id2str(pool, s->evr);
    const char *a = pool_id2str(pool, s->arch);

    return
        g_strcmp0(priv->name, n) ||
        pool_evrcmp_str(pool, priv->evr, e, EVRCMP_COMPARE) ||
        g_strcmp0(priv->arch, a);
}

/**
 * dnf_advisorypkg_get_name:
 * @advisorypkg: a #DnfAdvisoryPkg instance.
 *
 * Returns the advisory package name.
 *
 * Returns: string, or %NULL.
 *
 * Since: 0.7.0
 */
const char *
dnf_advisorypkg_get_name(DnfAdvisoryPkg *advisorypkg)
{
    DnfAdvisoryPkgPrivate *priv = GET_PRIVATE(advisorypkg);
    return priv->name;
}

/**
 * dnf_advisorypkg_get_evr:
 * @advisorypkg: a #DnfAdvisoryPkg instance.
 *
 * Returns the advisory package EVR.
 *
 * Returns: string, or %NULL.
 *
 * Since: 0.7.0
 */
const char *
dnf_advisorypkg_get_evr(DnfAdvisoryPkg *advisorypkg)
{
    DnfAdvisoryPkgPrivate *priv = GET_PRIVATE(advisorypkg);
    return priv->evr;
}

/**
 * dnf_advisorypkg_get_arch:
 * @advisorypkg: a #DnfAdvisoryPkg instance.
 *
 * Returns the advisory package architecture.
 *
 * Returns: string, or %NULL.
 *
 * Since: 0.7.0
 */
const char *
dnf_advisorypkg_get_arch(DnfAdvisoryPkg *advisorypkg)
{
    DnfAdvisoryPkgPrivate *priv = GET_PRIVATE(advisorypkg);
    return priv->arch;
}

/**
 * dnf_advisorypkg_get_filename:
 * @advisorypkg: a #DnfAdvisoryPkg instance.
 *
 * Returns the advisory package filename.
 *
 * Returns: string, or %NULL.
 *
 * Since: 0.7.0
 */
const char *
dnf_advisorypkg_get_filename(DnfAdvisoryPkg *advisorypkg)
{
    DnfAdvisoryPkgPrivate *priv = GET_PRIVATE(advisorypkg);
    return priv->filename;
}

/**
 * dnf_advisorypkg_new:
 *
 * Creates a new #DnfAdvisoryPkg.
 *
 * Returns:(transfer full): a #DnfAdvisoryPkg
 *
 * Since: 0.7.0
 **/
DnfAdvisoryPkg *
dnf_advisorypkg_new(void)
{
    DnfAdvisoryPkg *advisorypkg;
    advisorypkg = g_object_new(DNF_TYPE_ADVISORY_PKG, NULL);
    return DNF_ADVISORY_PKG(advisorypkg);
}
