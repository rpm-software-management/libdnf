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

#include "ConfigRepo.hpp"
#include "Const.hpp"
#include "Config-private.hpp"

namespace libdnf {

class ConfigRepo::Impl {
    friend class ConfigRepo;

    Impl(Config & owner, ConfigMain & masterConfig) : owner(owner), masterConfig(masterConfig) {}

    Config & owner;
    ConfigMain & masterConfig;

    OptionString name{""};
    OptionBinds::Item nameBinding{owner.optBinds(), name, "name"};

    OptionChild<OptionBool> enabled{masterConfig.enabled()};
    OptionBinds::Item enabledBinding{owner.optBinds(), enabled, "enabled"};

    OptionChild<OptionString> basecachedir{masterConfig.cachedir()};
    OptionBinds::Item baseCacheDirBindings{owner.optBinds(), basecachedir, "cachedir"};

    OptionStringList baseurl{std::vector<std::string>{}};
    OptionBinds::Item baseUrlBinding{owner.optBinds(), baseurl, "baseurl"};

    OptionString mirrorlist{nullptr};
    OptionBinds::Item mirrorListBinding{owner.optBinds(), mirrorlist, "mirrorlist"};

    OptionString metalink{nullptr};
    OptionBinds::Item metaLinkBinding{owner.optBinds(), metalink, "metalink"};

    OptionString type{""};
    OptionBinds::Item typeBinding{owner.optBinds(), type, "type"};

    OptionString mediaid{""};
    OptionBinds::Item mediaIdBinding{owner.optBinds(), mediaid, "mediaid"};

    OptionStringList gpgkey{std::vector<std::string>{}};
    OptionBinds::Item gpgKeyBinding{owner.optBinds(), gpgkey, "gpgkey"};

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

    OptionChild<OptionBool> fastestmirror{masterConfig.fastestmirror()};
    OptionBinds::Item fastestMirrorBinding{owner.optBinds(), fastestmirror, "fastestmirror"};

    OptionChild<OptionString> proxy{masterConfig.proxy()};
    OptionBinds::Item proxyBinding{owner.optBinds(), proxy, "proxy"};

    OptionChild<OptionString> proxy_username{masterConfig.proxy_username()};
    OptionBinds::Item proxyUsernameBinding{owner.optBinds(), proxy_username, "proxy_username"};

    OptionChild<OptionString> proxy_password{masterConfig.proxy_password()};
    OptionBinds::Item proxyPasswordBinding{owner.optBinds(), proxy_password, "proxy_password"};

    OptionChild<OptionEnum<std::string> > proxy_auth_method{masterConfig.proxy_auth_method()};
    OptionBinds::Item proxyAuthMethodBinding{owner.optBinds(), proxy_auth_method, "proxy_auth_method"};

    OptionChild<OptionString> username{masterConfig.username()};
    OptionBinds::Item usernameBinding{owner.optBinds(), username, "username"};

    OptionChild<OptionString> password{masterConfig.password()};
    OptionBinds::Item passwordBinding{owner.optBinds(), password, "password"};

    OptionChild<OptionStringList> protected_packages{masterConfig.protected_packages()};
    OptionBinds::Item protectedPackagesBinding{owner.optBinds(), protected_packages, "protected_packages"};

    OptionChild<OptionBool> gpgcheck{masterConfig.gpgcheck()};
    OptionBinds::Item gpgCheckBinding{owner.optBinds(), gpgcheck, "gpgcheck"};

    OptionChild<OptionBool> repo_gpgcheck{masterConfig.repo_gpgcheck()};
    OptionBinds::Item repoGpgCheckBinding{owner.optBinds(), repo_gpgcheck, "repo_gpgcheck"};

    OptionChild<OptionBool> enablegroups{masterConfig.enablegroups()};
    OptionBinds::Item enableGroupsBinding{owner.optBinds(), enablegroups, "enablegroups"};

    OptionChild<OptionNumber<std::uint32_t> > retries{masterConfig.retries()};
    OptionBinds::Item retriesBinding{owner.optBinds(), retries, "retries"};

