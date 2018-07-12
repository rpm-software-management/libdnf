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

#ifndef _LOGGER_HPP_
#define _LOGGER_HPP_

#include <string>

#include <sys/types.h>
#include <time.h>
#include <unistd.h>

class Logger {
public:
    enum class Level {CRITICAL, ERROR, WARNING, NOTICE, INFO, DEBUG, TRACE};
    static constexpr const char * levelToCStr(Level level) noexcept { return level<Level::CRITICAL || level>Level::TRACE ? "USER" : levelCStr[static_cast<int>(level)]; }

    virtual void write(int source, Level level, const std::string & message);
    virtual void write(int source, time_t time, pid_t pid, Level level, const std::string & message) = 0;
    virtual ~Logger() = default;

private:
    static constexpr const char * levelCStr[]{"CRITICAL", "ERROR", "WARNING", "NOTICE", "INFO", "DEBUG", "TRACE"};
};

#endif // _LOGGER_HPP_
