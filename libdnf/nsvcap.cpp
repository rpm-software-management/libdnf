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

#include "nsvcap.hpp"

#include <regex/regex.hpp>

namespace libdnf {

#define MODULE_NAME "([-a-zA-Z0-9\\._]+)"
#define MODULE_STREAM MODULE_NAME
#define MODULE_VERSION "([0-9]+)"
#define MODULE_CONTEXT "([0-9a-f]+)"
#define MODULE_ARCH MODULE_NAME
#define MODULE_PROFILE MODULE_NAME

static constexpr const char * NSVCAP_FORM_REGEX[] = {
    "^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION ":" MODULE_CONTEXT "::?" MODULE_ARCH "\\/"  MODULE_PROFILE "$",
    "^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION ":" MODULE_CONTEXT "::?" MODULE_ARCH "\\/?" "()"           "$",
    "^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION     "()"           "::"  MODULE_ARCH "\\/"  MODULE_PROFILE "$",
    "^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION     "()"           "::"  MODULE_ARCH "\\/?" "()"           "$",
    "^" MODULE_NAME ":" MODULE_STREAM     "()"               "()"           "::"  MODULE_ARCH "\\/"  MODULE_PROFILE "$",
    "^" MODULE_NAME ":" MODULE_STREAM     "()"               "()"           "::"  MODULE_ARCH "\\/?" "()"           "$",
    "^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION ":" MODULE_CONTEXT       "()"        "\\/"  MODULE_PROFILE "$",
    "^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION     "()"                 "()"        "\\/"  MODULE_PROFILE "$",
    "^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION ":" MODULE_CONTEXT       "()"        "\\/?" "()"           "$",
    "^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION     "()"                 "()"        "\\/?" "()"           "$",
    "^" MODULE_NAME ":" MODULE_STREAM     "()"               "()"                 "()"        "\\/"  MODULE_PROFILE "$",
    "^" MODULE_NAME ":" MODULE_STREAM     "()"               "()"                 "()"        "\\/?" "()"           "$",
    "^" MODULE_NAME     "()"              "()"               "()"           "::"  MODULE_ARCH "\\/"  MODULE_PROFILE "$",
    "^" MODULE_NAME     "()"              "()"               "()"           "::"  MODULE_ARCH "\\/?" "()"           "$",
    "^" MODULE_NAME     "()"              "()"               "()"                 "()"        "\\/"  MODULE_PROFILE "$",
    "^" MODULE_NAME     "()"              "()"               "()"                 "()"        "\\/?" "()"           "$"
};

bool Nsvcap::parse(const char *nsvcapStr, HyModuleForm form)
{
    enum { NAME = 1, STREAM = 2, VERSION = 3, CONTEXT = 4, ARCH = 5, PROFILE = 6, _LAST_ };
    Regex reg(NSVCAP_FORM_REGEX[form - 1], REG_EXTENDED);
    auto matchResult = reg.match(nsvcapStr, false, _LAST_);
    if (!matchResult.isMatched() || matchResult.getMatchedLen(NAME) == 0)
        return false;
    name = matchResult.getMatchedString(NAME);
    if (matchResult.getMatchedLen(VERSION) > 0)
        version = atoll(matchResult.getMatchedString(VERSION).c_str());
    else
        version = VERSION_NOT_SET;
    stream = matchResult.getMatchedString(STREAM);
    context = matchResult.getMatchedString(CONTEXT);
    arch = matchResult.getMatchedString(ARCH);
    profile = matchResult.getMatchedString(PROFILE);
    return true;
}

void
Nsvcap::clear()
{
    name.clear();
    stream.clear();
    version = VERSION_NOT_SET;
    context.clear();
    arch.clear();
    profile.clear();
}

}