    OptionChild<OptionNumber<std::uint32_t> > bandwidth{masterConfig.bandwidth()};
    OptionBinds::Item bandwidthBinding{owner.optBinds(), bandwidth, "bandwidth"};

    OptionChild<OptionNumber<std::uint32_t> > minrate{masterConfig.minrate()};
    OptionBinds::Item minRateBinding{owner.optBinds(), minrate, "minrate"};

    OptionChild<OptionEnum<std::string> > ip_resolve{masterConfig.ip_resolve()};
    OptionBinds::Item ipResolveBinding{owner.optBinds(), ip_resolve, "ip_resolve"};

    OptionChild<OptionNumber<float> > throttle{masterConfig.throttle()};
    OptionBinds::Item throttleBinding{owner.optBinds(), throttle, "throttle"};

    OptionChild<OptionSeconds> timeout{masterConfig.timeout()};
    OptionBinds::Item timeoutBinding{owner.optBinds(), timeout, "timeout"};

    OptionChild<OptionNumber<std::uint32_t> >  max_parallel_downloads{masterConfig.max_parallel_downloads()};
    OptionBinds::Item maxParallelDownloadsBinding{owner.optBinds(), max_parallel_downloads, "max_parallel_downloads"};

    OptionChild<OptionSeconds> metadata_expire{masterConfig.metadata_expire()};
    OptionBinds::Item metadataExpireBinding{owner.optBinds(), metadata_expire, "metadata_expire"};

    OptionNumber<std::int32_t> cost{1000};
    OptionBinds::Item costBinding{owner.optBinds(), cost, "cost"};

    OptionNumber<std::int32_t> priority{99};
    OptionBinds::Item priorityBinding{owner.optBinds(), priority, "priority"};

    OptionBool module_hotfixes{false};
    OptionBinds::Item moduleHotfixesBinding{owner.optBinds(), module_hotfixes, "module_hotfixes"};

    OptionChild<OptionString> sslcacert{masterConfig.sslcacert()};
    OptionBinds::Item sslCaCertBinding{owner.optBinds(), sslcacert, "sslcacert"};

    OptionChild<OptionBool> sslverify{masterConfig.sslverify()};
    OptionBinds::Item sslVerifyBinding{owner.optBinds(), sslverify, "sslverify"};

    OptionChild<OptionString> sslclientcert{masterConfig.sslclientcert()};
    OptionBinds::Item sslClientCertBinding{owner.optBinds(), sslclientcert, "sslclientcert"};

    OptionChild<OptionString> sslclientkey{masterConfig.sslclientkey()};
    OptionBinds::Item sslClientKeyBinding{owner.optBinds(), sslclientkey, "sslclientkey"};

    OptionChild<OptionBool> deltarpm{masterConfig.deltarpm()};
    OptionBinds::Item deltaRpmBinding{owner.optBinds(), deltarpm, "deltarpm"};

    OptionChild<OptionNumber<std::uint32_t> > deltarpm_percentage{masterConfig.deltarpm_percentage()};
    OptionBinds::Item deltaRpmPercentageBinding{owner.optBinds(), deltarpm_percentage, "deltarpm_percentage"};

    OptionBool skip_if_unavailable{true};
    OptionBinds::Item skipIfUnavailableBinding{owner.optBinds(), skip_if_unavailable, "skip_if_unavailable"};

    OptionString enabled_metadata{""};
    OptionBinds::Item enabledMetadataBinding{owner.optBinds(), enabled_metadata, "enabled_metadata"};

