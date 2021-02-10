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

#define HEXADECIMAL DIGITS "abcdef"
#define MODULE_SPECIAL "+._-"
#define GLOB "][*?!"

#include "nsvcap.hpp"

#include "libdnf/utils/utils.hpp"

#include <regex/regex.hpp>

namespace libdnf {

#define MODULE_NAME "([" GLOB ASCII_LETTERS DIGITS MODULE_SPECIAL "]+)"
#define MODULE_STREAM MODULE_NAME
#define MODULE_VERSION "([" GLOB DIGITS "-]+)"
#define MODULE_CONTEXT MODULE_NAME
#define MODULE_ARCH MODULE_NAME
#define MODULE_PROFILE MODULE_NAME

static const Regex NSVCAP_FORM_REGEX[]{
    Regex("^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION ":" MODULE_CONTEXT "::?" MODULE_ARCH "\\/"  MODULE_PROFILE "$", REG_EXTENDED),
    Regex("^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION ":" MODULE_CONTEXT "::?" MODULE_ARCH "\\/?" "()"           "$", REG_EXTENDED),
    Regex("^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION     "()"           "::"  MODULE_ARCH "\\/"  MODULE_PROFILE "$", REG_EXTENDED),
    Regex("^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION     "()"           "::"  MODULE_ARCH "\\/?" "()"           "$", REG_EXTENDED),
    Regex("^" MODULE_NAME ":" MODULE_STREAM     "()"               "()"           "::"  MODULE_ARCH "\\/"  MODULE_PROFILE "$", REG_EXTENDED),
    Regex("^" MODULE_NAME ":" MODULE_STREAM     "()"               "()"           "::"  MODULE_ARCH "\\/?" "()"           "$", REG_EXTENDED),
    Regex("^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION ":" MODULE_CONTEXT       "()"        "\\/"  MODULE_PROFILE "$", REG_EXTENDED),
    Regex("^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION     "()"                 "()"        "\\/"  MODULE_PROFILE "$", REG_EXTENDED),
    Regex("^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION ":" MODULE_CONTEXT       "()"        "\\/?" "()"           "$", REG_EXTENDED),
    Regex("^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION     "()"                 "()"        "\\/?" "()"           "$", REG_EXTENDED),
    Regex("^" MODULE_NAME ":" MODULE_STREAM     "()"               "()"                 "()"        "\\/"  MODULE_PROFILE "$", REG_EXTENDED),
    Regex("^" MODULE_NAME ":" MODULE_STREAM     "()"               "()"                 "()"        "\\/?" "()"           "$", REG_EXTENDED),
    Regex("^" MODULE_NAME     "()"              "()"               "()"           "::"  MODULE_ARCH "\\/"  MODULE_PROFILE "$", REG_EXTENDED),
    Regex("^" MODULE_NAME     "()"              "()"               "()"           "::"  MODULE_ARCH "\\/?" "()"           "$", REG_EXTENDED),
    Regex("^" MODULE_NAME     "()"              "()"               "()"                 "()"        "\\/"  MODULE_PROFILE "$", REG_EXTENDED),
    Regex("^" MODULE_NAME     "()"              "()"               "()"                 "()"        "\\/?" "()"           "$", REG_EXTENDED)
};

bool Nsvcap::parse(const char *nsvcapStr, HyModuleForm form)
{
    enum { NAME = 1, STREAM = 2, VERSION = 3, CONTEXT = 4, ARCH = 5, PROFILE = 6, _LAST_ };
    auto matchResult = NSVCAP_FORM_REGEX[form - 1].match(nsvcapStr, false, _LAST_);
    if (!matchResult.isMatched() || matchResult.getMatchedLen(NAME) == 0)
        return false;
    name = matchResult.getMatchedString(NAME);
    version = matchResult.getMatchedString(VERSION);
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
    version.clear();
    context.clear();
    arch.clear();
    profile.clear();
}

}
