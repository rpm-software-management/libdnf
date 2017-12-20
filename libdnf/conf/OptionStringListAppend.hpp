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

#ifndef _LIBDNF_OPTION_STRING_LIST_APPEND_HPP
#define _LIBDNF_OPTION_STRING_LIST_APPEND_HPP

#include "OptionStringList.hpp"

namespace libdnf {

class OptionStringListAppend : public OptionStringList {
public:
    OptionStringListAppend(const ValueType & defaultValue);
    OptionStringListAppend(const ValueType & defaultValue, const std::string & regex, bool icase);
    OptionStringListAppend(const std::string & defaultValue);
    OptionStringListAppend(const std::string & defaultValue, const std::string & regex, bool icase);
    using OptionStringList::set;
    void set(Priority priority, const ValueType & value) override;
    void reset(Priority priority, const ValueType & value);
};

}

#endif
