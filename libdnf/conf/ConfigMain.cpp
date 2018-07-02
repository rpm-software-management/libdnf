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

#include "ConfigMain.hpp"
#include "Const.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <istream>
#include <ostream>
#include <fstream>
#include <utility>

#include <dirent.h>
#include <glob.h>
#include <string.h>
#include <sys/types.h>

#include "bgettext/bgettext-lib.h"
#include "tinyformat/tinyformat.hpp"

namespace libdnf {

/**
* @brief Converts a friendly bandwidth option to bytes
* 
* Function converts a friendly bandwidth option to bytes.  The input
* should be a string containing a (possibly floating point)
* number followed by an optional single character unit. Valid
* units are 'k', 'M', 'G'. Case is ignored. The convention that
* 1k = 1024 bytes is used.
* 
* @param str Bandwidth as user friendly string
* @return int Number of bytes
*/
static int strToBytes(const std::string & str)
{
    if (str.empty())
        throw Option::InvalidValue(_("no value specified"));

    std::size_t idx;
    auto res = std::stod(str, &idx);
    if (res < 0)
        throw Option::InvalidValue(tfm::format(_("seconds value '%s' must not be negative"), str));

    if (idx < str.length()) {
        if (idx < str.length() - 1)
            throw Option::InvalidValue(tfm::format(_("could not convert '%s' to bytes"), str));
        switch (str.back()) {
            case 'k': case 'K':
                res *= 1024;
                break;
            case 'm': case 'M':
                res *= 1024 * 1024;
                break;
            case 'g': case 'G':
                res *= 1024 * 1024 * 1024;
                break;
            default:
                throw Option::InvalidValue(tfm::format(_("unknown unit '%s'"), str.back()));
        }
    }

    return res;
}

static void addFromFile(std::ostream & out, const std::string & filePath)
{
    std::ifstream ifs(filePath);
    if (!ifs)
        throw std::runtime_error("addFromFile(): Can't open file");
    ifs.exceptions(std::ifstream::badbit);

    std::string line;
    while (!ifs.eof()) {
        std::getline(ifs, line);
        auto start = line.find_first_not_of(" \t\r");
        if (start == std::string::npos)
            continue;
        if (line[start] == '#')
            continue;
        auto end = line.find_last_not_of(" \t\r");

        out.write(line.c_str()+start, end - start + 1);
        out.put(' ');
    }
}

static void addFromFiles(std::ostream & out, const std::string & globPath)
{
    glob_t globBuf;
    glob(globPath.c_str(), GLOB_MARK | GLOB_NOSORT, NULL, &globBuf);
    for (size_t i = 0; i < globBuf.gl_pathc; ++i) {
        auto path = globBuf.gl_pathv[i];
        if (path[strlen(path)-1] != '/')
            addFromFile(out, path);
    }
    globfree(&globBuf);
}

/**
* @brief Replaces globs (like /etc/foo.d/\\*.foo) by content of matching files.
* 
* Ignores comment lines (start with '#') and blank lines in files.
* Result:
* Words delimited by spaces. Characters ',' and '\n' are replaced by spaces.
* Extra spaces are removed.
* @param strWithGlobs Input string with globs
* @return Words delimited by space
*/
static std::string resolveGlobs(const std::string & strWithGlobs)
{
    std::ostringstream res;
    std::string::size_type start{0};
    while (start < strWithGlobs.length()) {
        auto end = strWithGlobs.find_first_of(" ,\n", start);
        if (strWithGlobs.compare(start, 5, "glob:") == 0) {
            start += 5;
            if (start >= strWithGlobs.length())
                break;
            if (end == std::string::npos) {
                addFromFiles(res, strWithGlobs.substr(start));
                break;
            }
            if (end - start != 0)
                addFromFiles(res, strWithGlobs.substr(start, end - start));
        } else {
            if (end == std::string::npos) {
                res << strWithGlobs.substr(start);
                break;
            }
            if (end - start != 0)
                res << strWithGlobs.substr(start, end - start) << " ";
        }
        start = end + 1;
    }
    return res.str();
}

class ConfigMain::Impl {
    friend class ConfigMain;

