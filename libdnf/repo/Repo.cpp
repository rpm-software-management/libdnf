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

#define ASCII_LOWERCASE "abcdefghijklmnopqrstuvwxyz"
#define ASCII_UPPERCASE "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define ASCII_LETTERS ASCII_LOWERCASE ASCII_UPPERCASE
#define DIGITS "0123456789"
#define REPOID_CHARS ASCII_LETTERS DIGITS "-_.:"

#include "Repo.hpp"

#include <solv/repo.h>

namespace libdnf {

class Repo::Impl {
public:
    Impl(const std::string & id, std::unique_ptr<ConfigRepo> && config);
    ~Impl();

    std::string id;
    std::unique_ptr<ConfigRepo> conf;
};

Repo::Impl::Impl(const std::string & id, std::unique_ptr<ConfigRepo> && config)
: id(id), conf(std::move(config)) {}

Repo::Impl::~Impl()
{
}

Repo::Repo(const std::string & id, std::unique_ptr<ConfigRepo> && config)
: pImpl(new Impl(id, std::move(config))) {}

Repo::~Repo() = default;

int Repo::verifyId(const std::string & repoId)
{
    auto idx = repoId.find_first_not_of(REPOID_CHARS);
    return idx == repoId.npos ? -1 : idx;
}

ConfigRepo * Repo::getConfig() noexcept
{
    return pImpl->conf.get();
}

const std::string & Repo::getId() const noexcept
{
    return pImpl->id;
}

void Repo::enable()
{
    pImpl->conf->enabled().set(Option::Priority::RUNTIME, true);
}

void Repo::disable()
{
    pImpl->conf->enabled().set(Option::Priority::RUNTIME, false);
}

}