    OptionEnum<std::string> failovermethod{"priority", {"priority", "roundrobin"}};
};

ConfigRepo::ConfigRepo(ConfigMain & masterConfig) : pImpl(new Impl(*this, masterConfig)) {}
ConfigRepo::~ConfigRepo() = default;
ConfigRepo::ConfigRepo(ConfigRepo && src) : pImpl(std::move(src.pImpl)) {}

ConfigMain & ConfigRepo::getMasterConfig() { return pImpl->masterConfig; }

OptionString & ConfigRepo::name() { return pImpl->name; }
OptionChild<OptionBool> & ConfigRepo::enabled() { return pImpl->enabled; }
OptionChild<OptionString> & ConfigRepo::basecachedir() { return pImpl->basecachedir; }
OptionStringList & ConfigRepo::baseurl() { return pImpl->baseurl; }
OptionString & ConfigRepo::mirrorlist() { return pImpl->mirrorlist; }
OptionString & ConfigRepo::metalink() { return pImpl->metalink; }
OptionString & ConfigRepo::type() { return pImpl->type; }
OptionString & ConfigRepo::mediaid() { return pImpl->mediaid; }
OptionStringList & ConfigRepo::gpgkey() { return pImpl->gpgkey; }
OptionStringList & ConfigRepo::excludepkgs() { return pImpl->excludepkgs; }
OptionStringList & ConfigRepo::includepkgs() { return pImpl->includepkgs; }
OptionChild<OptionBool> & ConfigRepo::fastestmirror() { return pImpl->fastestmirror; }
OptionChild<OptionString> & ConfigRepo::proxy() { return pImpl->proxy; }
OptionChild<OptionString> & ConfigRepo::proxy_username() { return pImpl->proxy_username; }
OptionChild<OptionString> & ConfigRepo::proxy_password() { return pImpl->proxy_password; }
OptionChild<OptionEnum<std::string> > & ConfigRepo::proxy_auth_method() { return pImpl->proxy_auth_method; }
OptionChild<OptionString> & ConfigRepo::username() { return pImpl->username; }
OptionChild<OptionString> & ConfigRepo::password() { return pImpl->password; }
OptionChild<OptionStringList> & ConfigRepo::protected_packages() { return pImpl->protected_packages; }
OptionChild<OptionBool> & ConfigRepo::gpgcheck() { return pImpl->gpgcheck; }
OptionChild<OptionBool> & ConfigRepo::repo_gpgcheck() { return pImpl->repo_gpgcheck; }
OptionChild<OptionBool> & ConfigRepo::enablegroups() { return pImpl->enablegroups; }
OptionChild<OptionNumber<std::uint32_t> > & ConfigRepo::retries() { return pImpl->retries; }
OptionChild<OptionNumber<std::uint32_t> > & ConfigRepo::bandwidth() { return pImpl->bandwidth; }
OptionChild<OptionNumber<std::uint32_t> > & ConfigRepo::minrate() { return pImpl->minrate; }
OptionChild<OptionEnum<std::string> > & ConfigRepo::ip_resolve() { return pImpl->ip_resolve; }
OptionChild<OptionNumber<float> > & ConfigRepo::throttle() { return pImpl->throttle; }
OptionChild<OptionSeconds> & ConfigRepo::timeout() { return pImpl->timeout; }
OptionChild<OptionNumber<std::uint32_t> > & ConfigRepo::max_parallel_downloads() { return pImpl->max_parallel_downloads; }
OptionChild<OptionSeconds> & ConfigRepo::metadata_expire() { return pImpl->metadata_expire; }
OptionNumber<std::int32_t> & ConfigRepo::cost() { return pImpl->cost; }
OptionNumber<std::int32_t> & ConfigRepo::priority() { return pImpl->priority; }
OptionBool & ConfigRepo::module_hotfixes() { return pImpl->module_hotfixes; }
OptionChild<OptionString> & ConfigRepo::sslcacert() { return pImpl->sslcacert; }
OptionChild<OptionBool> & ConfigRepo::sslverify() { return pImpl->sslverify; }
OptionChild<OptionString> & ConfigRepo::sslclientcert() { return pImpl->sslclientcert; }
OptionChild<OptionString> & ConfigRepo::sslclientkey() { return pImpl->sslclientkey; }
OptionChild<OptionBool> & ConfigRepo::deltarpm() { return pImpl->deltarpm; }
OptionChild<OptionNumber<std::uint32_t> > & ConfigRepo::deltarpm_percentage() { return pImpl->deltarpm_percentage; }
OptionBool & ConfigRepo::skip_if_unavailable() { return pImpl->skip_if_unavailable; }
OptionString & ConfigRepo::enabled_metadata() { return pImpl->enabled_metadata; }
OptionEnum<std::string> & ConfigRepo::failovermethod() { return pImpl->failovermethod; }

}
