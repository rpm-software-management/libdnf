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


#ifndef __CHANGELOG_HPP
#define __CHANGELOG_HPP

#include <ctime>
#include <string>

namespace libdnf {

struct Changelog {
private:
    time_t timestamp;
    std::string author;
    std::string text;
public:
    Changelog(time_t timestamp, std::string && author, std::string && text):
        timestamp(timestamp), author(std::move(author)), text(std::move(text)) {}
    time_t getTimestamp() const { return timestamp; };
    const std::string & getAuthor() const { return author; };
    const std::string & getText() const { return text; };
};

}

#endif /* __CHANGELOG_HPP */
