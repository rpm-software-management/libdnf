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

#include "OptionStringListAppend.hpp"

namespace libdnf {

OptionStringListAppend::OptionStringListAppend(const ValueType & defaultValue)
: OptionStringList(defaultValue) {}

OptionStringListAppend::OptionStringListAppend(const ValueType & defaultValue, const std::string & regex, bool icase)
: OptionStringList(defaultValue, regex, icase) {}

OptionStringListAppend::OptionStringListAppend(const std::string & defaultValue)
: OptionStringList(defaultValue) {}

OptionStringListAppend::OptionStringListAppend(const std::string & defaultValue, const std::string & regex, bool icase)
: OptionStringList(defaultValue, regex, icase) {}

void OptionStringListAppend::set(Priority priority, const ValueType & value)
{
    if (value.empty()) {
        if (priority >= this->priority) {
            this->value.clear();
            this->priority = priority;
        }
    } else {
        test(value);
        this->value.insert(this->value.end(), value.begin(), value.end());
        if (priority >= this->priority)
            this->priority = priority;
    }
}

void OptionStringListAppend::reset(Priority priority, const ValueType & value)
{
    OptionStringList::set(priority, value);
}

}
