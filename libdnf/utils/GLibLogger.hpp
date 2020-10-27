/*
 * Copyright (C) 2020 Red Hat, Inc.
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

#ifndef _GLIB_LOGGER_HPP_
#define _GLIB_LOGGER_HPP_

#include <logger.hpp>

namespace libdnf {

class GLibLogger : public Logger {
public:
    explicit GLibLogger(std::string domain) : domain(domain) {}
    void write(int source, Level level, const std::string & message) override;
    void write(int source, time_t time, pid_t pid, Level level, const std::string & message) override;

private:
    std::string domain;
};

}

#endif // _GLIB_LOGGER_HPP_
