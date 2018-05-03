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

#include "nevra.hpp"
#include "dnf-sack.h"

#include "regex/regex.hpp"

namespace libdnf {

#define PKG_NAME "([^:(/=<> ]+)"
#define PKG_EPOCH "(([0-9]+):)?"
#define PKG_VERSION "([^-:(/=<> ]+)"
#define PKG_RELEASE PKG_VERSION
#define PKG_ARCH "([^-:.(/=<> ]+)"

static const Regex NEVRA_FORM_REGEX[]{
    Regex("^" PKG_NAME "-" PKG_EPOCH PKG_VERSION "-" PKG_RELEASE "\\." PKG_ARCH "$", REG_EXTENDED),
    Regex("^" PKG_NAME "-" PKG_EPOCH PKG_VERSION "-" PKG_RELEASE          "()"  "$", REG_EXTENDED),
    Regex("^" PKG_NAME "-" PKG_EPOCH PKG_VERSION        "()"              "()"  "$", REG_EXTENDED),
    Regex("^" PKG_NAME      "()()"      "()"            "()"     "\\." PKG_ARCH "$", REG_EXTENDED),
    Regex("^" PKG_NAME      "()()"      "()"            "()"              "()"  "$", REG_EXTENDED)
};

bool Nevra::parse(const char * nevraStr, HyForm form)
{
    enum { NAME = 1, EPOCH = 3, VERSION = 4, RELEASE = 5, ARCH = 6, _LAST_ };
    auto matchResult = NEVRA_FORM_REGEX[form - 1].match(nevraStr, false, _LAST_);
    if (!matchResult.isMatched() || matchResult.getMatchedLen(NAME) == 0)
        return false;
    name = matchResult.getMatchedString(NAME);
    if (matchResult.getMatchedLen(EPOCH) > 0)
        epoch = atoi(matchResult.getMatchedString(EPOCH).c_str());
    else
        epoch = EPOCH_NOT_SET;
    version = matchResult.getMatchedString(VERSION);
    release = matchResult.getMatchedString(RELEASE);
    arch = matchResult.getMatchedString(ARCH);
    return true;
}

void
Nevra::clear() noexcept
{
    name.clear();
    epoch = EPOCH_NOT_SET;
    version.clear();
    release.clear();
    arch.clear();
}

std::string
Nevra::getEvr() const
{
    if (epoch == EPOCH_NOT_SET)
        return version + "-" + release;
    return std::to_string(epoch) + ":" + version + "-" + release;
}

bool
Nevra::hasJustName() const
{
    return !name.empty() && epoch == EPOCH_NOT_SET && 
        version.empty() && release.empty() && arch.empty();
}

int
Nevra::compareEvr(const Nevra & nevra2, DnfSack *sack) const
{
    return dnf_sack_evr_cmp(sack, getEvr().c_str(), nevra2.getEvr().c_str());
}

int
Nevra::compare(const Nevra & nevra2) const
{
    auto ret = name.compare(nevra2.name);
    if (ret != 0)
        return ret;
    ret = compareEvr(nevra2, nullptr);
    if (ret != 0)
        return ret;
    return arch.compare(nevra2.arch);
}

}
