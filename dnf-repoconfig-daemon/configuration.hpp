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

#ifndef DNFDAEMON_CONFIGURATION_HPP
#define DNFDAEMON_CONFIGURATION_HPP

#include "../libdnf/conf/ConfigMain.hpp"
#include "../libdnf/conf/ConfigParser.hpp"
#include "../libdnf/conf/ConfigRepo.hpp"

#include <map>
#include <memory>
#include <vector>


class Configuration {
public:
    struct RepoInfo {
        std::string file_path;
        std::unique_ptr<libdnf::ConfigRepo> repoconfig;
    };

    Configuration() : cfg_main(new libdnf::ConfigMain) {};
    ~Configuration() = default;

    void read_configuration();
    const std::map<std::string, std::unique_ptr<RepoInfo>> & get_repos() {return repos;}
    RepoInfo* find_repo(const std::string &repoid);
    libdnf::ConfigParser* find_parser(const std::string &file_path);

private:
    std::unique_ptr<libdnf::ConfigMain> cfg_main;
    // repoid: repoinfo
    std::map<std::string, std::unique_ptr<RepoInfo>> repos;
    // repo_config_file_path: parser
    std::map<std::string, std::unique_ptr<libdnf::ConfigParser>> config_parsers;
    std::map<std::string, std::string> substitutions;

    void read_repos(const libdnf::ConfigParser *repo_parser, const std::string &file_path);
    void set_substitutions();
    void read_main_config();
    void read_repo_configs();
};


#endif
