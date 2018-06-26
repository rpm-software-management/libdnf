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

#define METADATA_RELATIVE_DIR "repodata"
#define PACKAGES_RELATIVE_DIR "packages"
#define METALINK_FILENAME "metalink.xml"
#define MIRRORLIST_FILENAME  "mirrorlist"
#define RECOGNIZED_CHKSUMS {"sha512", "sha256"}

#define ASCII_LOWERCASE "abcdefghijklmnopqrstuvwxyz"
#define ASCII_UPPERCASE "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define ASCII_LETTERS ASCII_LOWERCASE ASCII_UPPERCASE
#define DIGITS "0123456789"
#define REPOID_CHARS ASCII_LETTERS DIGITS "-_.:"

#include "Repo.hpp"
#include "../dnf-utils.h"
#include "../hy-iutil.h"
#include "../hy-util-private.hpp"
#include "../hy-types.h"

#include "bgettext/bgettext-lib.h"
#include "tinyformat/tinyformat.hpp"
#include <utils.hpp>

#include <librepo/librepo.h>
#include <utime.h>

#include <solv/chksum.h>
#include <solv/repo.h>

#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <sstream>

#include <errno.h>
#include <stdio.h>
#include <time.h>

#include <glib.h>

template<>
struct std::default_delete<GError> {
    void operator()(GError * ptr) noexcept { g_error_free(ptr); }
};

template<>
struct std::default_delete<LrHandle> {
    void operator()(LrHandle * ptr) noexcept { lr_handle_free(ptr); }
};

template<>
struct std::default_delete<LrResult> {
    void operator()(LrResult * ptr) noexcept { lr_result_free(ptr); }
};

namespace libdnf {

static void throwException(std::unique_ptr<GError> && err)
{
    switch (err->code) {
        case LRE_IO:
            throw LrIO(err->message);
        case LRE_CANNOTCREATEDIR:
            throw LrCannotCreateDir(err->message);
        case LRE_CANNOTCREATETMP:
            throw LrCannotCreateTmp(err->message);
        case LRE_MEMORY:
            throw LrMemory(err->message);
        case LRE_BADFUNCARG:
            throw LrBadFuncArg(err->message);
        case LRE_BADOPTARG:
            throw LrBadOptArg(err->message);
        default:
            throw LrException(err->message);
    }
}

template<typename T>
inline static void handleSetOpt(LrHandle * handle, LrHandleOption option, T value) {
    GError * errP{nullptr};
    if (!lr_handle_setopt(handle, &errP, option, value)) {
        throwException(std::unique_ptr<GError>(errP));
    }
}

inline static void handleGetInfo(LrHandle * handle, LrHandleInfoOption option, void * value) {
    GError * errP{nullptr};
    if (!lr_handle_getinfo(handle, &errP, option, value)) {
        throwException(std::unique_ptr<GError>(errP));
    }
}

template<typename T>
inline static void resultGetInfo(LrResult * result, LrResultInfoOption option, T value) {
    GError * errP{nullptr};
    if (!lr_result_getinfo(result, &errP, option, value)) {
        throwException(std::unique_ptr<GError>(errP));
    }
}

/* Callback stuff */

int RepoCB::progress(double totalToDownload, double downloaded) { return 0; }
void RepoCB::fastestMirror(FastestMirrorStage stage, const char * ptr) {}
int RepoCB::handleMirrorFailure(const char * msg, const char * url, const char * metadata) { return 0; }


// Map string from config option proxy_auth_method to librepo LrAuth value
static constexpr struct {
    const char * name;
    LrAuth code;
} PROXYAUTHMETHODS[] = {
    {"none", LR_AUTH_NONE},
    {"basic", LR_AUTH_BASIC},
    {"digest", LR_AUTH_DIGEST},
    {"negotiate", LR_AUTH_NEGOTIATE},
    {"ntlm", LR_AUTH_NTLM},
    {"digest_ie", LR_AUTH_DIGEST_IE},
    {"ntlm_wb", LR_AUTH_NTLM_WB},
    {"any", LR_AUTH_ANY}
};

class Repo::Impl {
public:
    Impl(const std::string & id, std::unique_ptr<ConfigRepo> && conf);
    ~Impl();

    bool load();
    bool loadCache(bool throwExcept);
    bool isInSync();
    void fetch();
    std::string getCachedir() const;
    int getAge() const;
    void expire();
    bool isExpired() const;
    int getExpiresIn() const;
    std::unique_ptr<LrHandle> lrHandleInitBase();
    std::unique_ptr<LrHandle> lrHandleInitLocal();
    std::unique_ptr<LrHandle> lrHandleInitRemote(const char *destdir, bool mirrorSetup = true);

    std::string id;
    std::unique_ptr<ConfigRepo> conf;

    char ** mirrors{nullptr};
    int maxMirrorTries{0}; // try them all
    // 0 forces expiration on the next call to load(), -1 means undefined value
    int timestamp;
    int max_timestamp;
    std::string repomd_fn;
    std::string primary_fn;
    std::string filelists_fn;
    std::string presto_fn;
    std::string updateinfo_fn;
    std::string comps_fn;
#ifdef MODULEMD
    std::string modules_fn;
#endif
    std::string revision;
    std::vector<std::string> content_tags;
    std::vector<std::pair<std::string, std::string>> distro_tags;
    unsigned char checksum[CHKSUM_BYTES];
    bool useIncludes;
    std::map<std::string, std::string> substitutions;

    std::unique_ptr<RepoCB> callbacks;
    std::string repoFilePath;
    LrHandle * getCachedHandle();

    SyncStrategy syncStrategy;
private:
    void lrHandlePerform(LrHandle * handle, LrResult * result);
    bool isMetalinkInSync();
    bool isRepomdInSync();
    void resetMetadataExpired();

    static int progressCB(void * data, double totalToDownload, double downloaded);
    static void fastestMirrorCB(void * data, LrFastestMirrorStages stage, void *ptr);
    static int mirrorFailureCB(void * data, const char * msg, const char * url, const char * metadata);

