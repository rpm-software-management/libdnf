/*
Copyright (C) 2020 Red Hat, Inc.

This file is part of microdnf: https://github.com/rpm-software-management/libdnf/

Microdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Microdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with microdnf.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "utils.hpp"

#include <libdnf/base/base.hpp>
#include <libdnf/logger/memory_buffer_logger.hpp>
#include <libdnf/logger/stream_logger.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>

static bool userconfirm(libdnf::ConfigMain & config) {
    std::string msg;
    if (config.defaultyes().get_value()) {
        msg = "Is this ok [Y/n]: ";
    } else {
        msg = "Is this ok [y/N]: ";
    }
    while (true) {
        std::cout << msg;

        std::string choice;
        std::getline(std::cin, choice);

        if (choice.empty()) {
            return config.defaultyes().get_value();
        }
        if (choice == "y" || choice == "Y") {
            return true;
        }
        if (choice == "n" || choice == "N") {
            return false;
        }
    }
}

class DNFPRepoCB : public libdnf::rpm::RepoCB {
public:
    explicit DNFPRepoCB(libdnf::ConfigMain & config)
        : config(&config)  {}

    void start(const char *what) override
    {
        std::cout << "Start downloading: \"" << what << "\"" << std::endl;
    }

    void end() override
    {
        std::cout << "End downloading" << std::endl;
    }

    int progress(double /*totalToDownload*/, double /*downloaded*/) override
    {
        //std::cout << "Downloaded " << downloaded << "/" << totalToDownload << std::endl;
        return 0;
    }

    bool repokey_import(
        const std::string & id,
        const std::string & user_id,
        const std::string & fingerprint,
        const std::string & url,
        long int /*timestamp*/) override {

        auto tmp_id = id.size() > 8 ? id.substr(id.size() - 8) : id;
        std::cout << "Importing GPG key 0x" << id << ":\n";
        std::cout << " Userid     : \"" << user_id << "\"\n";
        std::cout << " Fingerprint: " << fingerprint << "\n";
        std::cout << " From       : " << url << std::endl;

        if (config->assumeyes().get_value()) {
            return true;
        }
        if (config->assumeno().get_value()) {
            return false;
        }
        return userconfirm(*config);
    }

private:
    libdnf::ConfigMain * config;
};

int main() {
    libdnf::Base base;

    auto logger = base.get_logger();

    // Add circular memory buffer logger
    const std::size_t max_log_items_to_keep = 10000;
    const std::size_t prealloc_log_items = 256;
    logger->add_logger(std::make_unique<libdnf::MemoryBufferLogger>(max_log_items_to_keep, prealloc_log_items));

    logger->info("Microdnf start");

    // load configuration from file and from directory with drop-in files
    base.load_config_from_file();
    base.load_config_from_dir();

    // determine cache and log paths
    auto config = base.get_config();
    if (microdnf::am_i_root()) {
        config->cachedir().set(libdnf::Option::Priority::DEFAULT, "/var/cache/dnf");
        config->logdir().set(libdnf::Option::Priority::DEFAULT, "/var/log");
    } else {
        auto cachedir = microdnf::get_cache_dir();
        config->cachedir().set(libdnf::Option::Priority::DEFAULT, cachedir);
        config->logdir().set(libdnf::Option::Priority::DEFAULT, cachedir);
    }

    // swap to destination logger and write messages from memory buffer logger to it
    auto log_stream = std::make_unique<std::ofstream>(config->logdir().get_value() + "/microdnf5.log", std::ios::app);
    std::unique_ptr<libdnf::Logger> stream_logger = std::make_unique<libdnf::StreamLogger>(std::move(log_stream));
    logger->swap_logger(stream_logger, 0); 
    dynamic_cast<libdnf::MemoryBufferLogger &>(*stream_logger).write_to_logger(logger);

    // detect values of basic variables (arch, basearch, and releasever) for substitutions
    auto arch = microdnf::detect_arch();
    auto & substitutions = *base.get_substitutions();
    substitutions["arch"] = arch;
    substitutions["basearch"] = microdnf::get_base_arch(arch.c_str());
    substitutions["releasever"] = microdnf::detect_release(config->installroot().get_value());

    // load additional variables from environment and directories
    libdnf::ConfigMain::add_vars_from_env(substitutions);
    for (auto & dir : config->varsdir().get_value()) {
        libdnf::ConfigMain::add_vars_from_dir(substitutions, dir);
    }

    // create rpm repositories according configuration files
    auto rpm_repo_sack = base.get_rpm_repo_sack();
    rpm_repo_sack->new_repos_from_file();
    rpm_repo_sack->new_repos_from_dirs();

    auto query = rpm_repo_sack->new_query();
    for (auto & repo : query.ifilter_enabled(true).get_data()) {
        repo->set_substitutions(substitutions);
        repo->set_callbacks(std::make_unique<DNFPRepoCB>(*config));
        try {
            repo->load();
        } catch (const std::runtime_error & ex) {
            logger->warning(ex.what());
            std::cout << ex.what() << std::endl;
        }
    }

    //TODO(jrohel): only test/demo code will gone out
    using SackQueryCmp = libdnf::sack::QueryCmp;

    // version 1
    {
        using RpmRepoFilter = libdnf::rpm::RepoQuery::F;
        query.ifilter(RpmRepoFilter::enabled, SackQueryCmp::EQ, true);
        query.ifilter(RpmRepoFilter::id, SackQueryCmp::REGEX, "fedora.*");
        std::cout << "Found repos:" << std::endl;
        auto repos = query.get_data();
        for (auto & repo : repos) {
            std::cout << repo->get_id() << std::endl;
        }
    }

    //version 2
    {
        query.ifilter_enabled(true).ifilter_id(SackQueryCmp::REGEX, "fedora.*");
        std::cout << "\nFound repos:" << std::endl;
        auto repos = query.get_data();
        for (auto & repo : repos) {
            std::cout << repo->get_id() << std::endl;
        }
    }

    logger->info("Microdnf end");

    return 0;
}
