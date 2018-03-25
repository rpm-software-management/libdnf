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

#ifndef HY_NEVRA_HPP
#define HY_NEVRA_HPP

#include "dnf-types.h"

#include <memory>
#include <string>

namespace libdnf {

struct Nevra {
public:
    static constexpr int IgnoreEpoch = -1;

    Nevra();
    Nevra(const Nevra & src);
    Nevra(Nevra && src);

    ~Nevra();

    Nevra & operator=(const Nevra & src);
    Nevra & operator=(Nevra && src) noexcept;
    bool operator==(const Nevra & r) const;
    bool operator!=(const Nevra & r) const;

    void swap(Nevra & nevra) noexcept;
    void clear() noexcept;

    const std::string & getName() const noexcept;
    int getEpoch() const noexcept;
    const std::string & getVersion() const  noexcept;
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
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}

#endif // HY_NEVRA_HPP
