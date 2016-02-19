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
 * SECTION:hif-advisory
 * @short_description: Update advisory
 * @include: libhif.h
 * @stability: Unstable
 *
 * This object represents a single update.
 *
 * See also: #HifContext
 */


#include <glib.h>

#include <solv/repo.h>
#include <solv/util.h>

#include "hif-advisory-private.h"
#include "hif-advisorypkg-private.h"
#include "hif-advisoryref-private.h"
#include "hy-iutil.h"

typedef struct
{
    Pool    *pool;
    Id       a_id;
} HifAdvisoryPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(HifAdvisory, hif_advisory, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (hif_advisory_get_instance_private (o))

static HifAdvisoryKind str2hif_advisory_kind(const char *str);

/**
 * hif_advisory_init:
 **/
static void
hif_advisory_init(HifAdvisory *advisory)
{
}

/**
 * hif_advisory_class_init:
 **/
static void
hif_advisory_class_init(HifAdvisoryClass *klass)
{
}

/**
 * hif_advisory_new:
 *
 * Creates a new #HifAdvisory.
 *
 * Returns:(transfer full): a #HifAdvisory
 *
 * Since: 0.7.0
 **/
HifAdvisory *
hif_advisory_new(Pool *pool, Id a_id)
{
    HifAdvisory *advisory;
    HifAdvisoryPrivate *priv;
    advisory = g_object_new(HIF_TYPE_ADVISORY, NULL);
    priv = GET_PRIVATE(advisory);
    priv->pool = pool;
    priv->a_id = a_id;
    return HIF_ADVISORY(advisory);
}

/**
 * hif_advisory_compare:
 * @left: a #HifAdvisory instance.
 * @right: another #HifAdvisory instance.
 *
 * Compares two #HifAdvisory objects for equality.
 *
 * Returns: %TRUE if they are the same
 *
 * Since: 0.7.0
 */
int
hif_advisory_compare(HifAdvisory *left, HifAdvisory *right)
{
    HifAdvisoryPrivate *lpriv = GET_PRIVATE(left);
    HifAdvisoryPrivate *rpriv = GET_PRIVATE(right);
    return lpriv->a_id == rpriv->a_id;
}

/**
 * hif_advisory_get_title:
 * @advisory: a #HifAdvisory instance.
 *
 * Gets the title assigned to the advisory.
 *
 * Returns: a string value, or %NULL for unset
 *
 * Since: 0.7.0
 */
const char *
hif_advisory_get_title(HifAdvisory *advisory)
{
    HifAdvisoryPrivate *priv = GET_PRIVATE(advisory);
    return pool_lookup_str(priv->pool, priv->a_id, SOLVABLE_SUMMARY);
}

/**
 * hif_advisory_get_id:
 * @advisory: a #HifAdvisory instance.
 *
 * Gets the ID assigned to the advisory.
 *
 * Returns: a string value, or %NULL for unset
 *
 * Since: 0.7.0
 */
const char *
hif_advisory_get_id(HifAdvisory *advisory)
{
    HifAdvisoryPrivate *priv = GET_PRIVATE(advisory);
    const char *id;

    id = pool_lookup_str(priv->pool, priv->a_id, SOLVABLE_NAME);
    g_assert(g_str_has_prefix(id, SOLVABLE_NAME_ADVISORY_PREFIX));
    //remove the prefix
    id += strlen(SOLVABLE_NAME_ADVISORY_PREFIX);

    return id;
}

/**
 * hif_advisory_get_kind:
 * @advisory: a #HifAdvisory instance.
 *
 * Gets the advisory kind.
 *
 * Returns: a #HifAdvisoryKind, e.g. %HIF_ADVISORY_KIND_BUGFIX
 *
 * Since: 0.7.0
 */
HifAdvisoryKind
hif_advisory_get_kind(HifAdvisory *advisory)
{
    HifAdvisoryPrivate *priv = GET_PRIVATE(advisory);
    const char *type;
    type = pool_lookup_str(priv->pool, priv->a_id, SOLVABLE_PATCHCATEGORY);
    return str2hif_advisory_kind(type);
}

/**
 * hif_advisory_get_description:
 * @advisory: a #HifAdvisory instance.
 *
 * Gets the advisory description.
 *
 * Returns: a string value, or %NULL for unset
 *
 * Since: 0.7.0
 */
const char *
hif_advisory_get_description(HifAdvisory *advisory)
{
    HifAdvisoryPrivate *priv = GET_PRIVATE(advisory);
    return pool_lookup_str(priv->pool, priv->a_id, SOLVABLE_DESCRIPTION);
}

/**
 * hif_advisory_get_rights:
 * @advisory: a #HifAdvisory instance.
 *
 * Gets the update rights for the advisory.
 *
 * Returns: a string value, or %NULL for unset
 *
 * Since: 0.7.0
 */
const char *
hif_advisory_get_rights(HifAdvisory *advisory)
{
    HifAdvisoryPrivate *priv = GET_PRIVATE(advisory);
    return pool_lookup_str(priv->pool, priv->a_id, UPDATE_RIGHTS);
}

/**
 * hif_advisory_get_updated:
 * @advisory: a #HifAdvisory instance.
 *
 * Gets the timestamp when the advisory was created.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
guint64
hif_advisory_get_updated(HifAdvisory *advisory)
{
    HifAdvisoryPrivate *priv = GET_PRIVATE(advisory);
    return pool_lookup_num(priv->pool, priv->a_id, SOLVABLE_BUILDTIME, 0);
}

/**
 * hif_advisory_get_packages:
 * @advisory: a #HifAdvisory instance.
 *
 * Gets any packages referenced by the advisory.
 *
 * Returns: (transfer container) (element-type HifAdvisoryPkg): a list of packages
 *
 * Since: 0.7.0
 */
GPtrArray *
hif_advisory_get_packages(HifAdvisory *advisory)
{
    HifAdvisoryPrivate *priv = GET_PRIVATE(advisory);
    Dataiterator di;
    HifAdvisoryPkg *pkg;
    GPtrArray *pkglist = g_ptr_array_new_with_free_func((GDestroyNotify) g_object_unref);

    dataiterator_init(&di, priv->pool, 0, priv->a_id, UPDATE_COLLECTION, 0, 0);
    while (dataiterator_step(&di)) {
        dataiterator_setpos(&di);
        pkg = hif_advisorypkg_new();
        hif_advisorypkg_set_name(pkg,
                pool_lookup_str(priv->pool, SOLVID_POS, UPDATE_COLLECTION_NAME));
        hif_advisorypkg_set_evr(pkg,
                pool_lookup_str(priv->pool, SOLVID_POS, UPDATE_COLLECTION_EVR));
        hif_advisorypkg_set_arch(pkg,
                pool_lookup_str(priv->pool, SOLVID_POS, UPDATE_COLLECTION_ARCH));
        hif_advisorypkg_set_filename(pkg,
                pool_lookup_str(priv->pool, SOLVID_POS, UPDATE_COLLECTION_FILENAME));
        g_ptr_array_add(pkglist, pkg);
    }
    dataiterator_free(&di);

    return pkglist;
}

/**
 * hif_advisory_get_references:
 * @advisory: a #HifAdvisory instance.
 *
 * Gets any references referenced by the advisory.
 *
 * Returns: (transfer container) (element-type HifAdvisoryRef): a list of references
 *
 * Since: 0.7.0
 */
GPtrArray *
hif_advisory_get_references(HifAdvisory *advisory)
{
    HifAdvisoryPrivate *priv = GET_PRIVATE(advisory);
    Dataiterator di;
    HifAdvisoryRef *ref;
    GPtrArray *reflist = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);

    dataiterator_init(&di, priv->pool, 0, priv->a_id, UPDATE_REFERENCE, 0, 0);
    for (int index = 0; dataiterator_step(&di); index++) {
        ref = hif_advisoryref_new(priv->pool, priv->a_id, index);
        g_ptr_array_add(reflist, ref);
    }
    dataiterator_free(&di);

    return reflist;
}

