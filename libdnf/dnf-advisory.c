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
 * SECTION:dnf-advisory
 * @short_description: Update advisory
 * @include: libdnf.h
 * @stability: Unstable
 *
 * This object represents a single update.
 *
 * See also: #DnfContext
 */


#include <glib.h>

#include <solv/repo.h>
#include <solv/util.h>

#include "dnf-advisory-private.h"
#include "dnf-advisorypkg-private.h"
#include "dnf-advisoryref-private.h"
#include "hy-iutil.h"

typedef struct
{
    Pool    *pool;
    Id       a_id;
} DnfAdvisoryPrivate;

G_DEFINE_TYPE_WITH_PRIVATE(DnfAdvisory, dnf_advisory, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (dnf_advisory_get_instance_private (o))

static DnfAdvisoryKind str2dnf_advisory_kind(const char *str);

/**
 * dnf_advisory_init:
 **/
static void
dnf_advisory_init(DnfAdvisory *advisory)
{
}

/**
 * dnf_advisory_class_init:
 **/
static void
dnf_advisory_class_init(DnfAdvisoryClass *klass)
{
}

/**
 * dnf_advisory_new:
 *
 * Creates a new #DnfAdvisory.
 *
 * Returns:(transfer full): a #DnfAdvisory
 *
 * Since: 0.7.0
 **/
DnfAdvisory *
dnf_advisory_new(Pool *pool, Id a_id)
{
    DnfAdvisory *advisory;
    DnfAdvisoryPrivate *priv;
    advisory = g_object_new(DNF_TYPE_ADVISORY, NULL);
    priv = GET_PRIVATE(advisory);
    priv->pool = pool;
    priv->a_id = a_id;
    return DNF_ADVISORY(advisory);
}

/**
 * dnf_advisory_compare:
 * @left: a #DnfAdvisory instance.
 * @right: another #DnfAdvisory instance.
 *
 * Compares two #DnfAdvisory objects for equality.
 *
 * Returns: %TRUE if they are the same
 *
 * Since: 0.7.0
 */
int
dnf_advisory_compare(DnfAdvisory *left, DnfAdvisory *right)
{
    DnfAdvisoryPrivate *lpriv = GET_PRIVATE(left);
    DnfAdvisoryPrivate *rpriv = GET_PRIVATE(right);
    return lpriv->a_id == rpriv->a_id;
}

/**
 * dnf_advisory_get_title:
 * @advisory: a #DnfAdvisory instance.
 *
 * Gets the title assigned to the advisory.
 *
 * Returns: a string value, or %NULL for unset
 *
 * Since: 0.7.0
 */
const char *
dnf_advisory_get_title(DnfAdvisory *advisory)
{
    DnfAdvisoryPrivate *priv = GET_PRIVATE(advisory);
    return pool_lookup_str(priv->pool, priv->a_id, SOLVABLE_SUMMARY);
}

/**
 * dnf_advisory_get_id:
 * @advisory: a #DnfAdvisory instance.
 *
 * Gets the ID assigned to the advisory.
 *
 * Returns: a string value, or %NULL for unset
 *
 * Since: 0.7.0
 */
const char *
dnf_advisory_get_id(DnfAdvisory *advisory)
{
    DnfAdvisoryPrivate *priv = GET_PRIVATE(advisory);
    const char *id;

    id = pool_lookup_str(priv->pool, priv->a_id, SOLVABLE_NAME);
    g_assert(g_str_has_prefix(id, SOLVABLE_NAME_ADVISORY_PREFIX));
    //remove the prefix
    id += strlen(SOLVABLE_NAME_ADVISORY_PREFIX);

    return id;
}

/**
 * dnf_advisory_get_kind:
 * @advisory: a #DnfAdvisory instance.
 *
 * Gets the advisory kind.
 *
 * Returns: a #DnfAdvisoryKind, e.g. %DNF_ADVISORY_KIND_BUGFIX
 *
 * Since: 0.7.0
 */
DnfAdvisoryKind
dnf_advisory_get_kind(DnfAdvisory *advisory)
{
    DnfAdvisoryPrivate *priv = GET_PRIVATE(advisory);
    const char *type;
    type = pool_lookup_str(priv->pool, priv->a_id, SOLVABLE_PATCHCATEGORY);
    return str2dnf_advisory_kind(type);
}

/**
 * dnf_advisory_get_description:
 * @advisory: a #DnfAdvisory instance.
 *
 * Gets the advisory description.
 *
 * Returns: a string value, or %NULL for unset
 *
 * Since: 0.7.0
 */
const char *
dnf_advisory_get_description(DnfAdvisory *advisory)
{
    DnfAdvisoryPrivate *priv = GET_PRIVATE(advisory);
    return pool_lookup_str(priv->pool, priv->a_id, SOLVABLE_DESCRIPTION);
}

/**
 * dnf_advisory_get_rights:
 * @advisory: a #DnfAdvisory instance.
 *
 * Gets the update rights for the advisory.
 *
 * Returns: a string value, or %NULL for unset
 *
 * Since: 0.7.0
 */
const char *
dnf_advisory_get_rights(DnfAdvisory *advisory)
{
    DnfAdvisoryPrivate *priv = GET_PRIVATE(advisory);
    return pool_lookup_str(priv->pool, priv->a_id, UPDATE_RIGHTS);
}

/**
 * dnf_advisory_get_updated:
 * @advisory: a #DnfAdvisory instance.
 *
 * Gets the timestamp when the advisory was created.
 *
 * Returns: %TRUE for success
 *
 * Since: 0.7.0
 */
guint64
dnf_advisory_get_updated(DnfAdvisory *advisory)
{
    DnfAdvisoryPrivate *priv = GET_PRIVATE(advisory);
    return pool_lookup_num(priv->pool, priv->a_id, SOLVABLE_BUILDTIME, 0);
}

/**
 * dnf_advisory_get_packages:
 * @advisory: a #DnfAdvisory instance.
 *
 * Gets any packages referenced by the advisory.
 *
 * Returns: (transfer container) (element-type DnfAdvisoryPkg): a list of packages
 *
 * Since: 0.7.0
 */
GPtrArray *
dnf_advisory_get_packages(DnfAdvisory *advisory)
{
    DnfAdvisoryPrivate *priv = GET_PRIVATE(advisory);
    Dataiterator di;
    DnfAdvisoryPkg *pkg;
    GPtrArray *pkglist = g_ptr_array_new_with_free_func((GDestroyNotify) g_object_unref);

    dataiterator_init(&di, priv->pool, 0, priv->a_id, UPDATE_COLLECTION, 0, 0);
    while (dataiterator_step(&di)) {
        dataiterator_setpos(&di);
        pkg = dnf_advisorypkg_new();
        dnf_advisorypkg_set_name(pkg,
                pool_lookup_str(priv->pool, SOLVID_POS, UPDATE_COLLECTION_NAME));
        dnf_advisorypkg_set_evr(pkg,
                pool_lookup_str(priv->pool, SOLVID_POS, UPDATE_COLLECTION_EVR));
        dnf_advisorypkg_set_arch(pkg,
                pool_lookup_str(priv->pool, SOLVID_POS, UPDATE_COLLECTION_ARCH));
        dnf_advisorypkg_set_filename(pkg,
                pool_lookup_str(priv->pool, SOLVID_POS, UPDATE_COLLECTION_FILENAME));
        g_ptr_array_add(pkglist, pkg);
    }
    dataiterator_free(&di);

    return pkglist;
}

/**
 * dnf_advisory_get_references:
 * @advisory: a #DnfAdvisory instance.
 *
 * Gets any references referenced by the advisory.
 *
 * Returns: (transfer container) (element-type DnfAdvisoryRef): a list of references
 *
 * Since: 0.7.0
 */
GPtrArray *
dnf_advisory_get_references(DnfAdvisory *advisory)
{
    DnfAdvisoryPrivate *priv = GET_PRIVATE(advisory);
    Dataiterator di;
    DnfAdvisoryRef *ref;
    GPtrArray *reflist = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);

    dataiterator_init(&di, priv->pool, 0, priv->a_id, UPDATE_REFERENCE, 0, 0);
    for (int index = 0; dataiterator_step(&di); index++) {
        ref = dnf_advisoryref_new(priv->pool, priv->a_id, index);
        g_ptr_array_add(reflist, ref);
    }
    dataiterator_free(&di);

    return reflist;
}

