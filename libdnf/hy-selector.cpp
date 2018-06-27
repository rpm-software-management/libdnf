/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * Copyright (C) 2012-2014 Red Hat, Inc.
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

#include "sack/selector.hpp"

HySelector
hy_selector_create(DnfSack *sack)
{
    return new libdnf::Selector(sack);
}

void
hy_selector_free(HySelector sltr)
{
    delete sltr;
}

int
hy_selector_pkg_set(HySelector sltr, DnfPackageSet *pset)
{
    return sltr->set(pset);
}

int
hy_selector_set(HySelector sltr, int keyname, int cmp_type, const char *match)
{
    return sltr->set(keyname, cmp_type, match);
}

GPtrArray *
hy_selector_matches(HySelector sltr)
{
    return sltr->matches();
}
