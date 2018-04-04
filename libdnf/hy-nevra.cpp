/*
 * Copyright (C) 2017 Red Hat, Inc.
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

#include "hy-nevra.hpp"
#include "dnf-sack.h"

namespace libdnf {

void
Nevra::clear() noexcept
{
    name.clear();
    epoch = EpochNotSet;
    version.clear();
    release.clear();
    arch.clear();
}

std::string
Nevra::getEvr() const
{
    if (epoch == EpochNotSet)
        return version + "-" + release;
    return std::to_string(epoch) + ":" + version + "-" + release;
}

bool
Nevra::hasJustName() const
{
    return !name.empty() && epoch == EpochNotSet && 
        version.empty() && release.empty() && arch.empty();
}

int
Nevra::compareEvr(const Nevra & nevra2, DnfSack *sack) const
{
    return dnf_sack_evr_cmp(sack, getEvr().c_str(), nevra2.getEvr().c_str());
}

int
Nevra::compare(const Nevra & nevra2) const
{
    auto ret = name.compare(nevra2.name);
    if (ret != 0)
        return ret;
    ret = compareEvr(nevra2, nullptr);
    if (ret != 0)
        return ret;
    return arch.compare(nevra2.arch);
}

}
