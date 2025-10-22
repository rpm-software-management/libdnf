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
 * License along with this library; if not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <GLibLogger.hpp>

#include <glib.h>

namespace libdnf {

static inline void write_g_log(const std::string & domain, Logger::Level level, const std::string & message)
{
    GLogLevelFlags gLogLevel;
    switch (level) {
        // In GLib, the "G_LOG_LEVEL_ERROR" is more severe than "G_LOG_LEVEL_CRITICAL".
        // In fact, the "G_LOG_LEVEL_ERROR" is always fatal. We won't use it now.
        case Logger::Level::CRITICAL:
            gLogLevel = G_LOG_LEVEL_CRITICAL;
            break;
        case Logger::Level::ERROR:
            gLogLevel = G_LOG_LEVEL_CRITICAL;
            break;

        case Logger::Level::WARNING:
            gLogLevel = G_LOG_LEVEL_WARNING;
            break;
        case Logger::Level::NOTICE:
            gLogLevel = G_LOG_LEVEL_MESSAGE;
            break;
        case Logger::Level::INFO:
            gLogLevel = G_LOG_LEVEL_INFO;
            break;
        case Logger::Level::DEBUG:
        case Logger::Level::TRACE:
        default:
            gLogLevel = G_LOG_LEVEL_DEBUG;
            break;
    }
    g_log(domain.c_str(), gLogLevel, "%s", message.c_str());
}

void GLibLogger::write(int /*source*/, Level level, const std::string & message)
{
    write_g_log(domain, level, message);
}

void GLibLogger::write(int /*source*/, time_t /*time*/, pid_t /*pid*/, Level level, const std::string & message)
{
    write_g_log(domain, level, message);
}

}
