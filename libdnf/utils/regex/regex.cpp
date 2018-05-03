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

#include "regex.hpp"

#include <string.h>

Regex::Regex(const char * regex, int flags)
{
    auto errCode = regcomp(&exp, regex, flags);
    if (errCode != 0) {
        auto size = regerror(errCode, &exp, nullptr, 0);
        if (size) {
            std::string msg(size, '\0');
            regerror(errCode, &exp, &msg.front(), size);
            throw LibraryException(errCode, msg);
        }
        throw LibraryException(errCode, "");
    }
}

Regex &
Regex::operator=(Regex && src) noexcept
{
    free();
    freed = src.freed;
    exp = src.exp;
    src.freed = true;
    return *this;
}

Regex::Result
Regex::match(const char * str, bool copyStr, std::size_t count) const
{
    if (freed)
        throw InvalidException();
    auto maxMatches = exp.re_nsub + 1;
    auto size = count < maxMatches ? count : maxMatches;
    Result ret(str, copyStr, size);
    ret.matched = regexec(&exp, str, size, ret.matches.data(), 0) == 0;
    return ret;
}

Regex::Result::Result(const Regex::Result& src)
: sourceOwner(src.sourceOwner), matched(src.matched), matches(src.matches)
{
    if (sourceOwner) {
        auto tmp = new char[strlen(src.source) + 1];
        this->source = strcpy(tmp, src.source);
    } else
        this->source = src.source;
}

Regex::Result::Result(Regex::Result && src)
: source(src.source), sourceOwner(src.sourceOwner)
, matched(src.matched), matches(std::move(src.matches))
{
    src.source = nullptr;
    src.sourceOwner = false;
    src.matched = false;
}

Regex::Result::Result(const char * source, bool copySource, std::size_t count)
: sourceOwner(copySource), matched(false), matches(count)
{
    if (copySource) {
        auto tmp = new char[strlen(source) + 1];
        this->source = strcpy(tmp, source);
    } else
        this->source = source;
}

std::string
Regex::Result::getMatchedString(std::size_t index) const
{
    if (matched && index < matches.size() && matches[index].rm_so != NOT_FOUND) {
        auto subexpr_len = matches[index].rm_eo - matches[index].rm_so;
        if (subexpr_len > 0)
            return std::string(source + matches[index].rm_so, subexpr_len);
    }
    return "";
}
