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

#include <map>
#include <cctype>
#include <glib.h>

#include <fstream>
#include <iostream>

namespace libdnf {

/* Callback stuff */

int RepoCB::progress(double totalToDownload, double downloaded) { return 0; }
void RepoCB::fastestMirror(int stage, const char * ptr) {}
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
    bool loadCache();
    bool isInSync();
    void fetch();
    std::string getCachedir();
    int getAge() const;
    void expire();
    bool isExpired() const;
    int getExpiresIn() const;
    LrHandle * lrHandleInitBase();
    LrHandle * lrHandleInitLocal();
    LrHandle * lrHandleInitRemote(const char *destdir, bool mirrorSetup = true);

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
    bool lrHandlePerform(LrHandle * handle, LrResult * result);
    bool isMetalinkInSync();
    bool isRepomdInSync();
    void resetMetadataExpired();

    static int progressCB(void * data, double totalToDownload, double downloaded);
    static void fastestMirrorCB(void * data, LrFastestMirrorStages stage, void *ptr);
    static int mirrorFailureCB(void * data, const char * msg, const char * url, const char * metadata);

    bool expired;
    std::unique_ptr<LrHandle, decltype(&lr_handle_free)> handle{nullptr, &lr_handle_free};
//    std::unique_ptr<LrHandle, decltype(&lr_handle_free)> handle2;
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
    cbObject->fastestMirror(stage, msg);
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

bool Repo::load() { return pImpl->load(); }
bool Repo::loadCache() { return pImpl->loadCache(); }
bool Repo::getUseIncludes() const { return pImpl->useIncludes; }
void Repo::setUseIncludes(bool enabled) { pImpl->useIncludes = enabled; }
int Repo::getCost() const { return pImpl->conf->cost().getValue(); }
int Repo::getPriority() const { return pImpl->conf->priority().getValue(); }
std::string Repo::getCompsFn() { return pImpl->comps_fn; }
int Repo::getAge() const { return pImpl->getAge(); }
void Repo::expire() { pImpl->expire(); }
bool Repo::isExpired() const { return pImpl->isExpired(); }
int Repo::getExpiresIn() const { return pImpl->getExpiresIn(); }

LrHandle * Repo::Impl::lrHandleInitBase()
{
    LrHandle *h = lr_handle_init();
    const char *dlist[] = {"primary", "filelists", "prestodelta", "group_gz",
                           "updateinfo", NULL};
    lr_handle_setopt(h, NULL, LRO_REPOTYPE, LR_YUMREPO);
    lr_handle_setopt(h, NULL, LRO_USERAGENT, "libdnf/1.0"); //FIXME
    lr_handle_setopt(h, NULL, LRO_YUMDLIST, dlist);
    lr_handle_setopt(h, NULL, LRO_INTERRUPTIBLE, 1L);
    lr_handle_setopt(h, NULL, LRO_GPGCHECK, conf->repo_gpgcheck().getValue());
    lr_handle_setopt(h, NULL, LRO_MAXMIRRORTRIES, static_cast<long>(maxMirrorTries));
    lr_handle_setopt(h, NULL, LRO_MAXPARALLELDOWNLOADS,
                     conf->max_parallel_downloads().getValue());

    LrUrlVars * vars = NULL;
    vars = lr_urlvars_set(vars, "group_gz", "group");
    lr_handle_setopt(h, NULL, LRO_YUMSLIST, vars);

    return h;
}

LrHandle * Repo::Impl::lrHandleInitLocal()
{
    LrHandle *h = lrHandleInitBase();

    LrUrlVars * vars = NULL;
    for (const auto & item : substitutions)
        vars = lr_urlvars_set(vars, item.first.c_str(), item.second.c_str());
    lr_handle_setopt(h, NULL, LRO_VARSUB, vars);
    auto cachedir = getCachedir();
    //std::cout << "cachedir: " << cachedir << std::endl;
    lr_handle_setopt(h, NULL, LRO_DESTDIR, cachedir.c_str());
    const char *urls[] = {cachedir.c_str(), NULL};
    lr_handle_setopt(h, NULL, LRO_URLS, urls);
    lr_handle_setopt(h, NULL, LRO_LOCAL, 1L);
    return h;
}

LrHandle *
Repo::Impl::lrHandleInitRemote(const char *destdir, bool mirrorSetup)
{
    LrHandle *h = lrHandleInitBase();

    LrUrlVars * vars = NULL;
    for (const auto & item : substitutions)
        vars = lr_urlvars_set(vars, item.first.c_str(), item.second.c_str());
    lr_handle_setopt(h, NULL, LRO_VARSUB, vars);

    lr_handle_setopt(h, NULL, LRO_DESTDIR, destdir);

    auto & ipResolve = conf->ip_resolve().getValue();
    if (ipResolve == "ipv4")
        lr_handle_setopt(h, NULL, LRO_IPRESOLVE, LR_IPRESOLVE_V4);
    else if (ipResolve == "ipv6")
        lr_handle_setopt(h, NULL, LRO_IPRESOLVE, LR_IPRESOLVE_V6);

    enum class Source {NONE, METALINK, MIRRORLIST} source{Source::NONE};
    std::string tmp;
    if (!conf->metalink().empty() && !(tmp=conf->metalink().getValue()).empty())
        source = Source::METALINK;
    else if (!conf->mirrorlist().empty() && !(tmp=conf->mirrorlist().getValue()).empty())
        source = Source::MIRRORLIST;
    if (source != Source::NONE) {
        lr_handle_setopt(h, nullptr, LRO_HMFCB, static_cast<LrHandleMirrorFailureCb>(mirrorFailureCB));
        lr_handle_setopt(h, nullptr, LRO_PROGRESSDATA, callbacks.get());
        if (mirrorSetup) {
            if (source == Source::METALINK)
                lr_handle_setopt(h, nullptr, LRO_METALINKURL, tmp.c_str());
            else {
                lr_handle_setopt(h, nullptr, LRO_MIRRORLISTURL, tmp.c_str());
                // YUM-DNF compatibility hack. YUM guessed by content of keyword "metalink" if
                // mirrorlist is really mirrorlist or metalink)
                if (tmp.find("metalink") != tmp.npos)
                    lr_handle_setopt(h, nullptr, LRO_METALINKURL, tmp.c_str());
            }
            lr_handle_setopt(h, nullptr, LRO_FASTESTMIRROR, conf->fastestmirror().getValue() ? 1L : 0L);
            auto fastestMirrorCacheDir = conf->basecachedir().getValue();
            if (fastestMirrorCacheDir.back() != '/')
                fastestMirrorCacheDir.push_back('/');
            fastestMirrorCacheDir += "fastestmirror.cache";
            lr_handle_setopt(h, nullptr, LRO_FASTESTMIRRORCACHE, fastestMirrorCacheDir.c_str());
        } else {
            // use already resolved mirror list
            lr_handle_setopt(h, nullptr, LRO_URLS, mirrors);
        }
    } else if (!conf->baseurl().getValue().empty()) {
        size_t len = conf->baseurl().getValue().size();
        const char * urls[len + 1];
        for (size_t idx = 0; idx < len; ++idx)
            urls[idx] = conf->baseurl().getValue()[idx].c_str();
        urls[len] = nullptr;
        lr_handle_setopt(h, nullptr, LRO_URLS, urls);
    } else
        throw std::runtime_error(tfm::format(_("Cannot find a valid baseurl for repo: %s"), id));

    // setup username/password if needed
    auto userpwd = conf->username().getValue();
    if (!userpwd.empty()) {
        // TODO Use URL encoded form, needs support in librepo
        userpwd = formatUserPassString(userpwd, conf->password().getValue(), false);
        lr_handle_setopt(h, nullptr, LRO_USERPWD, userpwd.c_str());
    }

    // setup ssl stuff
    if (!conf->sslcacert().getValue().empty())
        lr_handle_setopt(h, nullptr, LRO_SSLCACERT, conf->sslcacert().getValue().c_str());
    if (!conf->sslclientcert().getValue().empty())
        lr_handle_setopt(h, nullptr, LRO_SSLCLIENTCERT, conf->sslclientcert().getValue().c_str());
    if (!conf->sslclientkey().getValue().empty())
        lr_handle_setopt(h, nullptr, LRO_SSLCLIENTKEY, conf->sslclientkey().getValue().c_str());

    lr_handle_setopt(h, nullptr, LRO_PROGRESSCB, static_cast<LrProgressCb>(progressCB));
    lr_handle_setopt(h, nullptr, LRO_PROGRESSDATA, callbacks.get());
    lr_handle_setopt(h, nullptr, LRO_FASTESTMIRRORCB, static_cast<LrFastestMirrorCb>(fastestMirrorCB));
    lr_handle_setopt(h, nullptr, LRO_FASTESTMIRRORDATA, callbacks.get());

    auto minrate = conf->minrate().getValue();
    lr_handle_setopt(h, nullptr, LRO_LOWSPEEDLIMIT, static_cast<long>(minrate));

    auto maxspeed = conf->throttle().getValue();
    if (maxspeed > 0 && maxspeed <= 1)
        maxspeed *= conf->bandwidth().getValue();
    if (maxspeed != 0 && maxspeed < minrate)
        throw std::runtime_error(_("Maximum download speed is lower than minimum. "
                                   "Please change configuration of minrate or throttle"));
    lr_handle_setopt(h, nullptr, LRO_MAXSPEED, static_cast<int64_t>(maxspeed));

    long timeout = conf->timeout().getValue();
    if (timeout > 0) {
        lr_handle_setopt(h, nullptr, LRO_CONNECTTIMEOUT, timeout);
        lr_handle_setopt(h, nullptr, LRO_LOWSPEEDTIME, timeout);
    } else {
        lr_handle_setopt(h, nullptr, LRO_CONNECTTIMEOUT, LRO_CONNECTTIMEOUT_DEFAULT);
        lr_handle_setopt(h, nullptr, LRO_LOWSPEEDTIME, LRO_LOWSPEEDTIME_DEFAULT);
    }

    if (!conf->proxy().empty() && !conf->proxy().getValue().empty())
        lr_handle_setopt(h, nullptr, LRO_PROXY, conf->proxy().getValue().c_str());

    //set proxy autorization method
    auto proxyAuthMethodStr = conf->proxy_auth_method().getValue();
    auto proxyAuthMethod = LR_AUTH_ANY;
    for (auto & auth : PROXYAUTHMETHODS) {
        if (proxyAuthMethodStr == auth.name) {
            proxyAuthMethod = auth.code;
            break;
        }
    }
    lr_handle_setopt(h, nullptr, LRO_PROXYAUTHMETHODS, static_cast<long>(proxyAuthMethod));

    if (!conf->proxy_username().empty()) {
        userpwd = conf->proxy_username().getValue();
        if (!userpwd.empty()) {
            userpwd = formatUserPassString(userpwd, conf->proxy_password().getValue(), true);
            lr_handle_setopt(h, nullptr, LRO_PROXYUSERPWD, userpwd.c_str());
        }
    }

    auto sslverify = conf->sslverify().getValue() ? 1L : 0L;
    lr_handle_setopt(h, nullptr, LRO_SSLVERIFYHOST, sslverify);
    lr_handle_setopt(h, nullptr, LRO_SSLVERIFYPEER, sslverify);

    return h;
}

bool Repo::Impl::lrHandlePerform(LrHandle * handle, LrResult * result)
{
    GError *err = NULL;

    if (callbacks)
        callbacks->start(
            !conf->name().getValue().empty() ? conf->name().getValue().c_str() :
            (!id.empty() ? id.c_str() : "unknown")
        );
    lr_handle_perform(handle, result, &err);
    if (callbacks)
        callbacks->end();
    if (err) {
        g_error_free(err);
        return false;
    }
    return true;
}

bool Repo::Impl::loadCache()
{
    std::unique_ptr<LrHandle, decltype(&lr_handle_free)> h(lrHandleInitLocal(), &lr_handle_free);
    std::unique_ptr<LrResult, decltype(&lr_result_free)> r(lr_result_init(), &lr_result_free);

    // Fetch data
    GError *err = NULL;
    lr_handle_perform(h.get(), r.get(), &err);
    if (err) {
        return false;
    }

    char **mirrors;
    LrYumRepo *yum_repo;
    LrYumRepoMd *yum_repomd;
    lr_handle_getinfo(h.get(), NULL, LRI_MIRRORS, &mirrors);
    lr_result_getinfo(r.get(), NULL, LRR_YUM_REPO, &yum_repo);
    lr_result_getinfo(r.get(), NULL, LRR_YUM_REPOMD, &yum_repomd);

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

    std::unique_ptr<LrHandle, decltype(&lr_handle_free)> h(lrHandleInitRemote(tmpdir),
                                                           &lr_handle_free);
    std::unique_ptr<LrResult, decltype(&lr_result_free)> r(lr_result_init(),
                                                           &lr_result_free);

    lr_handle_setopt(h.get(), NULL, LRO_FETCHMIRRORS, 1L);
    lrHandlePerform(h.get(), r.get());
    LrMetalink * metalink;
    lr_handle_getinfo(h.get(), NULL, LRI_METALINK, &metalink);
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

    std::unique_ptr<LrHandle, decltype(&lr_handle_free)> h(lrHandleInitRemote(tmpdir),
                                                           &lr_handle_free);
    std::unique_ptr<LrResult, decltype(&lr_result_free)> r(lr_result_init(),
                                                           &lr_result_free);

    lr_handle_setopt(h.get(), NULL, LRO_YUMDLIST, dlist);
    lrHandlePerform(h.get(), r.get());
    lr_result_getinfo(r.get(), NULL, LRR_YUM_REPO, &yum_repo);

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
    char tmpdir[] = "/var/tmp/tmpdir.XXXXXX";
    mkdtemp(tmpdir);
    std::string tmprepodir = tmpdir + std::string("/repodata");
    std::string repodir = getCachedir() + "/repodata";

    std::unique_ptr<LrHandle, decltype(&lr_handle_free)> h(lrHandleInitRemote(tmpdir),
                                                           &lr_handle_free);
    std::unique_ptr<LrResult, decltype(&lr_result_free)> r(lr_result_init(),
                                                           &lr_result_free);
    lrHandlePerform(h.get(), r.get());

    dnf_remove_recursive(repodir.c_str(), NULL);
    g_mkdir_with_parents(repodir.c_str(), 0);
    rename(tmprepodir.c_str(), repodir.c_str());
    dnf_remove_recursive(tmpdir, NULL);

    timestamp = -1;
}

bool Repo::Impl::load()
{
    //printf("check if cache present\n");
    if (loadCache()) {
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
        auto msg = tfm::format(_("Cache-only enabled but no cache for %s"), id);
        throw std::runtime_error(msg);
    }

    //printf("fetch\n");
    fetch();
    loadCache();
    return true;
}

std::string Repo::Impl::getCachedir()
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
    return expired;
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
        expired = time(NULL) - mtime(primary_fn.c_str()) > conf->metadata_expire().getValue();
}


