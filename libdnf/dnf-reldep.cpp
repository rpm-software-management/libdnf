/*
 * Copyright © 2016  Igor Gnatenko <ignatenko@redhat.com>
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

#include <solv/pool.h>

#include "dnf-types.h"
#include "dnf-sack-private.hpp"
#include "repo/solvable/Dependency.hpp"


/**
 * dnf_reldep_new:
 * @sack: a #DnfSack
 * @name: Name
 * @cmp_type: Comparison type
 * @evr: (nullable): EVR
 *
 * Returns: an #DnfReldep
 *
 * Since: 0.7.0
 */
DnfReldep *
dnf_reldep_new (DnfSack *sack, const char *name, int  cmp_type, const char *evr)
{
    return new libdnf::Dependency(sack, name, evr, cmp_type);
}

/**
 * dnf_reldep_to_string:
 * @reldep: a #DnfReldep
 *
 * Returns: a string
 *
 * Since: 0.7.0
 */
const char *
dnf_reldep_to_string (DnfReldep *reldep)
{
    return reldep->toString();
}

Id
dnf_reldep_get_id (DnfReldep *reldep)
{
    return reldep->getId();
}

void dnf_reldep_free(DnfReldep *reldep)
{
    delete reldep;
}
