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
 * SECTION:hif-advisoryref
 * @short_description: Update advisory reference
 * @include: libhif.h
 * @stability: Unstable
 *
 * This object represents a reference given to an update.
 *
 * See also: #HifContext
 */


#include <solv/repo.h>
#include <solv/util.h>

#include "hif-advisoryref-private.h"

typedef struct
{
    Pool    *pool;
    Id       a_id;
    int      index;
} HifAdvisoryRefPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(HifAdvisoryRef, hif_advisoryref, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (hif_advisoryref_get_instance_private (o))

/**
 * hif_advisoryref_init:
 **/
static void
hif_advisoryref_init(HifAdvisoryRef *advisoryref)
{
}

/**
 * hif_advisoryref_class_init:
 **/
static void
hif_advisoryref_class_init(HifAdvisoryRefClass *klass)
{
}

/**
 * hif_advisoryref_new:
 *
 * Creates a new #HifAdvisoryRef.
 *
 * Returns:(transfer full): a #HifAdvisoryRef
 *
 * Since: 0.7.0
 **/
HifAdvisoryRef *
hif_advisoryref_new(Pool *pool, Id a_id, int index)
{
    HifAdvisoryRef *advisoryref;
    HifAdvisoryRefPrivate *priv;
    advisoryref = g_object_new(HIF_TYPE_ADVISORYREF, NULL);
    priv = GET_PRIVATE(advisoryref);
    priv->pool = pool;
    priv->a_id = a_id;
    priv->index = index;
    return HIF_ADVISORYREF(advisoryref);
}

/**
 * hif_advisoryref_compare:
 * @left: a #HifAdvisoryRef instance.
 * @right: another #HifAdvisoryRef instance.
 *
 * Checks two #HifAdvisoryRef objects for equality.
 *
 * Returns: TRUE if the #HifAdvisoryRef objects are the same
 *
 * Since: 0.7.0
 */
int
hif_advisoryref_compare(HifAdvisoryRef *left, HifAdvisoryRef *right)
{
    HifAdvisoryRefPrivate *lpriv = GET_PRIVATE(left);
    HifAdvisoryRefPrivate *rpriv = GET_PRIVATE(right);
    return (lpriv->a_id == rpriv->a_id) && (lpriv->index == rpriv->index);
}

static const char *
advisoryref_get_str(HifAdvisoryRef *advisoryref, Id keyname)
{
    HifAdvisoryRefPrivate *priv = GET_PRIVATE(advisoryref);
    Dataiterator di;
    const char *str = NULL;
    int count = 0;

    dataiterator_init(&di, priv->pool, 0, priv->a_id, UPDATE_REFERENCE, 0, 0);
    while (dataiterator_step(&di)) {
        dataiterator_setpos(&di);
        if (count++ == priv->index) {
            str = pool_lookup_str(priv->pool, SOLVID_POS, keyname);
            break;
        }
    }
    dataiterator_free(&di);

    return str;
}

/**
 * hif_advisoryref_get_kind:
 * @advisoryref: a #HifAdvisoryRef instance.
 *
 * Gets the kind of advisory reference.
 *
 * Returns: a #HifAdvisoryRef, e.g. %HIF_REFERENCE_KIND_BUGZILLA
 *
 * Since: 0.7.0
 */
HifAdvisoryRefKind
hif_advisoryref_get_kind(HifAdvisoryRef *advisoryref)
{
    const char *type;
    type = advisoryref_get_str(advisoryref, UPDATE_REFERENCE_TYPE);
    if (type == NULL)
        return HIF_REFERENCE_KIND_UNKNOWN;
    if (!g_strcmp0 (type, "bugzilla"))
        return HIF_REFERENCE_KIND_BUGZILLA;
    if (!g_strcmp0 (type, "cve"))
        return HIF_REFERENCE_KIND_CVE;
    if (!g_strcmp0 (type, "vendor"))
        return HIF_REFERENCE_KIND_VENDOR;
    return HIF_REFERENCE_KIND_UNKNOWN;
}

/**
 * hif_advisoryref_get_id:
 * @advisoryref: a #HifAdvisoryRef instance.
 *
 * Gets an ID for the advisory.
 *
 * Returns: the advisory ID
 *
 * Since: 0.7.0
 */
const char *
hif_advisoryref_get_id(HifAdvisoryRef *advisoryref)
{
    return advisoryref_get_str(advisoryref, UPDATE_REFERENCE_ID);
}

/**
 * hif_advisoryref_get_title:
 * @advisoryref: a #HifAdvisoryRef instance.
 *
 * Gets a title to use for the advisory.
 *
 * Returns: the advisory title
 *
 * Since: 0.7.0
 */
const char *
hif_advisoryref_get_title(HifAdvisoryRef *advisoryref)
{
    return advisoryref_get_str(advisoryref, UPDATE_REFERENCE_TITLE);
}

/**
 * hif_advisoryref_get_url:
 * @advisoryref: a #HifAdvisoryRef instance.
 *
 * Gets the link for the advisory.
 *
 * Returns: the advisory URL
 *
 * Since: 0.7.0
 */
const char *
hif_advisoryref_get_url(HifAdvisoryRef *advisoryref)
{
    return advisoryref_get_str(advisoryref, UPDATE_REFERENCE_HREF);
}
