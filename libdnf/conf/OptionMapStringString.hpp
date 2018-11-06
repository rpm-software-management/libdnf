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

#ifndef _LIBDNF_OPTION_MAP_STRING_STRING_HPP
#define _LIBDNF_OPTION_MAP_STRING_STRING_HPP

#include "Option.hpp"
#include <map>

namespace libdnf {

class OptionMapStringString : public Option {
public:
    typedef std::map<std::string, std::string> ValueType;

    OptionMapStringString(const ValueType & defaultValue);
    OptionMapStringString(const std::string & defaultValue);
    void test(const ValueType & value) const;
    ValueType fromString(const std::string & value) const;
    virtual void set(Priority priority, const ValueType & value);
    void set(Priority priority, const std::string & value) override;
    const ValueType & getValue() const;
    const ValueType & getDefaultValue() const;
    std::string toString(const ValueType & value) const;
    std::string getValueString() const override;

protected:
    ValueType defaultValue;
    ValueType value;
};

inline void OptionMapStringString::test(const ValueType & value) const {}

inline const OptionMapStringString::ValueType & OptionMapStringString::getValue() const
{
    return value;
}

inline const OptionMapStringString::ValueType & OptionMapStringString::getDefaultValue() const
{
    return defaultValue;
}

inline std::string OptionMapStringString::getValueString() const
{
    return toString(value); 
}

}

#endif
