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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef __DNF_CONTEXT_HPP
#define __DNF_CONTEXT_HPP

#include "dnf-context.h"
#include "conf/ConfigMain.hpp"

#include <map>
#include <string>


inline DnfContextInvalidateFlags operator|(DnfContextInvalidateFlags a, DnfContextInvalidateFlags b)
{
    return static_cast<DnfContextInvalidateFlags>(static_cast<int>(a) | static_cast<int>(b));
}

namespace libdnf {

struct Setopt {
    Option::Priority priority;
    std::string key;
    std::string value;
};

std::map<std::string, std::string> & dnf_context_get_vars(DnfContext * context);
bool dnf_context_get_vars_cached(DnfContext * context);
void dnf_context_load_vars(DnfContext * context);
ConfigMain & getGlobalMainConfig(bool canReadConfigFile = true);
bool addSetopt(const char * key, Option::Priority priority, const char * value, GError ** error);
const std::vector<Setopt> & getGlobalSetopts();

}

#endif /* __DNF_CONTEXT_HPP */
