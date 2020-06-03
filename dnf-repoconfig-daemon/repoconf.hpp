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

#ifndef DNF_REPOCONFIG_DAEMON_REPOCONF_HPP
#define DNF_REPOCONFIG_DAEMON_REPOCONF_HPP

#include <sdbus-c++/sdbus-c++.h>
#include <vector>
#include <string>

const std::string REPO_CONF_ERROR = "org.rpm.dnf.v0.rpm.RepoConf.Error";

using KeyValueMap = std::map<std::string, sdbus::Variant>;
using KeyValueMapList = std::vector<KeyValueMap>;

class RepoConf {
public:
    RepoConf(sdbus::IConnection &connection);
    ~RepoConf() {
        dbus_object->unregister();
    }
    void list(sdbus::MethodCall call);
    void get(sdbus::MethodCall call);
    void enable_disable(sdbus::MethodCall call, const bool enable);

private:
    KeyValueMapList repo_list(const std::vector<std::string> &ids);
    std::vector<std::string> enable_disable_repos(const std::vector<std::string> &ids, const bool enable);
    void dbus_register_methods();
    bool check_authorization(const std::string &actionid, const std::string &sender);
    sdbus::IConnection &connection;
    std::unique_ptr<sdbus::IObject> dbus_object;
};

#endif