    Impl(Config & owner) : owner(owner) {}

    Config & owner;

    OptionNumber<std::int32_t> debuglevel{2, 0, 10};
    OptionBinding debugLevelBinding{owner, debuglevel, "debuglevel"};

    OptionNumber<std::int32_t> errorlevel{2, 0, 10};
    OptionBinding errorLevelBinding{owner, errorlevel, "errorlevel"};

    OptionPath installroot{"/"};
    OptionBinding installRootBinding{owner, installroot, "installroot"};

    OptionPath config_file_path{CONF_FILENAME};
    OptionBinding configFilePathBinding{owner, config_file_path, "config_file_path"};

    OptionBool plugins{true};
    OptionBinding pluginsBinding{owner, plugins, "plugins"};

    OptionStringList pluginpath{std::vector<std::string>{}};
    OptionBinding pluginPathBinding{owner, pluginpath, "pluginpath"};

    OptionStringList pluginconfpath{std::vector<std::string>{}};
    OptionBinding pluginConfPathBinding{owner, pluginconfpath, "pluginconfpath"};

    OptionPath persistdir{PERSISTDIR};
    OptionBinding persistDirBinding{owner, persistdir, "persistdir"};

    OptionBool transformdb{true};
    OptionBinding transformDbBinding{owner, transformdb, "transformdb"};

    OptionNumber<std::int32_t> recent{7, 0};
    OptionBinding recentBinding{owner, recent, "recent"};

    OptionBool reset_nice{true};
    OptionBinding resetNiceBinding{owner, reset_nice, "reset_nice"};

    OptionPath system_cachedir{SYSTEM_CACHEDIR};
    OptionBinding systemCacheDirBindings{owner, system_cachedir, "system_cachedir"};

    OptionBool cacheonly{false};
    OptionBinding cacheOnlyBinding{owner, cacheonly, "cacheonly"};

    OptionBool keepcache{false};
    OptionBinding keepCacheBinding{owner, keepcache, "keepcache"};

    OptionString logdir{"/var/log"};
    OptionBinding logDirBinding{owner, logdir, "logdir"};

    OptionStringList reposdir{{"/etc/yum.repos.d", "/etc/yum/repos.d", "/etc/distro.repos.d"}};
    OptionBinding reposDirBinding{owner, reposdir, "reposdir"};

    OptionBool debug_solver{false};
    OptionBinding debugSolverBinding{owner, debug_solver, "debug_solver"};

    OptionStringListAppend installonlypkgs{INSTALLONLYPKGS};
    OptionBinding installOnlyPkgsBinding{owner, installonlypkgs, "installonlypkgs"};

    OptionStringList group_package_types{GROUP_PACKAGE_TYPES};
    OptionBinding groupPackageTypesBinding{owner, group_package_types, "group_package_types"};

    OptionNumber<std::uint32_t> installonly_limit{3, 0,
        [](const std::string & value)->std::uint32_t{
            if (value == "<off>")
                return 0;
            try {
                return std::stoul(value);
            }
            catch (...) {
                return 0;
            }
        }
    };
    OptionBinding installOnlyLimitBinding{owner, installonly_limit, "installonly_limit"};

    OptionStringListAppend tsflags{std::vector<std::string>{}};
    OptionBinding tsFlagsBinding{owner, tsflags, "tsflags"};

    OptionBool assumeyes{false};
    OptionBinding assumeYesBinding{owner, assumeyes, "assumeyes"};

    OptionBool assumeno{false};
    OptionBinding assumeNoBinding{owner, assumeno, "assumeno"};

    OptionBool check_config_file_age{true};
    OptionBinding checkConfigFileAgeBinding{owner, check_config_file_age, "check_config_file_age"};

    OptionBool defaultyes{false};
    OptionBinding defaultYesBinding{owner, defaultyes, "defaultyes"};

    OptionBool diskspacecheck{true};
    OptionBinding diskSpaceCheckBinding{owner, diskspacecheck, "diskspacecheck"};

    OptionBool localpkg_gpgcheck{false};
    OptionBinding localPkgGpgCheckBinding{owner, localpkg_gpgcheck, "localpkg_gpgcheck"};