    bool expired;
    std::unique_ptr<LrHandle> handle;
};

int Repo::Impl::progressCB(void * data, double totalToDownload, double downloaded)
{
    if (!data)
        return 0;
    auto cbObject = static_cast<RepoCB *>(data);
    return cbObject->progress(totalToDownload, downloaded);
    //LB_CB_ERROR
    /*std::cout << "Progress total downloaded: " << totalToDownload << "now: " << downloaded << std::endl;
    return static_cast<int>(LR_CB_OK);*/
}

void Repo::Impl::fastestMirrorCB(void * data, LrFastestMirrorStages stage, void *ptr)
{
    if (!data)
        return;
    auto cbObject = static_cast<RepoCB *>(data);
//    std::cout << "Fastestmirror stage: " << stage << "data: " << ptr << std::endl;
    const char * msg;
    std::string msgString;
    if (ptr) {
        switch (stage) {
            case LR_FMSTAGE_CACHELOADING:
            case LR_FMSTAGE_CACHELOADINGSTATUS:
            case LR_FMSTAGE_STATUS:
                msg = static_cast<const char *>(ptr);
                break;
            case LR_FMSTAGE_DETECTION:
                msgString = std::to_string(*((long *)ptr));
                msg = msgString.c_str();
                break;
            default:
                msg = nullptr;
        }
    } else
        msg = nullptr;
    cbObject->fastestMirror(static_cast<RepoCB::FastestMirrorStage>(stage), msg);
}

int Repo::Impl::mirrorFailureCB(void * data, const char * msg, const char * url, const char * metadata)
{
    if (!data)
        return 0;
    auto cbObject = static_cast<RepoCB *>(data);
    return cbObject->handleMirrorFailure(msg, url, metadata);
/*    std::cout << "HMF msg: " << msg << "url: " << url << "metadata: "<< metadata << std::endl;
    return static_cast<int>(LR_CB_OK);*/
    //LR_CB_ERROR
};


/**
* @brief Converts the given input string to a URL encoded string
*
* All input characters that are not a-z, A-Z, 0-9, '-', '.', '_' or '~' are converted
* to their "URL escaped" version (%NN where NN is a two-digit hexadecimal number).
*
* @param src String to encode
* @return URL encoded string
*/
static std::string urlEncode(const std::string & src)
{
    auto noEncode = [](char ch)
    {
        return isalnum(ch) || ch=='-' || ch == '.' || ch == '_' || ch == '~';
    };

    // compute length of encoded string
    auto len = src.length();
    for (auto ch : src) {
        if (!noEncode(ch))
            len += 2;
    }

    // encode the input string
    std::string encoded;
    encoded.reserve(len);
    for (auto ch : src) {
        if (noEncode(ch))
            encoded.push_back(ch);
        else {
            encoded.push_back('%');
            unsigned char hex;
            hex = static_cast<unsigned char>(ch) >> 4;
            hex += hex <= 9 ? '0' : 'a' - 10;
            encoded.push_back(hex);
            hex = static_cast<unsigned char>(ch) & 0x0F;
            hex += hex <= 9 ? '0' : 'a' - 10;
            encoded.push_back(hex);
        }
    }

    return encoded;
}

/**
* @brief Format user password string
*
* Returns user and password in user:password form. If quote is True,
* special characters in user and password are URL encoded.
*
* @param user Username
* @param passwd Password
* @param encode If quote is True, special characters in user and password are URL encoded.
* @return User and password in user:password form
*/
static std::string formatUserPassString(const std::string & user, const std::string & passwd, bool encode)
{
    if (encode)
        return urlEncode(user) + ":" + urlEncode(passwd);
    else
        return user + ":" + passwd;
}

Repo::Impl::Impl(const std::string & id, std::unique_ptr<ConfigRepo> && conf)
: id(id), conf(std::move(conf)), timestamp(-1)
, syncStrategy(SyncStrategy::TRY_CACHE), expired(false) {}

Repo::Impl::~Impl()
{
    g_strfreev(mirrors);
}

Repo::Repo(const std::string & id, std::unique_ptr<ConfigRepo> && conf)
: pImpl(new Impl(id, std::move(conf))) {}

Repo::~Repo() = default;

void Repo::setCallbacks(std::unique_ptr<RepoCB> && callbacks)
{
    pImpl->callbacks = std::move(callbacks);
}

int Repo::verifyId(const std::string & id)
{
    auto idx = id.find_first_not_of(REPOID_CHARS);
    return idx == id.npos ? -1 : idx;
}

void Repo::verify() const
{
    if (pImpl->conf->baseurl().empty() &&
        (pImpl->conf->metalink().empty() || pImpl->conf->metalink().getValue().empty()) &&
        (pImpl->conf->mirrorlist().empty() || pImpl->conf->mirrorlist().getValue().empty()))
        throw std::runtime_error(tfm::format(_("Repository %s has no mirror or baseurl set."), pImpl->id));

    const auto & type = pImpl->conf->type().getValue();
    const char * supportedRepoTypes[]{"rpm-md", "rpm", "repomd", "rpmmd", "yum", "YUM"};
    if (!type.empty()) {
        for (auto supported : supportedRepoTypes) {
            if (type == supported)
                return;
        }
        throw std::runtime_error(tfm::format(_("Repository '%s' has unsupported type: 'type=%s', skipping."), pImpl->id, type));
    }
}

ConfigRepo * Repo::getConfig() noexcept
{
    return pImpl->conf.get();
}

const std::string & Repo::getId() const noexcept
{
    return pImpl->id;
}

void Repo::enable()
{
    pImpl->conf->enabled().set(Option::Priority::RUNTIME, true);
}

void Repo::disable()
{
    pImpl->conf->enabled().set(Option::Priority::RUNTIME, false);
}

bool Repo::isEnabled() const
{
    return pImpl->conf->enabled().getValue();
}

bool Repo::isLocal() const
{
    auto & conf = pImpl->conf;
    if ((!conf->metalink().empty() && !conf->metalink().getValue().empty()) ||
        (!conf->mirrorlist().empty() && !conf->mirrorlist().getValue().empty()))
        return false;
    if (!conf->baseurl().getValue().empty() && conf->baseurl().getValue()[0].compare(0, 7, "file://") == 0)
        return true;
    return false;
}

bool Repo::load() { return pImpl->load(); }
bool Repo::loadCache(bool throwExcept) { return pImpl->loadCache(throwExcept); }
bool Repo::getUseIncludes() const { return pImpl->useIncludes; }
void Repo::setUseIncludes(bool enabled) { pImpl->useIncludes = enabled; }
int Repo::getCost() const { return pImpl->conf->cost().getValue(); }
int Repo::getPriority() const { return pImpl->conf->priority().getValue(); }
std::string Repo::getCompsFn() { return pImpl->comps_fn; }

#ifdef MODULEMD
std::string Repo::getModulesFn() { return pImpl->modules_fn; }
#endif

int Repo::getAge() const { return pImpl->getAge(); }
void Repo::expire() { pImpl->expire(); }
bool Repo::isExpired() const { return pImpl->isExpired(); }
int Repo::getExpiresIn() const { return pImpl->getExpiresIn(); }

std::unique_ptr<LrHandle> Repo::Impl::lrHandleInitBase()
{
    std::unique_ptr<LrHandle> h(lr_handle_init());
#ifdef MODULEMD
    const char *dlist[] = {"primary", "filelists", "prestodelta", "group_gz",
                           "updateinfo", "modules", NULL};
#else
    const char *dlist[] = {"primary", "filelists", "prestodelta", "group_gz",
                           "updateinfo", NULL};
#endif
    handleSetOpt(h.get(), LRO_REPOTYPE, LR_YUMREPO);
    handleSetOpt(h.get(), LRO_USERAGENT, "libdnf/1.0"); //FIXME
    handleSetOpt(h.get(), LRO_YUMDLIST, dlist);
    handleSetOpt(h.get(), LRO_INTERRUPTIBLE, 1L);
    handleSetOpt(h.get(), LRO_GPGCHECK, conf->repo_gpgcheck().getValue());
    handleSetOpt(h.get(), LRO_MAXMIRRORTRIES, static_cast<long>(maxMirrorTries));
    handleSetOpt(h.get(), LRO_MAXPARALLELDOWNLOADS,
                     conf->max_parallel_downloads().getValue());

    LrUrlVars * vars = NULL;
    vars = lr_urlvars_set(vars, "group_gz", "group");
    handleSetOpt(h.get(), LRO_YUMSLIST, vars);

    return h;
}

std::unique_ptr<LrHandle> Repo::Impl::lrHandleInitLocal()
{
    std::unique_ptr<LrHandle> h(lrHandleInitBase());

    LrUrlVars * vars = NULL;
    for (const auto & item : substitutions)
        vars = lr_urlvars_set(vars, item.first.c_str(), item.second.c_str());
    handleSetOpt(h.get(), LRO_VARSUB, vars);
    auto cachedir = getCachedir();
    //std::cout << "cachedir: " << cachedir << std::endl;
    handleSetOpt(h.get(), LRO_DESTDIR, cachedir.c_str());
    const char *urls[] = {cachedir.c_str(), NULL};
    handleSetOpt(h.get(), LRO_URLS, urls);
    handleSetOpt(h.get(), LRO_LOCAL, 1L);
    return h;
}

std::unique_ptr<LrHandle> Repo::Impl::lrHandleInitRemote(const char *destdir, bool mirrorSetup)
{
    std::unique_ptr<LrHandle> h(lrHandleInitBase());

    LrUrlVars * vars = NULL;
    for (const auto & item : substitutions)
        vars = lr_urlvars_set(vars, item.first.c_str(), item.second.c_str());
    handleSetOpt(h.get(), LRO_VARSUB, vars);

    handleSetOpt(h.get(), LRO_DESTDIR, destdir);

    auto & ipResolve = conf->ip_resolve().getValue();
    if (ipResolve == "ipv4")
        handleSetOpt(h.get(), LRO_IPRESOLVE, LR_IPRESOLVE_V4);
    else if (ipResolve == "ipv6")
        handleSetOpt(h.get(), LRO_IPRESOLVE, LR_IPRESOLVE_V6);

    enum class Source {NONE, METALINK, MIRRORLIST} source{Source::NONE};
    std::string tmp;
    if (!conf->metalink().empty() && !(tmp=conf->metalink().getValue()).empty())
        source = Source::METALINK;
    else if (!conf->mirrorlist().empty() && !(tmp=conf->mirrorlist().getValue()).empty())
        source = Source::MIRRORLIST;
    if (source != Source::NONE) {
        handleSetOpt(h.get(), LRO_HMFCB, static_cast<LrHandleMirrorFailureCb>(mirrorFailureCB));
        handleSetOpt(h.get(), LRO_PROGRESSDATA, callbacks.get());
        if (mirrorSetup) {
            if (source == Source::METALINK)
                handleSetOpt(h.get(), LRO_METALINKURL, tmp.c_str());
            else {
                handleSetOpt(h.get(), LRO_MIRRORLISTURL, tmp.c_str());
                // YUM-DNF compatibility hack. YUM guessed by content of keyword "metalink" if
                // mirrorlist is really mirrorlist or metalink)
                if (tmp.find("metalink") != tmp.npos)
                    handleSetOpt(h.get(), LRO_METALINKURL, tmp.c_str());
            }
            handleSetOpt(h.get(), LRO_FASTESTMIRROR, conf->fastestmirror().getValue() ? 1L : 0L);
            auto fastestMirrorCacheDir = conf->basecachedir().getValue();
            if (fastestMirrorCacheDir.back() != '/')
                fastestMirrorCacheDir.push_back('/');
            fastestMirrorCacheDir += "fastestmirror.cache";
            handleSetOpt(h.get(), LRO_FASTESTMIRRORCACHE, fastestMirrorCacheDir.c_str());
        } else {
            // use already resolved mirror list
            handleSetOpt(h.get(), LRO_URLS, mirrors);
        }
    } else if (!conf->baseurl().getValue().empty()) {
        size_t len = conf->baseurl().getValue().size();
        const char * urls[len + 1];
        for (size_t idx = 0; idx < len; ++idx)
            urls[idx] = conf->baseurl().getValue()[idx].c_str();
        urls[len] = nullptr;
        handleSetOpt(h.get(), LRO_URLS, urls);
    } else
        throw std::runtime_error(tfm::format(_("Cannot find a valid baseurl for repo: %s"), id));

    // setup username/password if needed
    auto userpwd = conf->username().getValue();
    if (!userpwd.empty()) {
        // TODO Use URL encoded form, needs support in librepo
        userpwd = formatUserPassString(userpwd, conf->password().getValue(), false);
        handleSetOpt(h.get(), LRO_USERPWD, userpwd.c_str());
    }

    // setup ssl stuff
    if (!conf->sslcacert().getValue().empty())
        handleSetOpt(h.get(), LRO_SSLCACERT, conf->sslcacert().getValue().c_str());
    if (!conf->sslclientcert().getValue().empty())
        handleSetOpt(h.get(), LRO_SSLCLIENTCERT, conf->sslclientcert().getValue().c_str());
    if (!conf->sslclientkey().getValue().empty())
        handleSetOpt(h.get(), LRO_SSLCLIENTKEY, conf->sslclientkey().getValue().c_str());

    handleSetOpt(h.get(), LRO_PROGRESSCB, static_cast<LrProgressCb>(progressCB));
    handleSetOpt(h.get(), LRO_PROGRESSDATA, callbacks.get());
    handleSetOpt(h.get(), LRO_FASTESTMIRRORCB, static_cast<LrFastestMirrorCb>(fastestMirrorCB));
    handleSetOpt(h.get(), LRO_FASTESTMIRRORDATA, callbacks.get());

    auto minrate = conf->minrate().getValue();
    handleSetOpt(h.get(), LRO_LOWSPEEDLIMIT, static_cast<long>(minrate));

    auto maxspeed = conf->throttle().getValue();
    if (maxspeed > 0 && maxspeed <= 1)
        maxspeed *= conf->bandwidth().getValue();
    if (maxspeed != 0 && maxspeed < minrate)
        throw std::runtime_error(_("Maximum download speed is lower than minimum. "
                                   "Please change configuration of minrate or throttle"));
    handleSetOpt(h.get(), LRO_MAXSPEED, static_cast<int64_t>(maxspeed));

    long timeout = conf->timeout().getValue();
    if (timeout > 0) {
        handleSetOpt(h.get(), LRO_CONNECTTIMEOUT, timeout);
        handleSetOpt(h.get(), LRO_LOWSPEEDTIME, timeout);
    } else {
        handleSetOpt(h.get(), LRO_CONNECTTIMEOUT, LRO_CONNECTTIMEOUT_DEFAULT);
        handleSetOpt(h.get(), LRO_LOWSPEEDTIME, LRO_LOWSPEEDTIME_DEFAULT);
    }

    if (!conf->proxy().empty() && !conf->proxy().getValue().empty())
        handleSetOpt(h.get(), LRO_PROXY, conf->proxy().getValue().c_str());

    //set proxy autorization method
    auto proxyAuthMethodStr = conf->proxy_auth_method().getValue();
    auto proxyAuthMethod = LR_AUTH_ANY;
    for (auto & auth : PROXYAUTHMETHODS) {
        if (proxyAuthMethodStr == auth.name) {
            proxyAuthMethod = auth.code;
            break;
        }
    }
    handleSetOpt(h.get(), LRO_PROXYAUTHMETHODS, static_cast<long>(proxyAuthMethod));

    if (!conf->proxy_username().empty()) {
        userpwd = conf->proxy_username().getValue();
        if (!userpwd.empty()) {
            userpwd = formatUserPassString(userpwd, conf->proxy_password().getValue(), true);
            handleSetOpt(h.get(), LRO_PROXYUSERPWD, userpwd.c_str());
        }
    }

    auto sslverify = conf->sslverify().getValue() ? 1L : 0L;
    handleSetOpt(h.get(), LRO_SSLVERIFYHOST, sslverify);
    handleSetOpt(h.get(), LRO_SSLVERIFYPEER, sslverify);

    return h;
}

void Repo::Impl::lrHandlePerform(LrHandle * handle, LrResult * result)
{
    if (callbacks)
        callbacks->start(
            !conf->name().getValue().empty() ? conf->name().getValue().c_str() :
            (!id.empty() ? id.c_str() : "unknown")
        );

    GError * errP{nullptr};
    bool ret = lr_handle_perform(handle, result, &errP);
    std::unique_ptr<GError> err(errP);

    if (callbacks)
        callbacks->end();

    if (!ret)
        throwException(std::move(err));
}

bool Repo::Impl::loadCache(bool throwExcept)
{
    std::unique_ptr<LrHandle> h(lrHandleInitLocal());
    std::unique_ptr<LrResult> r(lr_result_init());

    // Fetch data
    GError * errP{nullptr};
    bool ret = lr_handle_perform(h.get(), r.get(), &errP);
    std::unique_ptr<GError> err(errP);
    if (!ret) {
        if (throwExcept)
            throwException(std::move(err));
        return false;
    }

    char **mirrors;
    LrYumRepo *yum_repo;
    LrYumRepoMd *yum_repomd;
    handleGetInfo(h.get(), LRI_MIRRORS, &mirrors);
    resultGetInfo(r.get(), LRR_YUM_REPO, &yum_repo);
    resultGetInfo(r.get(), LRR_YUM_REPOMD, &yum_repomd);

    // Populate repo
    repomd_fn = yum_repo->repomd;
    primary_fn = lr_yum_repo_path(yum_repo, "primary");
    filelists_fn = lr_yum_repo_path(yum_repo, "filelists");
    auto tmp = lr_yum_repo_path(yum_repo, "prestodelta");
    presto_fn = tmp ? tmp : "";
    tmp = lr_yum_repo_path(yum_repo, "updateinfo");
    updateinfo_fn = tmp ? tmp : "";
    tmp = lr_yum_repo_path(yum_repo, "group_gz");
    if (!tmp)
        tmp = lr_yum_repo_path(yum_repo, "group");
    comps_fn = tmp ? tmp : "";
#ifdef MODULEMD
    tmp = lr_yum_repo_path(yum_repo, "modules");
    modules_fn = tmp ? tmp : "";
#endif

    content_tags.clear();
    for (auto elem = yum_repomd->content_tags; elem; elem = g_slist_next(elem)) {
        if (elem->data)
            content_tags.emplace_back(static_cast<const char *>(elem->data));
    }

    distro_tags.clear();
    for (auto elem = yum_repomd->distro_tags; elem; elem = g_slist_next(elem)) {
        if (elem->data) {
            auto distroTag = static_cast<LrYumDistroTag *>(elem->data);
            if (distroTag->tag)
                distro_tags.emplace_back(distroTag->cpeid, distroTag->tag);
        }
    }

    revision = yum_repomd->revision;
    max_timestamp = lr_yum_repomd_get_highest_timestamp(yum_repomd, NULL);

    // Load timestamp unless explicitly expired
    if (timestamp != 0) {
        timestamp = mtime(primary_fn.c_str());
    }
    g_strfreev(this->mirrors);
    this->mirrors = mirrors;
    //std::cout << "load(): " << (mirrors ? mirrors[0] : nullptr) << std::endl;
    return true;
}

// Use metalink to check whether our metadata are still current.
bool Repo::Impl::isMetalinkInSync()
{
    char tmpdir[] = "/tmp/tmpdir.XXXXXX";
    mkdtemp(tmpdir);

    std::unique_ptr<LrHandle> h(lrHandleInitRemote(tmpdir));
    std::unique_ptr<LrResult> r(lr_result_init());

    handleSetOpt(h.get(), LRO_FETCHMIRRORS, 1L);
    lrHandlePerform(h.get(), r.get());
    LrMetalink * metalink;
    handleGetInfo(h.get(), LRI_METALINK, &metalink);
    if (!metalink) {
        //logger.debug(_("reviving: repo '%s' skipped, no metalink."), self.id)
        dnf_remove_recursive(tmpdir, NULL);
        return false;
    }

    // check all recognized hashes
    auto chksumFree = [](Chksum * ptr){solv_chksum_free(ptr, nullptr);};
    struct hashInfo {
        const LrMetalinkHash * lrMetalinkHash;
        std::unique_ptr<Chksum, decltype(chksumFree)> chksum;
    };
    std::vector<hashInfo> hashes;
    for (auto hash = metalink->hashes; hash; hash = hash->next) {
        auto lrMetalinkHash = static_cast<const LrMetalinkHash *>(hash->data);
        for (auto algorithm : RECOGNIZED_CHKSUMS) {
            if (strcmp(lrMetalinkHash->type, algorithm) == 0)
                hashes.push_back({lrMetalinkHash, {nullptr, chksumFree}});
        }
    }
    if (hashes.empty()) {
        //logger.debug(_("reviving: repo '%s' skipped, no usable hash."), self.id);
        dnf_remove_recursive(tmpdir, NULL);
        return false;
    }

    for (auto & hash : hashes) {
        auto chkType = solv_chksum_str2type(hash.lrMetalinkHash->type);
        hash.chksum.reset(solv_chksum_create(chkType));
    }

    std::ifstream repomd(repomd_fn, std::ifstream::binary);
    char buf[4096];
    int readed;
    while ((readed = repomd.readsome(buf, sizeof(buf))) > 0) {
        for (auto & hash : hashes)
            solv_chksum_add(hash.chksum.get(), buf, readed);
    }

    for (auto & hash : hashes) {
        int chksumLen;
        auto chksum = solv_chksum_get(hash.chksum.get(), &chksumLen);
        char chksumHex[chksumLen * 2 + 1];
        solv_bin2hex(chksum, chksumLen, chksumHex);
        if (strcmp(chksumHex, hash.lrMetalinkHash->value) != 0) {
            //logger.debug(_("reviving: failed for '%s', mismatched %s sum."), self.id, algo)
            dnf_remove_recursive(tmpdir, NULL);
            return false;
        }
    }

    dnf_remove_recursive(tmpdir, NULL);
    //logger.debug(_("reviving: '%s' can be revived - metalink checksums match."), self.id)
    return true;
}

// Use repomd to check whether our metadata are still current.
bool Repo::Impl::isRepomdInSync()
{
    LrYumRepo *yum_repo;
    char tmpdir[] = "/tmp/tmpdir.XXXXXX";
    mkdtemp(tmpdir);
    const char *dlist[] = LR_YUM_REPOMDONLY;

    std::unique_ptr<LrHandle> h(lrHandleInitRemote(tmpdir));
    std::unique_ptr<LrResult> r(lr_result_init());

    handleSetOpt(h.get(), LRO_YUMDLIST, dlist);
    lrHandlePerform(h.get(), r.get());
    resultGetInfo(r.get(), LRR_YUM_REPO, &yum_repo);

    auto same = compareFiles(repomd_fn.c_str(), yum_repo->repomd) == 0;
    dnf_remove_recursive(tmpdir, NULL);
    /*if (same)
        logger.debug(_("reviving: '%s' can be revived - repomd matches."), self.id)
    else
        logger.debug(_("reviving: failed for '%s', mismatched repomd."), self.id)*/
    return same;
}

bool Repo::Impl::isInSync()
{
    if (!conf->metalink().empty() && !conf->metalink().getValue().empty())
        return isMetalinkInSync();
    return isRepomdInSync();
}

void Repo::Impl::fetch()
{
    const auto cacheDir = getCachedir();
    auto repodir = cacheDir + "/repodata";
    if (g_mkdir_with_parents(cacheDir.c_str(), 0755) == -1) {
        const char * errTxt = strerror(errno);
        throw std::runtime_error(tfm::format(_("Cannot create repo cache directory \"%s\": %s"), cacheDir, errTxt));
    }
    auto tmpdir = cacheDir + "/tmpdir.XXXXXX";
    if (!mkdtemp(&tmpdir.front())) {
        const char * errTxt = strerror(errno);
        throw std::runtime_error(tfm::format(_("Cannot create repo temporary directory \"%s\": %s"), tmpdir.c_str(), errTxt));
    }
    auto tmprepodir = tmpdir + "/repodata";

    std::unique_ptr<LrHandle> h(lrHandleInitRemote(tmpdir.c_str()));
    std::unique_ptr<LrResult> r(lr_result_init());
    lrHandlePerform(h.get(), r.get());

    dnf_remove_recursive(repodir.c_str(), NULL);
    if (g_mkdir_with_parents(repodir.c_str(), 0755) == -1) {
        const char * errTxt = strerror(errno);
        throw std::runtime_error(tfm::format(_("Cannot create directory \"%s\": %s"), repodir, errTxt));
    }
    if (rename(tmprepodir.c_str(), repodir.c_str()) == -1) {
        const char * errTxt = strerror(errno);
        throw std::runtime_error(tfm::format(_("Cannot rename directory \"%s\" to \"%s\": %s"), tmprepodir, repodir, errTxt));
    }
    dnf_remove_recursive(tmpdir.c_str(), NULL);

    timestamp = -1;
}

bool Repo::Impl::load()
{
    //printf("check if cache present\n");
    if (!primary_fn.empty() || loadCache(false)) {
        if (conf->getMasterConfig().check_config_file_age().getValue() &&
            !repoFilePath.empty() && mtime(repoFilePath.c_str()) > mtime(primary_fn.c_str()))
            expired = true;
        if (!expired || syncStrategy == SyncStrategy::ONLY_CACHE || syncStrategy == SyncStrategy::LAZY) {
            //logger.debug(_('repo: using cache for: %s'), self.id)
            return false;
        }

        if (isInSync()) {
            //printf("reusing expired cache\n");
            // the expired metadata still reflect the origin:
            utimes(primary_fn.c_str(), NULL);
            expired = false;
            return true;
        }
    }
    if (syncStrategy == SyncStrategy::ONLY_CACHE) {
        //_("Cache-only enabled but no cache for '%s'") % self.id
        auto msg = tfm::format(_("Cache-only enabled but no cache for '%s'"), id);
        throw std::runtime_error(msg);
    }

    //printf("fetch\n");
    try {
        fetch();
        loadCache(true);
    } catch (const std::runtime_error & e) {
        //dmsg = _("Cannot download '%s': %s.")
        //logger.log(dnf.logging.DEBUG, dmsg, e.source_url, e.librepo_msg)
        //log(debug, e.what());
        auto msg = tfm::format(_("Failed to synchronize cache for repo '%s'"), id);
        throw std::runtime_error(msg);
    }
    expired = false;
    return true;
}

std::string Repo::Impl::getCachedir() const
{
    std::string tmp;
    if (conf->metalink().empty() || (tmp=conf->metalink().getValue()).empty()) {
        if (conf->mirrorlist().empty() || (tmp=conf->mirrorlist().getValue()).empty()) {
            if (!conf->baseurl().getValue().empty())
                tmp = conf->baseurl().getValue()[0];
            if (tmp.empty())
                tmp = id;
        }
    }

    auto chksumObj = solv_chksum_create(REPOKEY_TYPE_SHA256);
    solv_chksum_add(chksumObj, tmp.c_str(), tmp.length());
    int chksumLen;
    auto chksum = solv_chksum_get(chksumObj, &chksumLen);
    static constexpr int USE_CHECKSUM_BYTES = 8;
    if (chksumLen < USE_CHECKSUM_BYTES) {
        solv_chksum_free(chksumObj, nullptr);
        throw std::runtime_error(_("getCachedir(): Computation of SHA256 failed"));
    }
    char chksumCStr[USE_CHECKSUM_BYTES * 2 + 1];
    solv_bin2hex(chksum, USE_CHECKSUM_BYTES, chksumCStr);
    solv_chksum_free(chksumObj, nullptr);

    auto repodir(conf->basecachedir().getValue());
    if (repodir.back() != '/')
        repodir.push_back('/');
    return repodir + id + "-" + chksumCStr;
}

int Repo::Impl::getAge() const
{
    return time(NULL) - mtime(primary_fn.c_str());
}

void Repo::Impl::expire()
{
    expired = true;
    timestamp = 0;
}

bool Repo::Impl::isExpired() const
{
    if (expired)
        // explicitly requested expired state
        return true;
    if (conf->metadata_expire().getValue() == -1)
        return false;
    return getAge() > conf->metadata_expire().getValue();
}

int Repo::Impl::getExpiresIn() const
{
    return conf->metadata_expire().getValue() - getAge();
}

bool Repo::fresh()
{
    return pImpl->timestamp >= 0;
}

void Repo::Impl::resetMetadataExpired()
{
    if (expired)
        // explicitly requested expired state
        return;
    if (conf->metadata_expire().getValue() == -1)
        expired = false;
    else
        expired = getAge() > conf->metadata_expire().getValue();
}


/* Returns a librepo handle, set as per the repo options
   Note that destdir is None, and the handle is cached.*/
LrHandle * Repo::Impl::getCachedHandle()
{
    if (!handle)
        handle = lrHandleInitRemote(nullptr);
    return handle.get();
}

void Repo::setMaxMirrorTries(int maxMirrorTries)
{
    pImpl->maxMirrorTries = maxMirrorTries;
}

int Repo::getTimestamp() const
{
    return pImpl->timestamp;
}

int Repo::getMaxTimestamp()
{
    return pImpl->max_timestamp;
}

const std::vector<std::string> & Repo::getContentTags()
{
    return pImpl->content_tags;
}

const std::vector<std::pair<std::string, std::string>> & Repo::getDistroTags()
{
    return pImpl->distro_tags;
}

const std::string & Repo::getRevision() const
{
    return pImpl->revision;
}

void Repo::initHyRepo(HyRepo hrepo)
{
    hy_repo_set_string(hrepo, HY_REPO_MD_FN, pImpl->repomd_fn.c_str());
    hy_repo_set_string(hrepo, HY_REPO_PRIMARY_FN, pImpl->primary_fn.c_str());
    hy_repo_set_string(hrepo, HY_REPO_FILELISTS_FN, pImpl->filelists_fn.c_str());
#ifdef MODULEMD
    hy_repo_set_string(hrepo, MODULES_FN, pImpl->modules_fn.c_str());
#endif
    hy_repo_set_cost(hrepo, pImpl->conf->cost().getValue());
    hy_repo_set_priority(hrepo, pImpl->conf->priority().getValue());
    // TODO finish
    if (!pImpl->presto_fn.empty())
        hy_repo_set_string(hrepo, HY_REPO_PRESTO_FN, pImpl->presto_fn.c_str());
    if (!pImpl->updateinfo_fn.empty())
        hy_repo_set_string(hrepo, HY_REPO_UPDATEINFO_FN, pImpl->updateinfo_fn.c_str());

}

std::string Repo::getCachedir() const
{
    return pImpl->getCachedir();
}

void Repo::setRepoFilePath(const std::string & path)
{
    pImpl->repoFilePath = path;
}

const std::string & Repo::getRepoFilePath() const noexcept
{
    return pImpl->repoFilePath;
}

void Repo::setSyncStrategy(SyncStrategy strategy)
{
    pImpl->syncStrategy = strategy;
}

Repo::SyncStrategy Repo::getSyncStrategy() const noexcept
{
    return pImpl->syncStrategy;
}

void Repo::downloadUrl(const char * url, int fd)
{
    if (pImpl->callbacks)
        pImpl->callbacks->start(
            !pImpl->conf->name().getValue().empty() ? pImpl->conf->name().getValue().c_str() :
            (!pImpl->id.empty() ? pImpl->id.c_str() : "unknown")
        );

    GError * errP{nullptr};
    auto ret = lr_download_url(pImpl->getCachedHandle(), url, fd, &errP);
    std::unique_ptr<GError> err(errP);

    if (pImpl->callbacks)
        pImpl->callbacks->end();

    assert((ret && !errP) || (!ret && errP));

    if (err)
        throwException(std::move(err));
}

std::vector<std::string> Repo::getMirrors() const
{
    std::vector<std::string> mirrors;
    if (pImpl->mirrors) {
        for (auto mirror = pImpl->mirrors; *mirror; ++mirror)
            mirrors.emplace_back(*mirror);
    }
    return mirrors;
}


int PackageTargetCB::end(TransferStatus status, const char * msg) { return 0; }
int PackageTargetCB::progress(double totalToDownload, double downloaded) { return 0; }
int PackageTargetCB::mirrorFailure(const char *msg, const char *url) { return 0; }

class PackageTarget::Impl {
public:
    Impl(Repo * repo, const char * relativeUrl, const char * dest, int chksType, const char * chksum, int64_t expectedSize, const char * baseUrl, bool resume, int64_t byteRangeStart, int64_t byteRangeEnd, PackageTargetCB * callbacks);

