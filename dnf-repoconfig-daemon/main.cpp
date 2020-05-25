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

#include "logger.hpp"
#include "repoconf.hpp"

#include "../libdnf/log.hpp"
#include "../libdnf/utils/tinyformat/tinyformat.hpp"

#include <sdbus-c++/sdbus-c++.h>
#include <string>


int main(int argc, char *argv[])
{
    JournalLogger journal_logger;
    libdnf::Log::setLogger(&journal_logger);

    // Create D-Bus connection to the system bus and requests name on it.
    const char* serviceName = "org.rpm.dnf.v1.rpm.RepoConf";
    std::unique_ptr<sdbus::IConnection> connection = NULL;
    try {
        connection = sdbus::createSystemBusConnection(serviceName);
    } catch (const sdbus::Error &e) {
        journal_logger.error(tfm::format("Fatal error: %s", e.what()));
        return 1;
    }

    auto repo_conf = RepoConf(*connection);

    // Run the I/O event loop on the bus connection.
    connection->enterEventLoop();
}