/* Returns a librepo handle, set as per the repo options
   Note that destdir is None, and the handle is cached.*/
LrHandle * Repo::Impl::getCachedHandle()
{
    if (!handle)
        handle.reset(lrHandleInitRemote(nullptr));
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
    hy_repo_set_cost(hrepo, pImpl->conf->cost().getValue());
    hy_repo_set_priority(hrepo, pImpl->conf->priority().getValue());
    // TODO finish
    if (!pImpl->presto_fn.empty())
        hy_repo_set_string(hrepo, HY_REPO_PRESTO_FN, pImpl->presto_fn.c_str());
    if (!pImpl->updateinfo_fn.empty())
        hy_repo_set_string(hrepo, HY_REPO_UPDATEINFO_FN, pImpl->updateinfo_fn.c_str());

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
    GError *err = NULL;
    if (pImpl->callbacks)
        pImpl->callbacks->start(
            !pImpl->conf->name().getValue().empty() ? pImpl->conf->name().getValue().c_str() :
            (!pImpl->id.empty() ? pImpl->id.c_str() : "unknown")
        );
    auto ret = lr_download_url(pImpl->getCachedHandle(), url, fd, &err);
    if (pImpl->callbacks)
        pImpl->callbacks->end();

    std::string msg;
    if (err) {
        msg = std::string(err->message) + "  " + std::to_string(err->code);
        g_error_free(err);
    }
    if ((!ret && !err) || (ret && err))
        throw std::runtime_error("Error in lr_download_url");
    if (!ret) {
        throw std::runtime_error(msg);
    }
}

