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

#include "dnf-advisorypkg.h"
#include "sack/advisorypkg.hpp"


/**
 * dnf_advisorypkg_compare:
 * @left: a #DnfAdvisoryPkg instance.
 * @right: another #DnfAdvisoryPkg instance.
 *
 * Compares one advisory against another.
 *
 * Returns: 0 if they are the same
 *
 * Since: 0.7.0
 */
int
dnf_advisorypkg_compare(DnfAdvisoryPkg *left, DnfAdvisoryPkg *right)
{
    return !left->nevraEQ(*right);
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
    return !advisorypkg->nevraEQ(s);
}

/**
 * dnf_advisorypkg_free:
 * @advisorypkg: a #DnfAdvisoryPkg instance.
 *
 * Destructor of #DnfAdvisoryPkg
 *
 * Since: 0.13.0
 */
void
dnf_advisorypkg_free(DnfAdvisoryPkg *advisorypkg)
{
    delete advisorypkg;
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
    return advisorypkg->getNameString();
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
    return advisorypkg->getEVRString();
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
    return advisorypkg->getArchString();
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
    return advisorypkg->getFileName();
}
