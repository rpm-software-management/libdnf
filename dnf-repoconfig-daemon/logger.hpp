/*
 * Copyright (C) 2020 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef DNF_REPOCONFIG_DAEMON_LOGGER_HPP
#define DNF_REPOCONFIG_DAEMON_LOGGER_HPP

#include "../libdnf/utils/logger.hpp"

class JournalLogger : public libdnf::Logger {
public:
    void write(int, Level, const std::string &) override;
    void write(int, time_t, pid_t, Level, const std::string &) override {}
};

#endif