    OptionBool obsoletes{true};
    OptionBinding obsoletesBinding{owner, obsoletes, "obsoletes"};

    OptionBool showdupesfromrepos{false};
    OptionBinding showDupesFromReposBinding{owner, showdupesfromrepos, "showdupesfromrepos"};

    OptionBool exit_on_lock{false};
    OptionBinding exitOnLockBinding{owner, exit_on_lock, "exit_on_lock"};

    OptionSeconds metadata_timer_sync{60 * 60 * 3}; // 3 hours
    OptionBinding metadataTimerSyncBinding{owner, metadata_timer_sync, "metadata_timer_sync"};

    OptionStringList disable_excludes{std::vector<std::string>{}};
    OptionBinding disableExcludesBinding{owner, disable_excludes, "disable_excludes"};

    OptionEnum<std::string> multilib_policy{"best", {"best", "all"}}; // :api
    OptionBinding multilibPolicyBinding{owner, multilib_policy, "multilib_policy"};

    OptionBool best{false}; // :api
    OptionBinding bestBinding{owner, best, "best"};

    OptionBool install_weak_deps{true};
    OptionBinding installWeakDepsBinding{owner, install_weak_deps, "install_weak_deps"};

    OptionString bugtracker_url{BUGTRACKER};
    OptionBinding bugtrackerUrlBinding{owner, bugtracker_url, "bugtracker_url"};

    OptionEnum<std::string> color{"auto", {"auto", "never", "always"},
        [](const std::string & value){
            const std::array<const char *, 4> always{{"on", "yes", "1", "true"}};
            const std::array<const char *, 4> never{{"off", "no", "0", "false"}};
            const std::array<const char *, 2> aut{{"tty", "if-tty"}};
            std::string tmp;
            if (std::find(always.begin(), always.end(), value) != always.end())
                tmp = "always";
            else if (std::find(never.begin(), never.end(), value) != never.end())
                tmp = "never";
            else if (std::find(aut.begin(), aut.end(), value) != aut.end())
                tmp = "auto";
            else
                tmp = value;
            return tmp;
        }
    };
    OptionBinding colorBinding{owner, color, "color"};

    OptionString color_list_installed_older{"bold"};
    OptionBinding colorListInstalledOlderBinding{owner, color_list_installed_older, "color_list_installed_older"};

    OptionString color_list_installed_newer{"bold,yellow"};
    OptionBinding colorListInstalledNewerBinding{owner, color_list_installed_newer, "color_list_installed_newer"};

    OptionString color_list_installed_reinstall{"normal"};
    OptionBinding colorListInstalledReinstallBinding{owner, color_list_installed_reinstall, "color_list_installed_reinstall"};

    OptionString color_list_installed_extra{"bold,red"};
    OptionBinding colorListInstalledExtraBinding{owner, color_list_installed_extra, "color_list_installed_extra"};

    OptionString color_list_available_upgrade{"bold,blue"};
    OptionBinding colorListAvailableUpgradeBinding{owner, color_list_available_upgrade, "color_list_available_upgrade"};

    OptionString color_list_available_downgrade{"dim,cyan"};
    OptionBinding colorListAvailableDowngradeBinding{owner, color_list_available_downgrade, "color_list_available_downgrade"};

    OptionString color_list_available_reinstall{"bold,underline,green"};
    OptionBinding colorListAvailableReinstallBinding{owner, color_list_available_reinstall, "color_list_available_reinstall"};

    OptionString color_list_available_install{"normal"};
    OptionBinding colorListAvailableInstallBinding{owner, color_list_available_install, "color_list_available_install"};

    OptionString color_update_installed{"normal"};
    OptionBinding colorUpdateInstalledBinding{owner, color_update_installed, "color_update_installed"};

    OptionString color_update_local{"bold"};
    OptionBinding colorUpdateLocalBinding{owner, color_update_local, "color_update_local"};

    OptionString color_update_remote{"normal"};
    OptionBinding colorUpdateRemoteBinding{owner, color_update_remote, "color_update_remote"};

