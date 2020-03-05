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
    static constexpr int EPOCH_NOT_SET = -1;

    Nevra();

    bool parse(const char * nevraStr, HyForm form);
    void clear() noexcept;

    const std::string & getName() const noexcept;
    int getEpoch() const noexcept;
    const std::string & getVersion() const noexcept;
    const std::string & getRelease() const noexcept;
    const std::string & getArch() const noexcept;

    void setName(const std::string & name);
    void setEpoch(int epoch);
    void setVersion(const std::string & version);
    void setRelease(const std::string & release);
    void setArch(const std::string & arch);

    void setName(std::string && name);
    void setVersion(std::string && version);
    void setRelease(std::string && release);
    void setArch(std::string && arch);

    std::string getEvr() const;
    bool hasJustName() const;
    int compareEvr(const Nevra & nevra2, DnfSack *sack) const;
    int compare(const Nevra & nevra2) const;

private:
    std::string name;
    int epoch;
    std::string version;
    std::string release;
    std::string arch;
};

inline Nevra::Nevra()
: epoch(EPOCH_NOT_SET) {}

inline const std::string & Nevra::getName() const noexcept
{
    return name;
}

inline int Nevra::getEpoch() const noexcept
{
    return epoch;
}

inline const std::string & Nevra::getVersion() const noexcept
{
    return version;
}

inline const std::string & Nevra::getRelease() const noexcept
{
    return release;
}

inline const std::string & Nevra::getArch() const noexcept {
    return arch;
}

inline void Nevra::setName(const std::string & name)
{
    this->name = name;
}

inline void Nevra::setEpoch(int epoch)
{
    this->epoch = epoch;
}

inline void Nevra::setVersion(const std::string & version)
{
    this->version = version;
}

inline void Nevra::setRelease(const std::string & release)
{
    this->release = release;
}

inline void Nevra::setArch(const std::string & arch)
{
    this->arch = arch;
}

inline void Nevra::setName(std::string && name)
{
    this->name = std::move(name);
}

inline void Nevra::setVersion(std::string && version)
{
    this->version = std::move(version);
}

inline void Nevra::setRelease(std::string && release)
{
    this->release = std::move(release);
}

inline void Nevra::setArch(std::string && arch)
{
    this->arch = std::move(arch);
}

}

#endif // LIBDNF_NEVRA_HPP