    Impl(ConfigMain * cfg, const char * relativeUrl, const char * dest, int chksType, const char * chksum, int64_t expectedSize, const char * baseUrl, bool resume, int64_t byteRangeStart, int64_t byteRangeEnd, PackageTargetCB * callbacks);

    void download();

    ~Impl();

    PackageTargetCB * callbacks;

    std::unique_ptr<LrPackageTarget, decltype(&lr_packagetarget_free)> lrPkgTarget{nullptr, &lr_packagetarget_free};

private:
    void init(LrHandle * handle, const char * relativeUrl, const char * dest, int chksType, const char * chksum, int64_t expectedSize, const char * baseUrl, bool resume, int64_t byteRangeStart, int64_t byteRangeEnd);
//    LrHandle * newHandle(ConfigMain * conf);

    static int endCB(void * data, LrTransferStatus status, const char * msg);
    static int progressCB(void * data, double totalToDownload, double downloaded);
    static int mirrorFailureCB(void * data, const char * msg, const char * url);

    std::unique_ptr<LrHandle> lrHandle;

};


int PackageTarget::Impl::endCB(void * data, LrTransferStatus status, const char * msg)
{
    if (!data)
        return 0;
    auto cbObject = static_cast<PackageTargetCB *>(data);
    return cbObject->end(static_cast<PackageTargetCB::TransferStatus>(status), msg);
}

int PackageTarget::Impl::progressCB(void * data, double totalToDownload, double downloaded)
{
    if (!data)
        return 0;
    auto cbObject = static_cast<PackageTargetCB *>(data);
    return cbObject->progress(totalToDownload, downloaded);
}

int PackageTarget::Impl::mirrorFailureCB(void * data, const char * msg, const char * url)
{
    if (!data)
        return 0;
    auto cbObject = static_cast<PackageTargetCB *>(data);
    return cbObject->mirrorFailure(msg, url);
}


static LrHandle * newHandle(ConfigMain * conf)
{
    LrHandle *h = lr_handle_init();
    handleSetOpt(h, LRO_USERAGENT, "libdnf/1.0"); //FIXME
    // see dnf.repo.Repo._handle_new_remote() how to pass
    if (conf) {
        auto minrate = conf->minrate().getValue();
        handleSetOpt(h, LRO_LOWSPEEDLIMIT, static_cast<long>(minrate));

        auto maxspeed = conf->throttle().getValue();
        if (maxspeed > 0 && maxspeed <= 1)
            maxspeed *= conf->bandwidth().getValue();
        if (maxspeed != 0 && maxspeed < minrate)
            throw std::runtime_error(_("Maximum download speed is lower than minimum. "
                                       "Please change configuration of minrate or throttle"));
        handleSetOpt(h, LRO_MAXSPEED, static_cast<int64_t>(maxspeed));

        if (!conf->proxy().empty() && !conf->proxy().getValue().empty())
            handleSetOpt(h, LRO_PROXY, conf->proxy().getValue().c_str());

        //set proxy autorization method
        auto proxyAuthMethodStr = conf->proxy_auth_method().getValue();
        auto proxyAuthMethod = LR_AUTH_ANY;
        for (auto & auth : PROXYAUTHMETHODS) {
            if (proxyAuthMethodStr == auth.name) {
                proxyAuthMethod = auth.code;
                break;
            }
        }
        handleSetOpt(h, LRO_PROXYAUTHMETHODS, static_cast<long>(proxyAuthMethod));

        if (!conf->proxy_username().empty()) {
            auto userpwd = conf->proxy_username().getValue();
            if (!userpwd.empty()) {
                userpwd = formatUserPassString(userpwd, conf->proxy_password().getValue(), true);
                handleSetOpt(h, LRO_PROXYUSERPWD, userpwd.c_str());
            }
        }

        auto sslverify = conf->sslverify().getValue() ? 1L : 0L;
        handleSetOpt(h, LRO_SSLVERIFYHOST, sslverify);
        handleSetOpt(h, LRO_SSLVERIFYPEER, sslverify);
    }
    return h;
}

PackageTarget::ChecksumType PackageTarget::checksumType(const std::string & name)
{
    return static_cast<ChecksumType>(lr_checksum_type(name.c_str()));
}

void PackageTarget::downloadPackages(std::vector<PackageTarget *> & targets, bool failFast)
{
    // Convert vector to GSList
    GSList * list{nullptr};
    for (auto it = targets.rbegin(); it != targets.rend(); ++it)
        list = g_slist_prepend(list, (*it)->pImpl->lrPkgTarget.get());
    std::unique_ptr<GSList, decltype(&g_slist_free)> listGuard(list, &g_slist_free);

    LrPackageDownloadFlag flags = static_cast<LrPackageDownloadFlag>(0);
    if (failFast)
        flags = static_cast<LrPackageDownloadFlag>(flags | LR_PACKAGEDOWNLOAD_FAILFAST);

    GError * errP{nullptr};
    lr_download_packages(list, flags, &errP);
    std::unique_ptr<GError> err(errP);

    if (err)
        throwException(std::move(err));
}


PackageTarget::Impl::~Impl() {}

PackageTarget::Impl::Impl(Repo * repo, const char * relativeUrl, const char * dest, int chksType, const char * chksum, int64_t expectedSize, const char * baseUrl, bool resume, int64_t byteRangeStart, int64_t byteRangeEnd, PackageTargetCB * callbacks)
: callbacks(callbacks)
{
    init(repo->pImpl->getCachedHandle(), relativeUrl, dest, chksType, chksum, expectedSize, baseUrl, resume, byteRangeStart, byteRangeEnd);
}

PackageTarget::Impl::Impl(ConfigMain * cfg, const char * relativeUrl, const char * dest, int chksType, const char * chksum, int64_t expectedSize, const char * baseUrl, bool resume, int64_t byteRangeStart, int64_t byteRangeEnd, PackageTargetCB * callbacks)
: callbacks(callbacks)
{
    lrHandle.reset(newHandle(cfg));
    handleSetOpt(lrHandle.get(), LRO_REPOTYPE, LR_YUMREPO);
    init(lrHandle.get(), relativeUrl, dest, chksType, chksum, expectedSize, baseUrl, resume, byteRangeStart, byteRangeEnd);
}

void PackageTarget::Impl::init(LrHandle * handle, const char * relativeUrl, const char * dest, int chksType, const char * chksum, int64_t expectedSize, const char * baseUrl, bool resume, int64_t byteRangeStart, int64_t byteRangeEnd)
{
    LrChecksumType lrChksType = static_cast<LrChecksumType>(chksType);

    if (resume && byteRangeStart) {
        auto msg = _("resume cannot be used simultaneously with the byterangestart param");
        throw std::runtime_error(msg);
    }

    GError * errP{nullptr};
    lrPkgTarget.reset(lr_packagetarget_new_v3(handle, relativeUrl, dest, lrChksType, chksum,  expectedSize, baseUrl, resume, progressCB, callbacks, endCB, mirrorFailureCB, byteRangeStart, byteRangeEnd, &errP));
    std::unique_ptr<GError> err(errP);

    if (!lrPkgTarget) {
        auto msg = tfm::format(_("PackageTarget initialization failed: %s"), err->message);
        throw std::runtime_error(msg);
    }
}

PackageTarget::PackageTarget(Repo * repo, const char * relativeUrl, const char * dest, int chksType, const char * chksum, int64_t expectedSize, const char * baseUrl, bool resume, int64_t byteRangeStart, int64_t byteRangeEnd, PackageTargetCB * callbacks)
: pImpl(new Impl(repo, relativeUrl, dest, chksType, chksum, expectedSize, baseUrl, resume, byteRangeStart, byteRangeEnd, callbacks)) {}

PackageTarget::PackageTarget(ConfigMain * cfg, const char * relativeUrl, const char * dest, int chksType, const char * chksum, int64_t expectedSize, const char * baseUrl, bool resume, int64_t byteRangeStart, int64_t byteRangeEnd, PackageTargetCB * callbacks)
: pImpl(new Impl(cfg, relativeUrl, dest, chksType, chksum, expectedSize, baseUrl, resume, byteRangeStart, byteRangeEnd, callbacks)) {}


PackageTarget::~PackageTarget() {}

PackageTargetCB * PackageTarget::getCallbacks()
{
    return pImpl->callbacks;
}

const char * PackageTarget::getErr()
{
    return pImpl->lrPkgTarget->err;
}

void Downloader::downloadURL(ConfigMain * cfg, const char * url, int fd)
{
    std::unique_ptr<LrHandle> lrHandle(newHandle(cfg));
    GError * errP{nullptr};
    lr_download_url(lrHandle.get(), url, fd, &errP);
    std::unique_ptr<GError> err(errP);

    if (err)
        throwException(std::move(err));
}

// ============ librepo logging ===========

#define LR_LOGDOMAIN "librepo"

class LrHandleLogData {
public:
    std::string filePath;
    long uid;
    FILE *fd;
    bool used{false};
    guint handlerId;

