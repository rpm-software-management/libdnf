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


#ifndef __ADVISORY_PKG_HPP
#define __ADVISORY_PKG_HPP

#include <memory>

#include <solv/pooltypes.h>
#include <solv/solvable.h>

#include "../dnf-types.h"
#include "advisory.hpp"

namespace libdnf {

struct AdvisoryPkg {
public:
    AdvisoryPkg(DnfSack *sack, Id advisory, Id name, Id evr, Id arch, const char * filename);
    AdvisoryPkg(const AdvisoryPkg & src);
    AdvisoryPkg(AdvisoryPkg && src);
    ~AdvisoryPkg();
    AdvisoryPkg & operator=(const AdvisoryPkg & src);
    AdvisoryPkg & operator=(AdvisoryPkg && src) noexcept;
    bool nevraEQ(AdvisoryPkg & other);
    bool nevraEQ(Solvable *s);
    Advisory * getAdvisory() const;
    Id getName() const;
    const char * getNameString() const;
    Id getEVR() const;
    const char * getEVRString() const;
    Id getArch() const;
    const char * getArchString() const;
    const char * getFileName() const;
    DnfSack * getSack();
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}

#endif /* __ADVISORY_PKG_HPP */
