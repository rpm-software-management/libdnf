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


#ifndef __ADVISORY_REF_HPP
#define __ADVISORY_REF_HPP

#include <memory>

#include <solv/pooltypes.h>

#include "../dnf-types.h"

namespace libdnf {

struct AdvisoryRef {
public:
    AdvisoryRef(DnfSack *sack, Id advisory, int index);

    bool operator ==(const AdvisoryRef & other) const;
    Id getAdvisory() const;
    int getIndex() const;
    DnfSack * getDnfSack() const;
private:
    DnfSack *sack;
    Id advisory;
    int index;
};

}

#endif /* __ADVISORY_REF_HPP */
