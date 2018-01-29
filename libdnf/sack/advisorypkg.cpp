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

#include <string>

#include <solv/poolid.h>

#include "advisorypkg.hpp"


class AdvisoryPkg::Impl {
private:
    friend AdvisoryPkg;
    Id name;
    Id evr;
    Id arch;
    const char * filename;
    Pool *pool;
};

AdvisoryPkg::AdvisoryPkg(Pool *pool, Id name, Id evr, Id arch, const char * filename)
: pImpl(new Impl)
{
    pImpl->pool = pool;
    pImpl->name = name;
    pImpl->evr = evr;
    pImpl->arch = arch;
    pImpl->filename = filename;
}
AdvisoryPkg::AdvisoryPkg(const AdvisoryPkg & src) : pImpl(new Impl) { *pImpl = *src.pImpl; }
AdvisoryPkg::AdvisoryPkg(AdvisoryPkg && src) : pImpl(new Impl) { pImpl.swap(src.pImpl); }
AdvisoryPkg::~AdvisoryPkg() = default;

AdvisoryPkg & AdvisoryPkg::operator=(const AdvisoryPkg & src) { *pImpl = *src.pImpl; return *this; }
AdvisoryPkg & AdvisoryPkg::operator=(AdvisoryPkg && src) noexcept { *pImpl = std::move(*src.pImpl);
    return *this; }

bool
AdvisoryPkg::nevraEQ(AdvisoryPkg & other)
{
    return other.pImpl->name == pImpl->name &&
        other.pImpl->evr == pImpl->evr &&
        other.pImpl->arch == pImpl->arch;
}

bool
AdvisoryPkg::nevraEQ(Solvable *s)
{
    return s->name == pImpl->name && s->evr == pImpl->evr && s->arch == pImpl->arch;
}

Id AdvisoryPkg::getName() const { return pImpl->name; }
const char * AdvisoryPkg::getNameString() const { return pool_id2str(pImpl->pool, pImpl->name); }
Id AdvisoryPkg::getEVR() const { return pImpl->evr; }
const char * AdvisoryPkg::getEVRString() const { return pool_id2str(pImpl->pool, pImpl->evr); }
Id AdvisoryPkg::getArch() const { return pImpl->arch; }
const char * AdvisoryPkg::getArchString() const { return pool_id2str(pImpl->pool, pImpl->arch); }
const char * AdvisoryPkg::getFileName() const { return pImpl->filename; }
Pool * AdvisoryPkg::getPool() { return pImpl->pool; }
