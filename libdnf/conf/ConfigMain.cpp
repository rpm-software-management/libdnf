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
#include "Config-private.hpp"

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
    OptionBinds::Item debugLevelBinding{owner.optBinds(), debuglevel, "debuglevel"};

    OptionNumber<std::int32_t> errorlevel{3, 0, 10};
    OptionBinds::Item errorLevelBinding{owner.optBinds(), errorlevel, "errorlevel"};

    OptionPath installroot{"/"};
    OptionBinds::Item installRootBinding{owner.optBinds(), installroot, "installroot"};

    OptionPath config_file_path{CONF_FILENAME};
    OptionBinds::Item configFilePathBinding{owner.optBinds(), config_file_path, "config_file_path"};

    OptionBool plugins{true};
    OptionBinds::Item pluginsBinding{owner.optBinds(), plugins, "plugins"};

    OptionStringList pluginpath{std::vector<std::string>{}};
    OptionBinds::Item pluginPathBinding{owner.optBinds(), pluginpath, "pluginpath"};

    OptionStringList pluginconfpath{std::vector<std::string>{}};
    OptionBinds::Item pluginConfPathBinding{owner.optBinds(), pluginconfpath, "pluginconfpath"};

    OptionPath persistdir{PERSISTDIR};
    OptionBinds::Item persistDirBinding{owner.optBinds(), persistdir, "persistdir"};

    OptionBool transformdb{true};
    OptionBinds::Item transformDbBinding{owner.optBinds(), transformdb, "transformdb"};

    OptionNumber<std::int32_t> recent{7, 0};
    OptionBinds::Item recentBinding{owner.optBinds(), recent, "recent"};

    OptionBool reset_nice{true};
    OptionBinds::Item resetNiceBinding{owner.optBinds(), reset_nice, "reset_nice"};

    OptionPath system_cachedir{SYSTEM_CACHEDIR};
    OptionBinds::Item systemCacheDirBindings{owner.optBinds(), system_cachedir, "system_cachedir"};

    OptionBool cacheonly{false};
    OptionBinds::Item cacheOnlyBinding{owner.optBinds(), cacheonly, "cacheonly"};

    OptionBool keepcache{false};
    OptionBinds::Item keepCacheBinding{owner.optBinds(), keepcache, "keepcache"};

    OptionString logdir{"/var/log"};
    OptionBinds::Item logDirBinding{owner.optBinds(), logdir, "logdir"};

    OptionStringList reposdir{{"/etc/yum.repos.d", "/etc/yum/repos.d", "/etc/distro.repos.d"}};
    OptionBinds::Item reposDirBinding{owner.optBinds(), reposdir, "reposdir"};

    OptionBool debug_solver{false};
    OptionBinds::Item debugSolverBinding{owner.optBinds(), debug_solver, "debug_solver"};

    OptionStringList installonlypkgs{INSTALLONLYPKGS};
    OptionBinds::Item installOnlyPkgsBinding{owner.optBinds(), installonlypkgs, "installonlypkgs",
        [&](Option::Priority priority, const std::string & value){
            optionTListAppend(installonlypkgs, priority, value);
        }, nullptr, true
    };

    OptionStringList group_package_types{GROUP_PACKAGE_TYPES};
    OptionBinds::Item groupPackageTypesBinding{owner.optBinds(), group_package_types, "group_package_types"};

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
    OptionBinds::Item installOnlyLimitBinding{owner.optBinds(), installonly_limit, "installonly_limit"};

    OptionStringList tsflags{std::vector<std::string>{}};
    OptionBinds::Item tsFlagsBinding{owner.optBinds(), tsflags, "tsflags",
        [&](Option::Priority priority, const std::string & value){
            optionTListAppend(tsflags, priority, value);
        }, nullptr, true
    };

    OptionBool assumeyes{false};
    OptionBinds::Item assumeYesBinding{owner.optBinds(), assumeyes, "assumeyes"};

    OptionBool assumeno{false};
    OptionBinds::Item assumeNoBinding{owner.optBinds(), assumeno, "assumeno"};

    OptionBool check_config_file_age{true};
    OptionBinds::Item checkConfigFileAgeBinding{owner.optBinds(), check_config_file_age, "check_config_file_age"};

    OptionBool defaultyes{false};
    OptionBinds::Item defaultYesBinding{owner.optBinds(), defaultyes, "defaultyes"};

    OptionBool diskspacecheck{true};
    OptionBinds::Item diskSpaceCheckBinding{owner.optBinds(), diskspacecheck, "diskspacecheck"};

    OptionBool localpkg_gpgcheck{false};
    OptionBinds::Item localPkgGpgCheckBinding{owner.optBinds(), localpkg_gpgcheck, "localpkg_gpgcheck"};

    OptionBool gpgkey_dns_verification{false};
    OptionBinds::Item GpgkeyDnsVerificationBinding{owner.optBinds(), gpgkey_dns_verification, "gpgkey_dns_verification"};

    OptionBool obsoletes{true};
    OptionBinds::Item obsoletesBinding{owner.optBinds(), obsoletes, "obsoletes"};

    OptionBool showdupesfromrepos{false};
    OptionBinds::Item showDupesFromReposBinding{owner.optBinds(), showdupesfromrepos, "showdupesfromrepos"};

    OptionBool exit_on_lock{false};
    OptionBinds::Item exitOnLockBinding{owner.optBinds(), exit_on_lock, "exit_on_lock"};

    OptionSeconds metadata_timer_sync{60 * 60 * 3}; // 3 hours
    OptionBinds::Item metadataTimerSyncBinding{owner.optBinds(), metadata_timer_sync, "metadata_timer_sync"};

    OptionStringList disable_excludes{std::vector<std::string>{}};
    OptionBinds::Item disableExcludesBinding{owner.optBinds(), disable_excludes, "disable_excludes"};

    OptionEnum<std::string> multilib_policy{"best", {"best", "all"}}; // :api
    OptionBinds::Item multilibPolicyBinding{owner.optBinds(), multilib_policy, "multilib_policy"};

    OptionBool best{true}; // :api
    OptionBinds::Item bestBinding{owner.optBinds(), best, "best"};

    OptionBool install_weak_deps{true};
    OptionBinds::Item installWeakDepsBinding{owner.optBinds(), install_weak_deps, "install_weak_deps"};

    OptionString bugtracker_url{BUGTRACKER};
    OptionBinds::Item bugtrackerUrlBinding{owner.optBinds(), bugtracker_url, "bugtracker_url"};

    OptionBool zchunk{true};
    OptionBinds::Item zchunkBinding{owner.optBinds(), zchunk, "zchunk"};

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
    OptionBinds::Item colorBinding{owner.optBinds(), color, "color"};

    OptionString color_list_installed_older{"bold"};
    OptionBinds::Item colorListInstalledOlderBinding{owner.optBinds(), color_list_installed_older, "color_list_installed_older"};

    OptionString color_list_installed_newer{"bold,yellow"};
    OptionBinds::Item colorListInstalledNewerBinding{owner.optBinds(), color_list_installed_newer, "color_list_installed_newer"};

    OptionString color_list_installed_reinstall{"normal"};
    OptionBinds::Item colorListInstalledReinstallBinding{owner.optBinds(), color_list_installed_reinstall, "color_list_installed_reinstall"};

    OptionString color_list_installed_extra{"bold,red"};
    OptionBinds::Item colorListInstalledExtraBinding{owner.optBinds(), color_list_installed_extra, "color_list_installed_extra"};

    OptionString color_list_available_upgrade{"bold,blue"};
    OptionBinds::Item colorListAvailableUpgradeBinding{owner.optBinds(), color_list_available_upgrade, "color_list_available_upgrade"};

    OptionString color_list_available_downgrade{"dim,cyan"};
    OptionBinds::Item colorListAvailableDowngradeBinding{owner.optBinds(), color_list_available_downgrade, "color_list_available_downgrade"};

    OptionString color_list_available_reinstall{"bold,underline,green"};
    OptionBinds::Item colorListAvailableReinstallBinding{owner.optBinds(), color_list_available_reinstall, "color_list_available_reinstall"};

    OptionString color_list_available_install{"normal"};
    OptionBinds::Item colorListAvailableInstallBinding{owner.optBinds(), color_list_available_install, "color_list_available_install"};

    OptionString color_update_installed{"normal"};
    OptionBinds::Item colorUpdateInstalledBinding{owner.optBinds(), color_update_installed, "color_update_installed"};

    OptionString color_update_local{"bold"};
    OptionBinds::Item colorUpdateLocalBinding{owner.optBinds(), color_update_local, "color_update_local"};

    OptionString color_update_remote{"normal"};
    OptionBinds::Item colorUpdateRemoteBinding{owner.optBinds(), color_update_remote, "color_update_remote"};

    OptionString color_search_match{"bold"};
    OptionBinds::Item colorSearchMatchBinding{owner.optBinds(), color_search_match, "color_search_match"};

    OptionBool history_record{true};
    OptionBinds::Item historyRecordBinding{owner.optBinds(), history_record, "history_record"};

    OptionStringList history_record_packages{std::vector<std::string>{"dnf", "rpm"}};
    OptionBinds::Item historyRecordPackagesBinding{owner.optBinds(), history_record_packages, "history_record_packages"};

    OptionString rpmverbosity{"info"};
    OptionBinds::Item rpmverbosityBinding{owner.optBinds(), rpmverbosity, "rpmverbosity"};

    OptionBool strict{true}; // :api
    OptionBinds::Item strictBinding{owner.optBinds(), strict, "strict"};

    OptionBool skip_broken{false}; // :yum-compatibility
    OptionBinds::Item skipBrokenBinding{owner.optBinds(), skip_broken, "skip_broken"};

    OptionBool autocheck_running_kernel{true}; // :yum-compatibility
    OptionBinds::Item autocheckRunningKernelBinding{owner.optBinds(), autocheck_running_kernel, "autocheck_running_kernel"};

    OptionBool clean_requirements_on_remove{true};
    OptionBinds::Item cleanRequirementsOnRemoveBinding{owner.optBinds(), clean_requirements_on_remove, "clean_requirements_on_remove"};

    OptionEnum<std::string> history_list_view{"commands", {"single-user-commands", "users", "commands"},
        [](const std::string & value){
            if (value == "cmds" || value == "default")
                return std::string("commands");
            else
                return value;
        }
    };
    OptionBinds::Item historyListViewBinding{owner.optBinds(), history_list_view, "history_list_view"};

    OptionBool upgrade_group_objects_upgrade{true}; // :api
    OptionBinds::Item upgradeGroupObjectsUpgradeBinding{owner.optBinds(), upgrade_group_objects_upgrade, "upgrade_group_objects_upgrade"};

    OptionPath destdir{nullptr};
    OptionBinds::Item destDirBinding{owner.optBinds(), destdir, "destdir"};

    OptionString comment{nullptr};
    OptionBinds::Item commentBinding{owner.optBinds(), comment, "comment"};

    // runtime only options
    OptionBool downloadonly{false};

    OptionBool ignorearch{false};
    OptionBinds::Item ignoreArchBinding{owner.optBinds(), ignorearch, "ignorearch"};

    OptionString module_platform_id{nullptr};
    OptionBinds::Item modulePlatformId{owner.optBinds(), module_platform_id, "module_platform_id"};

    // Repo main config

    OptionNumber<std::uint32_t> retries{10};
    OptionBinds::Item retriesBinding{owner.optBinds(), retries, "retries"};

    OptionString cachedir{nullptr};
    OptionBinds::Item cacheDirBindings{owner.optBinds(), cachedir, "cachedir"};

    OptionBool fastestmirror{false};
    OptionBinds::Item fastestMirrorBinding{owner.optBinds(), fastestmirror, "fastestmirror"};

    OptionStringList excludepkgs{std::vector<std::string>{}};
    OptionBinds::Item excludePkgsBinding{owner.optBinds(), excludepkgs, "excludepkgs",
        [&](Option::Priority priority, const std::string & value){
            optionTListAppend(excludepkgs, priority, value);
        }, nullptr, true
    };
    OptionBinds::Item excludeBinding{owner.optBinds(), excludepkgs, "exclude", //compatibility with yum
        [&](Option::Priority priority, const std::string & value){
            optionTListAppend(excludepkgs, priority, value);
        }, nullptr, true
    };

    OptionStringList includepkgs{std::vector<std::string>{}};
    OptionBinds::Item includePkgsBinding{owner.optBinds(), includepkgs, "includepkgs",
        [&](Option::Priority priority, const std::string & value){
            optionTListAppend(includepkgs, priority, value);
        }, nullptr, true
    };

    OptionString proxy{""};
    OptionBinds::Item proxyBinding{owner.optBinds(), proxy, "proxy"};

    OptionString proxy_username{nullptr};
    OptionBinds::Item proxyUsernameBinding{owner.optBinds(), proxy_username, "proxy_username"};

    OptionString proxy_password{nullptr};
    OptionBinds::Item proxyPasswordBinding{owner.optBinds(), proxy_password, "proxy_password"};

    OptionEnum<std::string> proxy_auth_method{"any", {"any", "none", "basic", "digest",
        "negotiate", "ntlm", "digest_ie", "ntlm_wb"},
        [](const std::string & value){
            auto tmp = value;
            std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
            return tmp;
        }
    };
    OptionBinds::Item proxyAuthMethodBinding{owner.optBinds(), proxy_auth_method, "proxy_auth_method"};

    OptionStringList protected_packages{resolveGlobs("dnf glob:/etc/yum/protected.d/*.conf " \
                                          "glob:/etc/dnf/protected.d/*.conf")};
    OptionBinds::Item protectedPackagesBinding{owner.optBinds(), protected_packages, "protected_packages",
        [&](Option::Priority priority, const std::string & value){
            if (priority >= protected_packages.getPriority())
                protected_packages.set(priority, resolveGlobs(value));
        }, nullptr, false
    };

    OptionString username{""};
    OptionBinds::Item usernameBinding{owner.optBinds(), username, "username"};

    OptionString password{""};
    OptionBinds::Item passwordBinding{owner.optBinds(), password, "password"};

    OptionBool gpgcheck{false};
    OptionBinds::Item gpgCheckBinding{owner.optBinds(), gpgcheck, "gpgcheck"};

    OptionBool repo_gpgcheck{false};
    OptionBinds::Item repoGpgCheckBinding{owner.optBinds(), repo_gpgcheck, "repo_gpgcheck"};

    OptionBool enabled{true};
    OptionBinds::Item enabledBinding{owner.optBinds(), enabled, "enabled"};

    OptionBool enablegroups{true};
    OptionBinds::Item enableGroupsBinding{owner.optBinds(), enablegroups, "enablegroups"};

    OptionNumber<std::uint32_t> bandwidth{0, strToBytes};
    OptionBinds::Item bandwidthBinding{owner.optBinds(), bandwidth, "bandwidth"};

    OptionNumber<std::uint32_t> minrate{1000, strToBytes};
    OptionBinds::Item minRateBinding{owner.optBinds(), minrate, "minrate"};

    OptionEnum<std::string> ip_resolve{"whatever", {"ipv4", "ipv6", "whatever"},
        [](const std::string & value){
            auto tmp = value;
            if (value == "4") tmp = "ipv4";
            else if (value == "6") tmp = "ipv6";
            else std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::tolower);
            return tmp;
        }
    };
    OptionBinds::Item ipResolveBinding{owner.optBinds(), ip_resolve, "ip_resolve"};

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
    OptionBinds::Item throttleBinding{owner.optBinds(), throttle, "throttle"};

    OptionSeconds timeout{30};
    OptionBinds::Item timeoutBinding{owner.optBinds(), timeout, "timeout"};

    OptionNumber<std::uint32_t> max_parallel_downloads{3, 1};
    OptionBinds::Item maxParallelDownloadsBinding{owner.optBinds(), max_parallel_downloads, "max_parallel_downloads"};

    OptionSeconds metadata_expire{60 * 60 * 48};
    OptionBinds::Item metadataExpireBinding{owner.optBinds(), metadata_expire, "metadata_expire"};

    OptionString sslcacert{""};
    OptionBinds::Item sslCaCertBinding{owner.optBinds(), sslcacert, "sslcacert"};

    OptionBool sslverify{true};
    OptionBinds::Item sslVerifyBinding{owner.optBinds(), sslverify, "sslverify"};

    OptionString sslclientcert{""};
    OptionBinds::Item sslClientCertBinding{owner.optBinds(), sslclientcert, "sslclientcert"};

    OptionString sslclientkey{""};
    OptionBinds::Item sslClientKeyBinding{owner.optBinds(), sslclientkey, "sslclientkey"};

    OptionBool deltarpm{true};
    OptionBinds::Item deltaRpmBinding{owner.optBinds(), deltarpm, "deltarpm"};

    OptionNumber<std::uint32_t> deltarpm_percentage{75};
    OptionBinds::Item deltaRpmPercentageBinding{owner.optBinds(), deltarpm_percentage, "deltarpm_percentage"};
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
OptionStringList & ConfigMain::installonlypkgs() { return pImpl->installonlypkgs; }
OptionStringList & ConfigMain::group_package_types() { return pImpl->group_package_types; }
OptionNumber<std::uint32_t> & ConfigMain::installonly_limit() { return pImpl->installonly_limit; }
OptionStringList & ConfigMain::tsflags() { return pImpl->tsflags; }
OptionBool & ConfigMain::assumeyes() { return pImpl->assumeyes; }
OptionBool & ConfigMain::assumeno() { return pImpl->assumeno; }
OptionBool & ConfigMain::check_config_file_age() { return pImpl->check_config_file_age; }
OptionBool & ConfigMain::defaultyes() { return pImpl->defaultyes; }
OptionBool & ConfigMain::diskspacecheck() { return pImpl->diskspacecheck; }
OptionBool & ConfigMain::localpkg_gpgcheck() { return pImpl->localpkg_gpgcheck; }
OptionBool & ConfigMain::gpgkey_dns_verification() { return pImpl->gpgkey_dns_verification; }
OptionBool & ConfigMain::obsoletes() { return pImpl->obsoletes; }
OptionBool & ConfigMain::showdupesfromrepos() { return pImpl->showdupesfromrepos; }
OptionBool & ConfigMain::exit_on_lock() { return pImpl->exit_on_lock; }
OptionSeconds & ConfigMain::metadata_timer_sync() { return pImpl->metadata_timer_sync; }
OptionStringList & ConfigMain::disable_excludes() { return pImpl->disable_excludes; }
OptionEnum<std::string> & ConfigMain::multilib_policy() { return pImpl->multilib_policy; }
OptionBool & ConfigMain::best() { return pImpl->best; }
OptionBool & ConfigMain::install_weak_deps() { return pImpl->install_weak_deps; }
OptionString & ConfigMain::bugtracker_url() { return pImpl->bugtracker_url; }
OptionBool & ConfigMain::zchunk() { return pImpl->zchunk; }
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
OptionString & ConfigMain::color_update_remote() { return pImpl->color_update_remote; }
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

OptionString & ConfigMain::module_platform_id() { return pImpl->module_platform_id; }

// Repo main config
OptionNumber<std::uint32_t> & ConfigMain::retries() { return pImpl->retries; }
OptionString & ConfigMain::cachedir() { return pImpl->cachedir; }
OptionBool & ConfigMain::fastestmirror() { return pImpl->fastestmirror; }
OptionStringList & ConfigMain::excludepkgs() { return pImpl->excludepkgs; }
OptionStringList & ConfigMain::includepkgs() { return pImpl->includepkgs; }
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

}
