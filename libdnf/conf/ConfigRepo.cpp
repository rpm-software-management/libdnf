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
    OptionBinding nameBinding{owner, name, "name"};

    OptionChild<OptionBool> enabled{masterConfig.enabled()};
    OptionBinding enabledBinding{owner, enabled, "enabled"};

    OptionChild<OptionString> basecachedir{masterConfig.cachedir()};
    OptionBinding baseCacheDirBindings{owner, basecachedir, "cachedir"};

    OptionStringList baseurl{std::vector<std::string>{}};
    OptionBinding baseUrlBinding{owner, baseurl, "baseurl"};

    OptionString mirrorlist{nullptr};
    OptionBinding mirrorListBinding{owner, mirrorlist, "mirrorlist"};

    OptionString metalink{nullptr};
    OptionBinding metaLinkBinding{owner, metalink, "metalink"};

    OptionString type{""};
    OptionBinding typeBinding{owner, type, "type"};

    OptionString mediaid{""};
    OptionBinding mediaIdBinding{owner, mediaid, "mediaid"};

    OptionStringList gpgkey{std::vector<std::string>{}};
    OptionBinding gpgKeyBinding{owner, gpgkey, "gpgkey"};

    OptionStringList excludepkgs{std::vector<std::string>{}};
    OptionBinding excludePkgsBinding{owner, excludepkgs, "excludepkgs",
        [&](Option::Priority priority, const std::string & value){
            optionTListAppend(excludepkgs, priority, value);
        }, nullptr, true
    };
    OptionBinding excludeBinding{owner, excludepkgs, "exclude", //compatibility with yum
        [&](Option::Priority priority, const std::string & value){
            optionTListAppend(excludepkgs, priority, value);
        }, nullptr, true
    };

    OptionStringList includepkgs{std::vector<std::string>{}};
    OptionBinding includePkgsBinding{owner, includepkgs, "includepkgs",
        [&](Option::Priority priority, const std::string & value){
            optionTListAppend(includepkgs, priority, value);
        }, nullptr, true
    };

    OptionChild<OptionBool> fastestmirror{masterConfig.fastestmirror()};
    OptionBinding fastestMirrorBinding{owner, fastestmirror, "fastestmirror"};

    OptionChild<OptionString> proxy{masterConfig.proxy()};
    OptionBinding proxyBinding{owner, proxy, "proxy"};

    OptionChild<OptionString> proxy_username{masterConfig.proxy_username()};
    OptionBinding proxyUsernameBinding{owner, proxy_username, "proxy_username"};

    OptionChild<OptionString> proxy_password{masterConfig.proxy_password()};
    OptionBinding proxyPasswordBinding{owner, proxy_password, "proxy_password"};

    OptionChild<OptionEnum<std::string> > proxy_auth_method{masterConfig.proxy_auth_method()};
    OptionBinding proxyAuthMethodBinding{owner, proxy_auth_method, "proxy_auth_method"};

    OptionChild<OptionString> username{masterConfig.username()};
    OptionBinding usernameBinding{owner, username, "username"};

    OptionChild<OptionString> password{masterConfig.password()};
    OptionBinding passwordBinding{owner, password, "password"};

    OptionChild<OptionStringList> protected_packages{masterConfig.protected_packages()};
    OptionBinding protectedPackagesBinding{owner, protected_packages, "protected_packages"};

    OptionChild<OptionBool> gpgcheck{masterConfig.gpgcheck()};
    OptionBinding gpgCheckBinding{owner, gpgcheck, "gpgcheck"};

    OptionChild<OptionBool> repo_gpgcheck{masterConfig.repo_gpgcheck()};
    OptionBinding repoGpgCheckBinding{owner, repo_gpgcheck, "repo_gpgcheck"};

    OptionChild<OptionBool> enablegroups{masterConfig.enablegroups()};
    OptionBinding enableGroupsBinding{owner, enablegroups, "enablegroups"};

    OptionChild<OptionNumber<std::uint32_t> > retries{masterConfig.retries()};
    OptionBinding retriesBinding{owner, retries, "retries"};

    OptionChild<OptionNumber<std::uint32_t> > bandwidth{masterConfig.bandwidth()};
    OptionBinding bandwidthBinding{owner, bandwidth, "bandwidth"};

    OptionChild<OptionNumber<std::uint32_t> > minrate{masterConfig.minrate()};
    OptionBinding minRateBinding{owner, minrate, "minrate"};

    OptionChild<OptionEnum<std::string> > ip_resolve{masterConfig.ip_resolve()};
    OptionBinding ipResolveBinding{owner, ip_resolve, "ip_resolve"};

    OptionChild<OptionNumber<float> > throttle{masterConfig.throttle()};
    OptionBinding throttleBinding{owner, throttle, "throttle"};

    OptionChild<OptionSeconds> timeout{masterConfig.timeout()};
    OptionBinding timeoutBinding{owner, timeout, "timeout"};

    OptionChild<OptionNumber<std::uint32_t> >  max_parallel_downloads{masterConfig.max_parallel_downloads()};
    OptionBinding maxParallelDownloadsBinding{owner, max_parallel_downloads, "max_parallel_downloads"};

    OptionChild<OptionSeconds> metadata_expire{masterConfig.metadata_expire()};
    OptionBinding metadataExpireBinding{owner, metadata_expire, "metadata_expire"};

    OptionNumber<std::int32_t> cost{1000};
    OptionBinding costBinding{owner, cost, "cost"};

    OptionNumber<std::int32_t> priority{99};
    OptionBinding priorityBinding{owner, priority, "priority"};

    OptionBool module_hotfixes{false};
    OptionBinding moduleHotfixesBinding{owner, module_hotfixes, "module_hotfixes"};

    OptionChild<OptionString> sslcacert{masterConfig.sslcacert()};
    OptionBinding sslCaCertBinding{owner, sslcacert, "sslcacert"};

    OptionChild<OptionBool> sslverify{masterConfig.sslverify()};
    OptionBinding sslVerifyBinding{owner, sslverify, "sslverify"};

    OptionChild<OptionString> sslclientcert{masterConfig.sslclientcert()};
    OptionBinding sslClientCertBinding{owner, sslclientcert, "sslclientcert"};

    OptionChild<OptionString> sslclientkey{masterConfig.sslclientkey()};
    OptionBinding sslClientKeyBinding{owner, sslclientkey, "sslclientkey"};

    OptionChild<OptionBool> deltarpm{masterConfig.deltarpm()};
    OptionBinding deltaRpmBinding{owner, deltarpm, "deltarpm"};

    OptionChild<OptionNumber<std::uint32_t> > deltarpm_percentage{masterConfig.deltarpm_percentage()};
    OptionBinding deltaRpmPercentageBinding{owner, deltarpm_percentage, "deltarpm_percentage"};

    OptionBool skip_if_unavailable{true};
    OptionBinding skipIfUnavailableBinding{owner, skip_if_unavailable, "skip_if_unavailable"};

    OptionString enabled_metadata{""};
    OptionBinding enabledMetadataBinding{owner, enabled_metadata, "enabled_metadata"};

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
