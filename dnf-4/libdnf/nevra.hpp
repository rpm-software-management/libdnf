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

#ifndef LIBDNF_NEVRA_HPP
#define LIBDNF_NEVRA_HPP

#include "dnf-types.h"
#include "hy-subject.h"

#include <string>
#include <utility>

namespace libdnf {

struct Nevra {
public:

    std::string getEvr() const;
    int compareEvr(const Nevra & nevra2, DnfSack *sack) const;
    int compare(const Nevra & nevra2) const;

private:
    std::string name;
    int epoch;
    std::string version;
    std::string release;
    std::string arch;
};

}

#endif // LIBDNF_NEVRA_HPP