    OptionString color_search_match{"bold"};
    OptionBinding colorSearchMatchBinding{owner, color_search_match, "color_search_match"};

    OptionBool history_record{true};
    OptionBinding historyRecordBinding{owner, history_record, "history_record"};

    OptionStringList history_record_packages{std::vector<std::string>{"dnf", "rpm"}};
    OptionBinding historyRecordPackagesBinding{owner, history_record_packages, "history_record_packages"};

    OptionString rpmverbosity{"info"};
    OptionBinding rpmverbosityBinding{owner, rpmverbosity, "rpmverbosity"};

    OptionBool strict{true}; // :api
    OptionBinding strictBinding{owner, strict, "strict"};

    OptionBool skip_broken{false}; // :yum-compatibility
    OptionBinding skipBrokenBinding{owner, skip_broken, "skip_broken"};

    OptionBool autocheck_running_kernel{true}; // :yum-compatibility
    OptionBinding autocheckRunningKernelBinding{owner, autocheck_running_kernel, "autocheck_running_kernel"};

    OptionBool clean_requirements_on_remove{true};
    OptionBinding cleanRequirementsOnRemoveBinding{owner, clean_requirements_on_remove, "clean_requirements_on_remove"};

    OptionEnum<std::string> history_list_view{"commands", {"single-user-commands", "users", "commands"},
        [](const std::string & value){
            if (value == "cmds" || value == "default")
                return std::string("commands");
            else
                return value;
        }
    };
    OptionBinding historyListViewBinding{owner, history_list_view, "history_list_view"};

    OptionBool upgrade_group_objects_upgrade{true}; // :api
    OptionBinding upgradeGroupObjectsUpgradeBinding{owner, upgrade_group_objects_upgrade, "upgrade_group_objects_upgrade"};

    OptionPath destdir{nullptr};
    OptionBinding destDirBinding{owner, destdir, "destdir"};

    OptionString comment{nullptr};
    OptionBinding commentBinding{owner, comment, "comment"};

    // runtime only options
    OptionBool downloadonly{false};

    OptionBool ignorearch{false};
    OptionBinding ignoreArchBinding{owner, ignorearch, "ignorearch"};


    // Repo main config

    OptionNumber<std::uint32_t> retries{10};
    OptionBinding retriesBinding{owner, retries, "retries"};

    OptionString cachedir{nullptr};
    OptionBinding cacheDirBindings{owner, cachedir, "cachedir"};

    OptionBool fastestmirror{false};
    OptionBinding fastestMirrorBinding{owner, fastestmirror, "fastestmirror"};

    OptionStringListAppend excludepkgs{std::vector<std::string>{}};
    OptionBinding excludePkgsBinding{owner, excludepkgs, "excludepkgs"};
    OptionBinding excludeBinding{owner, excludepkgs, "exclude"}; //compatibility with yum

    OptionStringListAppend includepkgs{std::vector<std::string>{}};
    OptionBinding includePkgsBinding{owner, includepkgs, "includepkgs"};

    OptionString proxy{"", PROXY_URL_REGEX, true};
    OptionBinding proxyBinding{owner, proxy, "proxy"};

    OptionString proxy_username{nullptr};
    OptionBinding proxyUsernameBinding{owner, proxy_username, "proxy_username"};

    OptionString proxy_password{nullptr};
    OptionBinding proxyPasswordBinding{owner, proxy_password, "proxy_password"};

    OptionEnum<std::string> proxy_auth_method{"any", {"any", "none", "basic", "digest",
        "negotiate", "ntlm", "digest_ie", "ntlm_wb"},
        [](const std::string & value){
            auto tmp = value;
            std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
            return tmp;
        }
    };
    OptionBinding proxyAuthMethodBinding{owner, proxy_auth_method, "proxy_auth_method"};

    OptionStringList protected_packages{resolveGlobs("dnf glob:/etc/yum/protected.d/*.conf " \
                                          "glob:/etc/dnf/protected.d/*.conf")};
    OptionBinding protectedPackagesBinding{owner, protected_packages, "protected_packages",
        [&](Option::Priority priority, const std::string & value){
            if (priority >= protected_packages.getPriority())
                protected_packages.set(priority, resolveGlobs(value));
        }, nullptr
    };

