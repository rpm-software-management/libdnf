/*
 * Copyright (C) 2014-2018 Red Hat, Inc.
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


#include "dnf-advisory-private.hpp"
#include "dnf-advisorypkg.h"
#include "dnf-advisoryref.h"
#include "sack/advisory.hpp"
#include "sack/advisoryref.hpp"
#include "sack/advisorypkg.hpp"


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
dnf_advisory_new(DnfSack *sack, Id a_id)
{
    return new libdnf::Advisory(sack, a_id);
}

/**
 * dnf_advisory_free:
 *
 * Destructor of #DnfAdvisory.
 *
 * Since: 0.13.0
 **/
void
dnf_advisory_free(DnfAdvisory *advisory)
{
    delete advisory;
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
    return left == right;
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
    return advisory->getTitle();
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
    return advisory->getName();
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
    return advisory->getKind();
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
    return advisory->getDescription();
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
    return advisory->getRights();
}

/**
 * dnf_advisory_get_severity:
 * @advisory: a #DnfAdvisory instance.
 *
 * Gets the advisory severity.
 *
 * Returns: a string value, or %NULL for unset
 *
 * Since: 0.8.0
 */
const char *
dnf_advisory_get_severity(DnfAdvisory *advisory)
{
    return advisory->getSeverity();
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
    return advisory->getUpdated();
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
    std::vector<libdnf::AdvisoryPkg> pkgsvector;
    advisory->getPackages(pkgsvector);

    GPtrArray *pkglist = g_ptr_array_new_with_free_func((GDestroyNotify) dnf_advisorypkg_free);
    for (auto& advisorypkg : pkgsvector) {
        g_ptr_array_add(pkglist, new libdnf::AdvisoryPkg(std::move(advisorypkg)));
    }
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
    std::vector<libdnf::AdvisoryRef> refsvector;
    advisory->getReferences(refsvector);

    GPtrArray *reflist = g_ptr_array_new_with_free_func((GDestroyNotify) dnf_advisoryref_free);
    for (const auto& advisoryref : refsvector) {
        g_ptr_array_add(reflist, new libdnf::AdvisoryRef(advisoryref));
    }
    return reflist;
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
    return advisory->matchName(s);
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
    return advisory->matchKind(s);
}

/**
 * dnf_advisory_match_severity:
 * @advisory: a #DnfAdvisory instance.
 * @s: string
 *
 * Matches #DnfAdvisory severity against string
 *
 * Returns: %TRUE if they are the same
 *
 * Since: 0.8.0
 */
gboolean
dnf_advisory_match_severity(DnfAdvisory *advisory, const char *s)
{
    return advisory->matchSeverity(s);
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
    return advisory->matchCVE(s);
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
    return advisory->matchBug(s);
}
