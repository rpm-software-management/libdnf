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

#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>
#include <iostream>
#include <unistd.h>

int main(int argc, char *argv[])
{
    auto connection = sdbus::createSystemBusConnection();
    std::cout << "Client name: " << connection->getUniqueName() << std::endl;
    const char* destinationName = "org.rpm.dnf.v1.rpm.RepoConf";
    const char* objectPath = "/org/rpm/dnf/v1/rpm/RepoConf";
    auto repoconf_proxy = sdbus::createProxy(*connection, destinationName, objectPath);
    const char* interfaceName = "org.rpm.dnf.v1.rpm.RepoConf";
    repoconf_proxy->finishRegistration();

    {
        std::vector<std::string> ids = {"fedora", "updates"};
        std::vector<std::map<std::string, sdbus::Variant>> repolist;
        repoconf_proxy->callMethod("list").onInterface(interfaceName).withArguments(ids).storeResultsTo(repolist);
        for (auto &repo: repolist) {
            for (auto &item: repo) {
                std::cout << item.first << ": " << item.second.get<std::string>() << std::endl;
            }
            std::cout << "-----------" << std::endl;
        }
    }

    return 0;
}
