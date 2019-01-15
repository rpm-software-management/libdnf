/*
 * Copyright (C) 2017-2018 Red Hat, Inc.
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

#ifndef LIBDNF_NSVCAP_HPP
#define LIBDNF_NSVCAP_HPP

#include "hy-subject.h"

#include <string>

namespace libdnf {

struct Nsvcap {
public:
    bool parse(const char *nsvcapStr, HyModuleForm form);
    void clear();

    const std::string & getName() const noexcept;
    const std::string & getStream() const noexcept;
    const std::string & getVersion() const noexcept;
    const std::string & getContext() const noexcept;
    const std::string & getArch() const noexcept;
    const std::string & getProfile() const noexcept;

    void setName(const std::string & name);
    void setStream(const std::string & stream);
    void setVersion(const std::string & stream);
    void setContext(const std::string & context);
    void setArch(const std::string & arch);
    void setProfile(const std::string & profile);

    void setName(std::string && name);
    void setStream(std::string && stream);
    void setVersion(std::string && stream);
    void setContext(std::string && context);
    void setArch(std::string && arch);
    void setProfile(std::string && profile);

private:
    std::string name;
    std::string stream;
    std::string version;
    std::string context;
    std::string arch;
    std::string profile;
};

inline const std::string & Nsvcap::getName() const noexcept
{
    return name;
}

inline const std::string & Nsvcap::getStream() const noexcept
{
    return stream;
}

inline const std::string & Nsvcap::getVersion() const noexcept
{
    return version;
}

inline const std::string & Nsvcap::getContext() const noexcept
{
    return context;
}

inline const std::string & Nsvcap::getArch() const noexcept
{
    return arch;
}

inline const std::string & Nsvcap::getProfile() const noexcept
{
    return profile;
}

inline void Nsvcap::setName(const std::string & name)
{
    this->name = name;
}

inline void Nsvcap::setStream(const std::string & stream)
{
    this->stream = stream;
}

inline void Nsvcap::setVersion(const std::string & version)
{
    this->version = version;
}

inline void Nsvcap::setContext(const std::string & context)
{
    this->context = context;
}

inline void Nsvcap::setArch(const std::string & arch)
{
    this->arch = arch;
}

inline void Nsvcap::setProfile(const std::string & profile)
{
    this->profile = profile;
}

inline void Nsvcap::setName(std::string && name)
{
    this->name = std::move(name);
}

inline void Nsvcap::setStream(std::string && stream)
{
    this->stream = std::move(stream);
}

inline void Nsvcap::setVersion(std::string && version)
{
    this->version = std::move(version);
}

inline void Nsvcap::setContext(std::string && context)
{
    this->context = std::move(context);
}

inline void Nsvcap::setArch(std::string && arch)
{
    this->arch = std::move(arch);
}

inline void Nsvcap::setProfile(std::string && profile)
{
    this->profile = std::move(profile);
}

}

#endif //LIBDNF_NSVCAP_HPP
