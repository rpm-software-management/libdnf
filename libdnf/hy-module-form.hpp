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

#ifndef LIBDNF_HY_MODULE_FORM_HPP
#define LIBDNF_HY_MODULE_FORM_HPP

#include "hy-subject.h"

#include <string>

namespace libdnf {

struct ModuleForm {
public:
    static constexpr long long VersionNotSet = -1;

    ModuleForm();

    bool parse(const char *moduleFormStr, HyModuleFormEnum form);
    void clear();

    const std::string & getName() const noexcept;
    const std::string & getStream() const noexcept;
    long long getVersion() const noexcept;
    const std::string & getContext() const noexcept;
    const std::string & getArch() const noexcept;
    const std::string & getProfile() const noexcept;

    void setName(const std::string & name);
    void setStream(const std::string & stream);
    void setVersion(long long version);
    void setContext(const std::string & context);
    void setArch(const std::string & arch);
    void setProfile(const std::string & profile);

    void setName(std::string && name);
    void setStream(std::string && stream);
    void setContext(std::string && context);
    void setArch(std::string && arch);
    void setProfile(std::string && profile);

private:
    std::string name;
    std::string stream;
    long long version;
    std::string context;
    std::string arch;
    std::string profile;
};

inline ModuleForm::ModuleForm()
: version(VersionNotSet) {}

inline const std::string & ModuleForm::getName() const noexcept
{
    return name;
}

inline const std::string & ModuleForm::getStream() const noexcept
{
    return stream;
}

inline long long ModuleForm::getVersion() const noexcept
{
    return version;
}

inline const std::string & ModuleForm::getContext() const noexcept
{
    return context;
}

inline const std::string & ModuleForm::getArch() const noexcept
{
    return arch;
}

inline const std::string & ModuleForm::getProfile() const noexcept
{
    return profile;
}

inline void ModuleForm::setName(const std::string & name)
{
    this->name = name;
}

inline void ModuleForm::setStream(const std::string & stream)
{
    this->stream = stream;
}

inline void ModuleForm::setVersion(long long version)
{
    this->version = version;
}

inline void ModuleForm::setContext(const std::string & context)
{
    this->context = context;
}

inline void ModuleForm::setArch(const std::string & arch)
{
    this->arch = arch;
}

inline void ModuleForm::setProfile(const std::string & profile)
{
    this->profile = profile;
}

inline void ModuleForm::setName(std::string && name)
{
    this->name = std::move(name);
}

inline void ModuleForm::setStream(std::string && stream)
{
    this->stream = std::move(stream);
}

inline void ModuleForm::setContext(std::string && context)
{
    this->context = std::move(context);
}

inline void ModuleForm::setArch(std::string && arch)
{
    this->arch = std::move(arch);
}

inline void ModuleForm::setProfile(std::string && profile)
{
    this->profile = std::move(profile);
}

}

#endif //LIBDNF_HY_MODULE_FORM_HPP