/**
 * str2hif_advisory_kind:
 * @str: a string
 *
 * Returns: a #HifAdvisoryKind, e.g. %HIF_ADVISORY_KIND_BUGFIX
 *
 * Since: 0.7.0
 */
static HifAdvisoryKind
str2hif_advisory_kind(const char *str)
{
    if (str == NULL)
        return HIF_ADVISORY_KIND_UNKNOWN;
    if (!strcmp (str, "bugfix"))
        return HIF_ADVISORY_KIND_BUGFIX;
    if (!strcmp (str, "enhancement"))
        return HIF_ADVISORY_KIND_ENHANCEMENT;
    if (!strcmp (str, "security"))
        return HIF_ADVISORY_KIND_SECURITY;
    return HIF_ADVISORY_KIND_UNKNOWN;
}

/**
 * hif_advisory_match_id:
 * @advisory: a #HifAdvisory instance.
 * @s: string
 *
 * Matches #HifAdvisory id against string
 *
 * Returns: %TRUE if they are the same
 *
 * Since: 0.7.0
 */
int
hif_advisory_match_id(HifAdvisory *advisory, const char *s)
{
    const char* id = hif_advisory_get_id(advisory);
    return !g_strcmp0(id, s);
}

/**
 * hif_advisory_match_kind:
 * @advisory: a #HifAdvisory instance.
 * @s: string
 *
 * Matches #HifAdvisory kind against string
 *
 * Returns: %TRUE if they are the same
 *
 * Since: 0.7.0
 */
int
hif_advisory_match_kind(HifAdvisory *advisory, const char *s)
{
    HifAdvisoryKind kind = hif_advisory_get_kind(advisory);
    HifAdvisoryKind skind = str2hif_advisory_kind(s);
    return kind == skind;
}

/**
 * hif_advisory_match_cve:
 * @advisory: a #HifAdvisory instance.
 * @s: string
 *
 * Matches if #HifAdvisoryRef has a #HifAdvisoryRef which is CVE
 * and matches against string
 *
 * Returns: %TRUE if they are the same
 *
 * Since: 0.7.0
 */
int
hif_advisory_match_cve(HifAdvisory *advisory, const char *s)
{
    g_autoptr(GPtrArray) refs = hif_advisory_get_references(advisory);

    for (unsigned int r = 0; r < refs->len; ++r) {
        HifAdvisoryRef *ref = g_ptr_array_index(refs, r);
        if (hif_advisoryref_get_kind(ref) == HIF_REFERENCE_KIND_CVE) {
            const char *rid = hif_advisoryref_get_id(ref);
            if (!g_strcmp0(rid, s)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}