std::vector<std::string> Repo::getMirrors() const
{
    std::vector<std::string> mirrors;
    if (pImpl->mirrors) {
        for (auto mirror = pImpl->mirrors; *mirror; ++mirror)
            mirrors.emplace_back(*mirror);
    }
    //std::cout << "getMirrors(): " << mirrors.size() << std::endl;
    return mirrors;
}

int PackageTargetCB::end(int status, const char * msg) { return 0; }
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

    std::unique_ptr<LrHandle, decltype(&lr_handle_free)> lrHandle{nullptr, &lr_handle_free};

};


int PackageTarget::Impl::endCB(void * data, LrTransferStatus status, const char * msg)
{
    if (!data)
        return 0;
    auto cbObject = static_cast<PackageTargetCB *>(data);
    return cbObject->end(status, msg);
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
    lr_handle_setopt(h, NULL, LRO_USERAGENT, "libdnf/1.0"); //FIXME
    // see dnf.repo.Repo._handle_new_remote() how to pass
    if (conf) {
        auto minrate = conf->minrate().getValue();
        lr_handle_setopt(h, nullptr, LRO_LOWSPEEDLIMIT, static_cast<long>(minrate));

        auto maxspeed = conf->throttle().getValue();
        if (maxspeed > 0 && maxspeed <= 1)
            maxspeed *= conf->bandwidth().getValue();
        if (maxspeed != 0 && maxspeed < minrate)
            throw std::runtime_error(_("Maximum download speed is lower than minimum. "
                                       "Please change configuration of minrate or throttle"));
        lr_handle_setopt(h, nullptr, LRO_MAXSPEED, static_cast<int64_t>(maxspeed));

        if (!conf->proxy().empty() && !conf->proxy().getValue().empty())
            lr_handle_setopt(h, nullptr, LRO_PROXY, conf->proxy().getValue().c_str());

        //set proxy autorization method
        auto proxyAuthMethodStr = conf->proxy_auth_method().getValue();
        auto proxyAuthMethod = LR_AUTH_ANY;
        for (auto & auth : PROXYAUTHMETHODS) {
            if (proxyAuthMethodStr == auth.name) {
                proxyAuthMethod = auth.code;
                break;
            }
        }
        lr_handle_setopt(h, nullptr, LRO_PROXYAUTHMETHODS, static_cast<long>(proxyAuthMethod));

        if (!conf->proxy_username().empty()) {
            auto userpwd = conf->proxy_username().getValue();
            if (!userpwd.empty()) {
                userpwd = formatUserPassString(userpwd, conf->proxy_password().getValue(), true);
                lr_handle_setopt(h, nullptr, LRO_PROXYUSERPWD, userpwd.c_str());
            }
        }

        auto sslverify = conf->sslverify().getValue() ? 1L : 0L;
        lr_handle_setopt(h, nullptr, LRO_SSLVERIFYHOST, sslverify);
        lr_handle_setopt(h, nullptr, LRO_SSLVERIFYPEER, sslverify);
    }
    return h;
}

