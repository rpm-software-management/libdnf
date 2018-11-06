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

#include "OptionMapStringString.hpp"

#include "bgettext/bgettext-lib.h"

namespace libdnf {

OptionMapStringString::OptionMapStringString(const ValueType & defaultValue)
: Option(Priority::DEFAULT), defaultValue(defaultValue), value(defaultValue) {}

OptionMapStringString::OptionMapStringString(const std::string & defaultValue)
: Option(Priority::DEFAULT)
{
    this->value = this->defaultValue = fromString(defaultValue);
}

OptionMapStringString::ValueType OptionMapStringString::fromString(const std::string & value) const
{
    throw std::runtime_error(_("Not supported"));
}

void OptionMapStringString::set(Priority priority, const ValueType & value)
{
    if (priority >= this->priority) {
        test(value);
        this->value = value;
        this->priority = priority;
    }
}

void OptionMapStringString::set(Priority priority, const std::string & value)
{
    set(priority, fromString(value));
}

std::string OptionMapStringString::toString(const ValueType & value) const
{
    std::ostringstream oss;
    oss << "{";
    bool next{false};
    for (auto & val : value) {
        if (next)
            oss << ", ";
        else
            next = true;
        oss << '"' << val.first << '": "' << val.second << '"';
    }
    oss << "}";
    return oss.str();
}

}
