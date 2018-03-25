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

class Nevra::Impl {
public:
    std::string name;
    int epoch{IgnoreEpoch};
    std::string version;
    std::string release;
    std::string arch;
};

Nevra::Nevra() : pImpl(new Impl) {}
Nevra::Nevra(const Nevra & src) : pImpl(new Impl(*src.pImpl)) {}
Nevra::Nevra(Nevra && src) : pImpl(new Impl) { pImpl.swap(src.pImpl); }

Nevra::~Nevra() = default;

Nevra & Nevra::operator=(const Nevra & src) { *pImpl = *src.pImpl; return *this; }
Nevra & Nevra::operator=(Nevra && src) noexcept { *pImpl = std::move(*src.pImpl); return *this; }

void Nevra::swap(Nevra & nevra) noexcept { pImpl.swap(nevra.pImpl); }

const std::string & Nevra::getName() const noexcept { return pImpl->name; }
int Nevra::getEpoch() const noexcept { return pImpl->epoch; }
const std::string & Nevra::getVersion() const noexcept { return pImpl->version; }
const std::string & Nevra::getRelease() const noexcept { return pImpl->release; }
const std::string & Nevra::getArch() const noexcept { return pImpl->arch; }

void Nevra::setName(const std::string & name) { pImpl->name = name; }
void Nevra::setEpoch(int epoch) { pImpl->epoch = epoch; }
void Nevra::setVersion(const std::string & version) { pImpl->version = version; }
void Nevra::setRelease(const std::string & release) { pImpl->release = release; }
void Nevra::setArch(const std::string & arch) { pImpl->arch = arch; }

void Nevra::setName(std::string && name) { pImpl->name = std::move(name); }
void Nevra::setVersion(std::string && version) { pImpl->version = std::move(version); }
void Nevra::setRelease(std::string && release) { pImpl->release = std::move(release); }
void Nevra::setArch(std::string && arch) { pImpl->arch = std::move(arch); }

bool Nevra::operator!=(const Nevra & r) const { return !(*this == r); }

bool Nevra::operator==(const Nevra & r) const
{
    return pImpl->name == r.pImpl->name && pImpl->epoch == r.pImpl->epoch &&
        pImpl->version==r.pImpl->version && pImpl->release ==r.pImpl->release &&
        pImpl->arch==r.pImpl->arch;
}

void Nevra::clear() noexcept
{
    pImpl->name.clear();
    pImpl->epoch = IgnoreEpoch;
    pImpl->version.clear();
    pImpl->release.clear();
    pImpl->arch.clear();
}

bool Nevra::hasJustName() const
{
    return !pImpl->name.empty() && pImpl->epoch == IgnoreEpoch && 
        pImpl->version.empty() && pImpl->release.empty() && pImpl->arch.empty();
}

std::string Nevra::getEvr() const
{
    if (pImpl->epoch == IgnoreEpoch)
        return pImpl->version + "-" + pImpl->release;
    return std::to_string(pImpl->epoch) + ":" + pImpl->version + "-" + pImpl->release;
}

int Nevra::compareEvr(const Nevra & nevra2, DnfSack *sack) const
{
    return dnf_sack_evr_cmp(sack, getEvr().c_str(), nevra2.getEvr().c_str());
}

int Nevra::compare(const Nevra & nevra2) const
{
    auto ret = pImpl->name.compare(nevra2.pImpl->name);
    if (ret != 0)
        return ret;
    ret = compareEvr(nevra2, nullptr);
    if (ret != 0)
        return ret;
    return pImpl->arch.compare(nevra2.pImpl->arch);
}

}