/**
 * str2dnf_advisory_kind:
 * @str: a string
 *
 * Returns: a #DnfAdvisoryKind, e.g. %DNF_ADVISORY_KIND_BUGFIX
 *
 * Since: 0.7.0
 */
static DnfAdvisoryKind
str2dnf_advisory_kind(const char *str)
{
    if (str == NULL)
        return DNF_ADVISORY_KIND_UNKNOWN;
    if (!strcmp (str, "bugfix"))
        return DNF_ADVISORY_KIND_BUGFIX;
    if (!strcmp (str, "enhancement"))
        return DNF_ADVISORY_KIND_ENHANCEMENT;
    if (!strcmp (str, "security"))
        return DNF_ADVISORY_KIND_SECURITY;
    return DNF_ADVISORY_KIND_UNKNOWN;
}

/**
 * dnf_advisory_match_id:
 * @advisory: a #DnfAdvisory instance.
 * @s: string
 *
 * Matches #DnfAdvisory id against string
 *
 * Returns: %TRUE if they are the same
 *
 * Since: 0.7.0
 */
gboolean
dnf_advisory_match_id(DnfAdvisory *advisory, const char *s)
{
    const char *id = dnf_advisory_get_id(advisory);
    return !g_strcmp0(id, s);
}

/**
 * dnf_advisory_match_kind:
 * @advisory: a #DnfAdvisory instance.
 * @s: string
 *
 * Matches #DnfAdvisory kind against string
 *
 * Returns: %TRUE if they are the same
 *
 * Since: 0.7.0
 */
