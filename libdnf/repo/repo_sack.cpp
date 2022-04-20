/*
Copyright Contributors to the libdnf project.

This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

Libdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

Libdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with libdnf.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "libdnf/repo/repo_sack.hpp"

#include "rpm/package_sack_impl.hpp"
#include "utils/bgettext/bgettext-lib.h"

#include "libdnf/base/base.hpp"
#include "libdnf/common/exception.hpp"
#include "libdnf/conf/config_parser.hpp"
#include "libdnf/conf/option_bool.hpp"

extern "C" {
#include <solv/testcase.h>
}

#include <atomic>
#include <cerrno>
#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <thread>


using LibsolvRepo = ::Repo;

namespace {

// Names of special repositories
constexpr const char * SYSTEM_REPO_NAME = "@System";
constexpr const char * CMDLINE_REPO_NAME = "@commandline";
// TODO lukash: unused, remove?
//constexpr const char * MODULE_FAIL_SAFE_REPO_NAME = "@modulefailsafe";

}  // namespace

namespace libdnf::repo {

RepoSack::RepoSack(libdnf::Base & base) : RepoSack(base.get_weak_ptr()) {}


RepoWeakPtr RepoSack::create_repo(const std::string & id) {
    // TODO(jrohel): Test repo exists
    auto repo = std::make_unique<Repo>(base, id, Repo::Type::AVAILABLE);
    return add_item_with_return(std::move(repo));
}


RepoWeakPtr RepoSack::create_repo_from_libsolv_testcase(const std::string & id, const std::string & path) {
    auto repo = create_repo(id);
    repo->add_libsolv_testcase(path);
    return repo;
}


RepoWeakPtr RepoSack::get_cmdline_repo() {
    if (!cmdline_repo) {
        std::unique_ptr<Repo> repo(new Repo(base, CMDLINE_REPO_NAME, Repo::Type::COMMANDLINE));
        repo->get_config().build_cache().set(libdnf::Option::Priority::RUNTIME, false);
        cmdline_repo = repo.get();
        add_item(std::move(repo));
    }

    return cmdline_repo->get_weak_ptr();
}


RepoWeakPtr RepoSack::get_system_repo() {
    if (!system_repo) {
        std::unique_ptr<Repo> repo(new Repo(base, SYSTEM_REPO_NAME, Repo::Type::SYSTEM));
        // TODO(mblaha): re-enable caching once we can reliably detect whether system repo is up-to-date
        repo->get_config().build_cache().set(libdnf::Option::Priority::RUNTIME, false);
        system_repo = repo.get();
        add_item(std::move(repo));
    }

    return system_repo->get_weak_ptr();
}


void RepoSack::update_and_load_repos(libdnf::repo::RepoQuery & repos, Repo::LoadFlags flags) {
    std::atomic<bool> except_in_main_thread{false};  // set to true if an exception occurred in the main thread
    std::exception_ptr except_ptr;                   // for pass exception from thread_sack_loader to main thread,
                                                     // a default-constructed std::exception_ptr is a null pointer

    std::vector<libdnf::repo::Repo *> prepared_repos;  // array of repositories prepared to load into solv sack
    std::mutex prepared_repos_mutex;                   // mutex for the array
    std::condition_variable signal_prepared_repo;      // signals that next item is added into array
    std::size_t num_repos_loaded{0};                   // number of repositories already loaded into solv sack

    prepared_repos.reserve(repos.size() + 1);  // optimization: preallocate memory to avoid realocations, +1 stop tag

    // This thread loads prepared repositories into solvable sack
    std::thread thread_sack_loader([&]() {
        try {
            while (true) {
                std::unique_lock<std::mutex> lock(prepared_repos_mutex);
                while (prepared_repos.size() <= num_repos_loaded) {
                    signal_prepared_repo.wait(lock);
                }
                auto repo = prepared_repos[num_repos_loaded];
                lock.unlock();

                if (!repo || except_in_main_thread) {
                    break;  // nullptr mark - work is done, or exception in main thread
                }

                repo->load(flags);
                ++num_repos_loaded;
            }
        } catch (std::runtime_error & ex) {
            // The thread must not throw exceptions. Pass them to the main thread using exception_ptr.
            except_ptr = std::current_exception();
        }
    });

    // Adds information that all repos are updated (nullptr tag) and is waiting for thread_sack_loader to complete.
    auto finish_sack_loader = [&]() {
        {
            std::lock_guard<std::mutex> lock(prepared_repos_mutex);
            prepared_repos.push_back(nullptr);
        }
        signal_prepared_repo.notify_one();

        thread_sack_loader.join();  // waits for the thread_sack_loader to finish its execution
    };

    auto catch_thread_sack_loader_exceptions = [&]() {
        if (except_ptr) {
            if (thread_sack_loader.joinable()) {
                thread_sack_loader.join();
            }

            std::rethrow_exception(except_ptr);
        }
    };

    // If the input RepoQuery contains a system repo, we load the system repo first.
    for (auto & repo : repos) {
        if (repo->get_type() == libdnf::repo::Repo::Type::SYSTEM) {
            {
                std::lock_guard<std::mutex> lock(prepared_repos_mutex);
                prepared_repos.push_back(repo.get());
            }

            signal_prepared_repo.notify_one();
            break;
        }
    }

    // Prepares available repositories metadata for thread sack loader.
    for (auto & repo : repos) {
        if (repo->get_type() != libdnf::repo::Repo::Type::AVAILABLE) {
            continue;
        }
        if (any(flags & libdnf::repo::Repo::LoadFlags::OTHER)) {
            repo->set_load_metadata_other(true);
        }
        catch_thread_sack_loader_exceptions();
        try {
            repo->fetch_metadata();

            {
                std::lock_guard<std::mutex> lock(prepared_repos_mutex);
                prepared_repos.push_back(repo.get());
            }

            signal_prepared_repo.notify_one();
        } catch (const libdnf::repo::RepoDownloadError & e) {
            if (!repo->get_config().skip_if_unavailable().get_value()) {
                except_in_main_thread = true;
                finish_sack_loader();
                throw;
            } else {
                base->get_logger()->warning(
                    "Error loading repo \"{}\" (skipping due to \"skip_if_unavailable=true\"): {}",
                    repo->get_id(),
                    e.what());  // TODO(lukash) we should print nested exceptions
            }
        } catch (const std::runtime_error & e) {
            except_in_main_thread = true;
            finish_sack_loader();
            throw;
        }
    }

    finish_sack_loader();
    catch_thread_sack_loader_exceptions();

    base->get_rpm_package_sack()->load_config_excludes_includes();
}


void RepoSack::update_and_load_enabled_repos(bool load_system, Repo::LoadFlags flags) {
    if (load_system) {
        // create the system repository if it does not exist
        base->get_repo_sack()->get_system_repo();
    }

    libdnf::repo::RepoQuery repos(base);
    repos.filter_enabled(true);

    if (!load_system) {
        repos.filter_type(Repo::Type::SYSTEM, libdnf::sack::QueryCmp::NEQ);
    }

    update_and_load_repos(repos, flags);
}


void RepoSack::dump_debugdata(const std::string & dir) {
    Solver * solver = solver_create(*get_pool(base));

    try {
        std::filesystem::create_directory(dir);

        int ret = testcase_write(solver, dir.c_str(), 0, NULL, NULL);
        if (!ret) {
            throw SystemError(errno, "Failed to write debug data to {}", dir);
        }
    } catch (...) {
        solver_free(solver);
        throw;
    }
    solver_free(solver);
}


void RepoSack::create_repos_from_file(const std::string & path) {
    auto & logger = *base->get_logger();
    ConfigParser parser;
    parser.read(path);
    const auto & cfg_parser_data = parser.get_data();
    for (const auto & cfg_parser_data_iter : cfg_parser_data) {
        const auto & section = cfg_parser_data_iter.first;
        if (section == "main") {
            continue;
        }
        auto repo_id = base->get_vars()->substitute(section);

        logger.debug("Creating repo \"{}\" from config file \"{}\" section \"{}\"", repo_id, path, section);

        auto repo = create_repo(repo_id);
        auto & repo_cfg = repo->get_config();
        repo_cfg.load_from_parser(parser, section, *base->get_vars(), *base->get_logger());

        if (repo_cfg.name().get_priority() == Option::Priority::DEFAULT) {
            logger.debug("Repo \"{}\" is missing name in configuration file \"{}\", using id.", repo_id, path);
            repo_cfg.name().set(Option::Priority::REPOCONFIG, repo_id);
        }
    }
}

void RepoSack::create_repos_from_config_file() {
    create_repos_from_file(std::filesystem::path(base->get_config().config_file_path().get_value()));
}

void RepoSack::create_repos_from_dir(const std::string & dir_path) {
    std::filesystem::directory_iterator di(dir_path);
    std::vector<std::filesystem::path> paths;
    for (auto & dentry : di) {
        auto & path = dentry.path();
        if (dentry.is_regular_file() && path.extension() == ".repo") {
            paths.push_back(path);
        }
    }
    std::sort(paths.begin(), paths.end());
    for (auto & path : paths) {
        create_repos_from_file(path);
    }
}

void RepoSack::create_repos_from_reposdir() {
    for (auto & dir : base->get_config().reposdir().get_value()) {
        if (std::filesystem::exists(dir)) {
            create_repos_from_dir(dir);
        }
    }
}

void RepoSack::create_repos_from_system_configuration() {
    create_repos_from_config_file();
    create_repos_from_reposdir();
}

BaseWeakPtr RepoSack::get_base() const {
    return base;
}


void RepoSack::internalize_repos() {
    auto rq = RepoQuery(base);
    for (auto & repo : rq.get_data()) {
        repo->internalize();
    }

    if (system_repo) {
        system_repo->internalize();
    }

    if (cmdline_repo) {
        cmdline_repo->internalize();
    }
}

}  // namespace libdnf::repo
