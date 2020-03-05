/*
 * Copyright (C) 2018 Red Hat, Inc.
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

#include <solv/pooltypes.h>

#include "advisoryref.hpp"

namespace libdnf {

AdvisoryRef::AdvisoryRef(DnfSack *sack, Id advisory, int index)
: sack(sack), advisory(advisory), index(index) {}

bool
AdvisoryRef::operator==(const AdvisoryRef& other) const
{
    return advisory == other.advisory && index == other.index;
}

int AdvisoryRef::getIndex() const { return index; }
Id AdvisoryRef::getAdvisory() const { return advisory; }
DnfSack * AdvisoryRef::getDnfSack() const { return sack; }

}