    ~LrHandleLogData();
};

LrHandleLogData::~LrHandleLogData()
{
    if (used)
        g_log_remove_handler(LR_LOGDOMAIN, handlerId);
    fclose(fd);
}

static std::list<std::unique_ptr<LrHandleLogData>> lrLogDatas;
static std::mutex lrLogDatasMutex;

static const char * lrLogLevelFlagToCStr(GLogLevelFlags logLevelFlag)
{
    if (logLevelFlag & G_LOG_LEVEL_ERROR)
        return "ERROR";
    if (logLevelFlag & G_LOG_LEVEL_CRITICAL)
        return "CRITICAL";
    if (logLevelFlag & G_LOG_LEVEL_WARNING)
        return "WARNING";
    if (logLevelFlag & G_LOG_LEVEL_MESSAGE)
        return "MESSAGE";
    if (logLevelFlag & G_LOG_LEVEL_INFO)
        return "INFO";
    if (logLevelFlag & G_LOG_LEVEL_DEBUG)
        return "DEBUG";
    return "USER";
}

static void librepoLogCB(G_GNUC_UNUSED const gchar *log_domain, GLogLevelFlags log_level, const char *msg, gpointer user_data) noexcept
{
    // Ignore exception during logging. Eg. exception generated during logging of exception is not good.
    try {
        auto data = static_cast<LrHandleLogData *>(user_data);
        auto now = time(NULL);
        struct tm nowTm;
        gmtime_r(&now, &nowTm);

        std::ostringstream ss;
        ss << std::setfill('0');
        ss << std::setw(4) << nowTm.tm_year+1900 << "-" << std::setw(2) << nowTm.tm_mon+1 << "-" << std::setw(2) << nowTm.tm_mday;
        ss << "T" << std::setw(2) << nowTm.tm_hour << ":" << std::setw(2) << nowTm.tm_min << ":" << std::setw(2) << nowTm.tm_sec << "Z ";
        ss << lrLogLevelFlagToCStr(log_level) << " " << msg << std::endl;
        auto str = ss.str();
        fwrite(str.c_str(), sizeof(char), str.length(), data->fd);
        fflush(data->fd);
    } catch (const std::exception &) {
    }
}

long LibrepoLog::addHandler(const std::string & filePath)
{
    static long uid = 0;

    // Open the file
    FILE *fd = fopen(filePath.c_str(), "a");
    if (!fd)
        throw std::runtime_error(tfm::format(_("Cannot open %s: %s"), filePath, g_strerror(errno)));

    // Setup user data
    std::unique_ptr<LrHandleLogData> data(new LrHandleLogData);
    data->filePath = filePath;
    data->fd = fd;

    // Set handler
    data->handlerId = g_log_set_handler(LR_LOGDOMAIN, G_LOG_LEVEL_MASK, librepoLogCB, data.get());
    data->used = true;

    // Save user data (in a thread safe way)
    {
        std::lock_guard<std::mutex> guard(lrLogDatasMutex);

        // Get unique ID of the handler
        data->uid = ++uid;

        // Append the data to the global list
        lrLogDatas.push_front(std::move(data));
    }

    // Log librepo version and current time (including timezone)
    lr_log_librepo_summary();

    // Return unique id of the handler data
    return uid;
}

void LibrepoLog::removeHandler(long uid)
{
    std::lock_guard<std::mutex> guard(lrLogDatasMutex);

    // Search for the corresponding LogFileData
    auto it = lrLogDatas.begin();
    for (; it != lrLogDatas.end() && (*it)->uid != uid; ++it);
    if (it == lrLogDatas.end())
        throw std::runtime_error(tfm::format(_("Log handler with id %ld doesn't exist"), uid));

    // Remove the handler and free the data
    lrLogDatas.erase(it);
}

void LibrepoLog::removeAllHandlers()
{
    std::lock_guard<std::mutex> guard(lrLogDatasMutex);
    lrLogDatas.clear();
}

}
