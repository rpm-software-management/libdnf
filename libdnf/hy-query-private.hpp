/*
 * Copyright (C) 2012-2018 Red Hat, Inc.
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

#ifndef HY_QUERY_INTERNAL_H
#define HY_QUERY_INTERNAL_H

// libsolv
#include <solv/bitmap.h>

// hawkey
#include "hy-query.h"

// swdb
#include "transaction/Swdb.hpp"
#include "goal/IdQueue.hpp"

enum _match_type {
    _HY_VOID,
    _HY_NUM,
    _HY_PKG,
    _HY_RELDEP,
    _HY_STR,
};

int hy_filter_unneeded(HyQuery query, const libdnf::Swdb &swdb, bool debug_solver);

namespace libdnf {
void hy_query_to_name_ordered_queue(HyQuery query, libdnf::IdQueue * samename);
void hy_query_to_name_arch_ordered_queue(HyQuery query, libdnf::IdQueue * samename);
}

/**
* @brief Return HySelector from HyQuery
*
* @param query query
* @return HySelector
*/
HySelector hy_query_to_selector(HyQuery query);

#endif // HY_QUERY_INTERNAL_H
