/*
 * Copyright (C) 2019 Red Hat, Inc.
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

#include "advisory.hpp"
#include "advisorymodule.hpp"
#include "../dnf-sack-private.hpp"

namespace libdnf {

class AdvisoryModule::Impl {
private:
    friend AdvisoryModule;
    DnfSack *sack;
    Id advisory;
    Id name;
    Id stream;
    Id version;
    Id context;
    Id arch;
};

AdvisoryModule::AdvisoryModule(DnfSack *sack, Id advisory, Id name, Id stream, Id version, Id context, Id arch) : pImpl(new Impl)
{
    pImpl->sack = sack;
    pImpl->advisory = advisory;
    pImpl->name = name;
    pImpl->stream = stream;
    pImpl->version = version;
    pImpl->context = context;
    pImpl->arch = arch;
}
AdvisoryModule::AdvisoryModule(const AdvisoryModule & src) : pImpl(new Impl) { *pImpl = *src.pImpl; }
AdvisoryModule::AdvisoryModule(AdvisoryModule && src) : pImpl(new Impl) { pImpl.swap(src.pImpl); }
AdvisoryModule::~AdvisoryModule() = default;

AdvisoryModule & AdvisoryModule::operator=(const AdvisoryModule & src) { *pImpl = *src.pImpl; return *this; }

AdvisoryModule &
AdvisoryModule::operator=(AdvisoryModule && src) noexcept
{
    pImpl.swap(src.pImpl);
    return *this;
}

bool
AdvisoryModule::nsvcaEQ(AdvisoryModule & other)
{
    return other.pImpl->name == pImpl->name &&
        other.pImpl->stream == pImpl->stream &&
        other.pImpl->version == pImpl->version &&
        other.pImpl->context == pImpl->context &&
        other.pImpl->arch == pImpl->arch;
}

bool
AdvisoryModule::isApplicable() const {
    auto moduleContainer = dnf_sack_get_module_container(pImpl->sack);
    if (!moduleContainer) {
        return true;
    }

    if (!moduleContainer->isEnabled(getName(), getStream())) {
        return false;
    }

    return true;
}

Advisory * AdvisoryModule::getAdvisory() const
{
    return new Advisory(pImpl->sack, pImpl->advisory);
}

const char *
AdvisoryModule::getName() const
{
    return pool_id2str(dnf_sack_get_pool(pImpl->sack), pImpl->name);
}

const char *
AdvisoryModule::getStream() const
{
    return pool_id2str(dnf_sack_get_pool(pImpl->sack), pImpl->stream);
}

const char *
AdvisoryModule::getVersion() const
{
    return pool_id2str(dnf_sack_get_pool(pImpl->sack), pImpl->version);
}

const char *
AdvisoryModule::getContext() const
{
    return pool_id2str(dnf_sack_get_pool(pImpl->sack), pImpl->context);
}

const char *
AdvisoryModule::getArch() const
{
    return pool_id2str(dnf_sack_get_pool(pImpl->sack), pImpl->arch);
}

DnfSack * AdvisoryModule::getSack() { return pImpl->sack; }

}