    OptionString username{""};
    OptionBinding usernameBinding{owner, username, "username"};

    OptionString password{""};
    OptionBinding passwordBinding{owner, password, "password"};

    OptionBool gpgcheck{false};
    OptionBinding gpgCheckBinding{owner, gpgcheck, "gpgcheck"};

    OptionBool repo_gpgcheck{false};
    OptionBinding repoGpgCheckBinding{owner, repo_gpgcheck, "repo_gpgcheck"};

    OptionBool enabled{true};
    OptionBinding enabledBinding{owner, enabled, "enabled"};

    OptionBool enablegroups{true};
    OptionBinding enableGroupsBinding{owner, enablegroups, "enablegroups"};

    OptionNumber<std::uint32_t> bandwidth{0, strToBytes};
    OptionBinding bandwidthBinding{owner, bandwidth, "bandwidth"};

    OptionNumber<std::uint32_t> minrate{1000, strToBytes};
    OptionBinding minRateBinding{owner, minrate, "minrate"};

    OptionEnum<std::string> ip_resolve{"whatever", {"ipv4", "ipv6", "whatever"},
        [](const std::string & value){
            auto tmp = value;
            if (value == "4") tmp = "ipv4";
            else if (value == "6") tmp = "ipv6";
            else std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
            return tmp;
        }
    };
    OptionBinding ipResolveBinding{owner, ip_resolve, "ip_resolve"};

    OptionNumber<float> throttle{0, 0,
        [](const std::string & value)->float{
            if (!value.empty() && value.back()=='%') {
                std::size_t idx;
                auto res = std::stod(value, &idx);
                if (res < 0 || res > 100)
                    throw Option::InvalidValue(tfm::format(_("percentage '%s' is out of range"), value));
                return res/100;
            }
            return strToBytes(value);
        }
    };
    OptionBinding throttleBinding{owner, throttle, "throttle"};

    OptionSeconds timeout{30};
    OptionBinding timeoutBinding{owner, timeout, "timeout"};

    OptionNumber<std::uint32_t> max_parallel_downloads{3, 1};
    OptionBinding maxParallelDownloadsBinding{owner, max_parallel_downloads, "max_parallel_downloads"};

    OptionSeconds metadata_expire{60 * 60 * 48};
    OptionBinding metadataExpireBinding{owner, metadata_expire, "metadata_expire"};

    OptionString sslcacert{""};
    OptionBinding sslCaCertBinding{owner, sslcacert, "sslcacert"};

    OptionBool sslverify{true};
    OptionBinding sslVerifyBinding{owner, sslverify, "sslverify"};

    OptionString sslclientcert{""};
    OptionBinding sslClientCertBinding{owner, sslclientcert, "sslclientcert"};

    OptionString sslclientkey{""};
    OptionBinding sslClientKeyBinding{owner, sslclientkey, "sslclientkey"};

    OptionBool deltarpm{true};
    OptionBinding deltaRpmBinding{owner, deltarpm, "deltarpm"};

    OptionNumber<std::uint32_t> deltarpm_percentage{75};
    OptionBinding deltaRpmPercentageBinding{owner, deltarpm_percentage, "deltarpm_percentage"};

    OptionPath modulesdir{CONF_MODULESDIR};
    OptionBinding modulesdirBinding{owner, modulesdir, "modulesdir"};