void PackageTarget::downloadPackages(std::vector<PackageTarget *> & targets, bool failFast)
{
    // Convert vector to GSList
    GSList *list = nullptr;
    for (auto target : targets)
        list = g_slist_append(list, target->pImpl->lrPkgTarget.get());

    LrPackageDownloadFlag flags = static_cast<LrPackageDownloadFlag>(0);
    if (failFast)
        flags = static_cast<LrPackageDownloadFlag>(flags | LR_PACKAGEDOWNLOAD_FAILFAST);

    GError * err = nullptr;
    auto ret = lr_download_packages(list, flags, &err);

    g_slist_free(list);

    std::string msg;
    if (err) {
        msg = std::string(err->message) + "  " + std::to_string(err->code);
        g_error_free(err);
    }
    if ((!ret && !err) || (ret && err))
        throw std::runtime_error("Error in lr_download_packages");
    if (!ret) {
        throw std::runtime_error(msg);
    }
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
    lr_handle_setopt(lrHandle.get(), NULL, LRO_REPOTYPE, LR_YUMREPO);
    init(lrHandle.get(), relativeUrl, dest, chksType, chksum, expectedSize, baseUrl, resume, byteRangeStart, byteRangeEnd);
}

void PackageTarget::Impl::init(LrHandle * handle, const char * relativeUrl, const char * dest, int chksType, const char * chksum, int64_t expectedSize, const char * baseUrl, bool resume, int64_t byteRangeStart, int64_t byteRangeEnd)
{
    LrChecksumType lrChksType = static_cast<LrChecksumType>(chksType);

    if (resume && byteRangeStart) {
        auto msg = _("resume cannot be used simultaneously with the byterangestart param");
        throw std::runtime_error(msg);
    }

    GError * err = NULL;
    lrPkgTarget.reset(lr_packagetarget_new_v3(handle, relativeUrl, dest, lrChksType, chksum,  expectedSize, baseUrl, resume, progressCB, callbacks, endCB, mirrorFailureCB, byteRangeStart, byteRangeEnd, &err));
    if (!lrPkgTarget) {
        auto msg = tfm::format(_("PackageTarget initialization failed: %s"), err->message);
        g_error_free(err);
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
    std::unique_ptr<LrHandle, decltype(&lr_handle_free)> lrHandle{newHandle(cfg), &lr_handle_free};
    GError * err = NULL;
    auto ret = lr_download_url(lrHandle.get(), url, fd, &err);
    std::string msg;
    if (err) {
        msg = std::string(err->message) + "  " + std::to_string(err->code);
        g_error_free(err);
    }
    if ((!ret && !err) || (ret && err))
        throw std::runtime_error("Error in lr_download_packages");
    if (!ret) {
        throw std::runtime_error(msg);
    }
}

}