gboolean
dnf_advisory_match_kind(DnfAdvisory *advisory, const char *s)
{
    DnfAdvisoryKind kind = dnf_advisory_get_kind(advisory);
    DnfAdvisoryKind skind = str2dnf_advisory_kind(s);
    return kind == skind;
}

/**
 * dnf_advisory_match_cve:
 * @advisory: a #DnfAdvisory instance.
 * @s: string
 *
 * Matches if #DnfAdvisoryRef has a #DnfAdvisoryRef which is CVE
 * and matches against string
 *
 * Returns: %TRUE if they are the same
 *
 * Since: 0.7.0
 */
gboolean
dnf_advisory_match_cve(DnfAdvisory *advisory, const char *s)
{
    g_autoptr(GPtrArray) refs = dnf_advisory_get_references(advisory);

    for (guint r = 0; r < refs->len; ++r) {
        DnfAdvisoryRef *ref = g_ptr_array_index(refs, r);
        if (dnf_advisoryref_get_kind(ref) == DNF_REFERENCE_KIND_CVE) {
            const char *rid = dnf_advisoryref_get_id(ref);
            if (!g_strcmp0(rid, s)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

/**
 * dnf_advisory_match_bug:
 * @advisory: a #DnfAdvisory instance.
 * @s: string
 *
 * Matches if #DnfAdvisoryRef has a #DnfAdvisoryRef which is bug
 * and matches against string
 *
 * Returns: %TRUE if they are the same
 *
 * Since: 0.7.0
 */
gboolean
dnf_advisory_match_bug(DnfAdvisory *advisory, const char *s)
{
    g_autoptr(GPtrArray) refs = dnf_advisory_get_references(advisory);

    for (guint r = 0; r < refs->len; ++r) {
        DnfAdvisoryRef *ref = g_ptr_array_index(refs, r);
        if (dnf_advisoryref_get_kind(ref) == DNF_REFERENCE_KIND_BUGZILLA) {
            const char *rid = dnf_advisoryref_get_id(ref);
            if (!g_strcmp0(rid, s)) {
                return TRUE;
            }
        }
    }
    return FALSE;
}