    OptionPath moduledefaultsdir{CONF_MODULEDEFAULTSDIR};
    OptionBinding moduledefaultsdirBinding{owner, moduledefaultsdir, "moduledefaultsdir"};

};

ConfigMain::ConfigMain() { pImpl = std::unique_ptr<Impl>(new Impl(*this)); }
ConfigMain::~ConfigMain() = default;

OptionNumber<std::int32_t> & ConfigMain::debuglevel() { return pImpl->debuglevel; }
OptionNumber<std::int32_t> & ConfigMain::errorlevel() { return pImpl->errorlevel; }
OptionString & ConfigMain::installroot() { return pImpl->installroot; }
OptionString & ConfigMain::config_file_path() { return pImpl->config_file_path; }
OptionBool & ConfigMain::plugins() { return pImpl->plugins; }
OptionStringList & ConfigMain::pluginpath() { return pImpl->pluginpath; }
OptionStringList & ConfigMain::pluginconfpath() { return pImpl->pluginconfpath; }
OptionString & ConfigMain::persistdir() { return pImpl->persistdir; }
OptionBool & ConfigMain::transformdb() { return pImpl->transformdb; }
OptionNumber<std::int32_t> & ConfigMain::recent() { return pImpl->recent; }
OptionBool & ConfigMain::reset_nice() { return pImpl->reset_nice; }
OptionString & ConfigMain::system_cachedir() { return pImpl->system_cachedir; }
OptionBool & ConfigMain::cacheonly() { return pImpl->cacheonly; }
OptionBool & ConfigMain::keepcache() { return pImpl->keepcache; }
OptionString & ConfigMain::logdir() { return pImpl->logdir; }
OptionStringList & ConfigMain::reposdir() { return pImpl->reposdir; }
OptionBool & ConfigMain::debug_solver() { return pImpl->debug_solver; }
OptionStringListAppend & ConfigMain::installonlypkgs() { return pImpl->installonlypkgs; }
OptionStringList & ConfigMain::group_package_types() { return pImpl->group_package_types; }
OptionNumber<std::uint32_t> & ConfigMain::installonly_limit() { return pImpl->installonly_limit; }
OptionStringListAppend & ConfigMain::tsflags() { return pImpl->tsflags; }
OptionBool & ConfigMain::assumeyes() { return pImpl->assumeyes; }
OptionBool & ConfigMain::assumeno() { return pImpl->assumeno; }
OptionBool & ConfigMain::check_config_file_age() { return pImpl->check_config_file_age; }
OptionBool & ConfigMain::defaultyes() { return pImpl->defaultyes; }
OptionBool & ConfigMain::diskspacecheck() { return pImpl->diskspacecheck; }
OptionBool & ConfigMain::localpkg_gpgcheck() { return pImpl->localpkg_gpgcheck; }
OptionBool & ConfigMain::obsoletes() { return pImpl->obsoletes; }
OptionBool & ConfigMain::showdupesfromrepos() { return pImpl->showdupesfromrepos; }
OptionBool & ConfigMain::exit_on_lock() { return pImpl->exit_on_lock; }
OptionSeconds & ConfigMain::metadata_timer_sync() { return pImpl->metadata_timer_sync; }
OptionStringList & ConfigMain::disable_excludes() { return pImpl->disable_excludes; }
OptionEnum<std::string> & ConfigMain::multilib_policy() { return pImpl->multilib_policy; }
OptionBool & ConfigMain::best() { return pImpl->best; }
OptionBool & ConfigMain::install_weak_deps() { return pImpl->install_weak_deps; }
OptionString & ConfigMain::bugtracker_url() { return pImpl->bugtracker_url; }
OptionEnum<std::string> & ConfigMain::color() { return pImpl->color; }
OptionString & ConfigMain::color_list_installed_older() { return pImpl->color_list_installed_older; }
OptionString & ConfigMain::color_list_installed_newer() { return pImpl->color_list_installed_newer; }
OptionString & ConfigMain::color_list_installed_reinstall() { return pImpl->color_list_installed_reinstall; }
OptionString & ConfigMain::color_list_installed_extra() { return pImpl->color_list_installed_extra; }
OptionString & ConfigMain::color_list_available_upgrade() { return pImpl->color_list_available_upgrade; }
OptionString & ConfigMain::color_list_available_downgrade() { return pImpl->color_list_available_downgrade; }
OptionString & ConfigMain::color_list_available_reinstall() { return pImpl->color_list_available_reinstall; }
OptionString & ConfigMain::color_list_available_install() { return pImpl->color_list_available_install; }
OptionString & ConfigMain::color_update_installed() { return pImpl->color_update_installed; }
OptionString & ConfigMain::color_update_local() { return pImpl->color_update_local; }
OptionString & ConfigMain::colorUpdateRemote() { return pImpl->color_update_remote; }
OptionString & ConfigMain::color_search_match() { return pImpl->color_search_match; }
OptionBool & ConfigMain::history_record() { return pImpl->history_record; }
OptionStringList & ConfigMain::history_record_packages() { return pImpl->history_record_packages; }
OptionString & ConfigMain::rpmverbosity() { return pImpl->rpmverbosity; }
OptionBool & ConfigMain::strict() { return pImpl->strict; }
OptionBool & ConfigMain::skip_broken() { return pImpl->skip_broken; }
OptionBool & ConfigMain::autocheck_running_kernel() { return pImpl->autocheck_running_kernel; }
OptionBool & ConfigMain::clean_requirements_on_remove() { return pImpl->clean_requirements_on_remove; }
OptionEnum<std::string> & ConfigMain::history_list_view() { return pImpl->history_list_view; }
OptionBool & ConfigMain::upgrade_group_objects_upgrade() { return pImpl->upgrade_group_objects_upgrade; }
OptionPath & ConfigMain::destdir() { return pImpl->destdir; }
OptionString & ConfigMain::comment() { return pImpl->comment; }
OptionBool & ConfigMain::downloadonly() { return pImpl->downloadonly; }
OptionBool & ConfigMain::ignorearch() { return pImpl->ignorearch; }

// Repo main config
OptionNumber<std::uint32_t> & ConfigMain::retries() { return pImpl->retries; }
OptionString & ConfigMain::cachedir() { return pImpl->cachedir; }
OptionBool & ConfigMain::fastestmirror() { return pImpl->fastestmirror; }
OptionStringListAppend & ConfigMain::excludepkgs() { return pImpl->excludepkgs; }
OptionStringListAppend & ConfigMain::includepkgs() { return pImpl->includepkgs; }
OptionString & ConfigMain::proxy() { return pImpl->proxy; }
OptionString & ConfigMain::proxy_username() { return pImpl->proxy_username; }
OptionString & ConfigMain::proxy_password() { return pImpl->proxy_password; }
OptionEnum<std::string> & ConfigMain::proxy_auth_method() { return pImpl->proxy_auth_method; }
OptionStringList & ConfigMain::protected_packages() { return pImpl->protected_packages; }
OptionString & ConfigMain::username() { return pImpl->username; }
OptionString & ConfigMain::password() { return pImpl->password; }
OptionBool & ConfigMain::gpgcheck() { return pImpl->gpgcheck; }
OptionBool & ConfigMain::repo_gpgcheck() { return pImpl->repo_gpgcheck; }
OptionBool & ConfigMain::enabled() { return pImpl->enabled; }
OptionBool & ConfigMain::enablegroups() { return pImpl->enablegroups; }
OptionNumber<std::uint32_t> & ConfigMain::bandwidth() { return pImpl->bandwidth; }
OptionNumber<std::uint32_t> & ConfigMain::minrate() { return pImpl->minrate; }
OptionEnum<std::string> & ConfigMain::ip_resolve() { return pImpl->ip_resolve; }
OptionNumber<float> & ConfigMain::throttle() { return pImpl->throttle; }
OptionSeconds & ConfigMain::timeout() { return pImpl->timeout; }
OptionNumber<std::uint32_t> & ConfigMain::max_parallel_downloads() { return pImpl->max_parallel_downloads; }
OptionSeconds & ConfigMain::metadata_expire() { return pImpl->metadata_expire; }
OptionString & ConfigMain::sslcacert() { return pImpl->sslcacert; }
OptionBool & ConfigMain::sslverify() { return pImpl->sslverify; }
OptionString & ConfigMain::sslclientcert() { return pImpl->sslclientcert; }
OptionString & ConfigMain::sslclientkey() { return pImpl->sslclientkey; }
OptionBool & ConfigMain::deltarpm() { return pImpl->deltarpm; }
OptionNumber<std::uint32_t> & ConfigMain::deltarpm_percentage() { return pImpl->deltarpm_percentage; }

// Modules main config
OptionPath & ConfigMain::modulesdir() { return pImpl->modulesdir; }
OptionPath & ConfigMain::moduledefaultsdir() { return pImpl->moduledefaultsdir; }

}
