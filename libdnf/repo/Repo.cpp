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

#define GPG_HOME_ENV "GNUPGHOME"

#define ASCII_LOWERCASE "abcdefghijklmnopqrstuvwxyz"
#define ASCII_UPPERCASE "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
#define ASCII_LETTERS ASCII_LOWERCASE ASCII_UPPERCASE
#define DIGITS "0123456789"
#define REPOID_CHARS ASCII_LETTERS DIGITS "-_.:"

#include "../log.hpp"
#include "Repo.hpp"
#include "../dnf-utils.h"
#include "../hy-iutil.h"
#include "../hy-util-private.hpp"
#include "../hy-types.h"

#include "bgettext/bgettext-lib.h"
#include "tinyformat/tinyformat.hpp"
#include <utils.hpp>

#include <librepo/librepo.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>

#include <gpgme.h>

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
#include <type_traits>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <glib.h>

namespace std {

template<>
struct default_delete<GError> {
    void operator()(GError * ptr) noexcept { g_error_free(ptr); }
};

template<>
struct default_delete<LrHandle> {
    void operator()(LrHandle * ptr) noexcept { lr_handle_free(ptr); }
};

template<>
struct default_delete<LrResult> {
    void operator()(LrResult * ptr) noexcept { lr_result_free(ptr); }
};

template<>
struct default_delete<LrPackageTarget> {
    void operator()(LrPackageTarget * ptr) noexcept { lr_packagetarget_free(ptr); }
};

template<>
struct default_delete<std::remove_pointer<gpgme_ctx_t>::type> {
    void operator()(gpgme_ctx_t ptr) noexcept { gpgme_release(ptr); }
};

} // namespace std

namespace libdnf {

class LrExceptionWithSourceUrl : public LrException {
public:
    LrExceptionWithSourceUrl(int code, const std::string & msg, const std::string & sourceUrl)
        : LrException(code, msg), sourceUrl(sourceUrl) {}
    const std::string & getSourceUrl() const { return sourceUrl; }
private:
    std::string sourceUrl;
};

static void throwException(std::unique_ptr<GError> && err)
{
    throw LrException(err->code, err->message);
}

template<typename T>
inline static void handleSetOpt(LrHandle * handle, LrHandleOption option, T value)
{
    GError * errP{nullptr};
    if (!lr_handle_setopt(handle, &errP, option, value)) {
        throwException(std::unique_ptr<GError>(errP));
    }
}

inline static void handleGetInfo(LrHandle * handle, LrHandleInfoOption option, void * value)
{
    GError * errP{nullptr};
    if (!lr_handle_getinfo(handle, &errP, option, value)) {
        throwException(std::unique_ptr<GError>(errP));
    }
}

template<typename T>
inline static void resultGetInfo(LrResult * result, LrResultInfoOption option, T value)
{
    GError * errP{nullptr};
    if (!lr_result_getinfo(result, &errP, option, value)) {
        throwException(std::unique_ptr<GError>(errP));
    }
}

/* Callback stuff */

int RepoCB::progress(double totalToDownload, double downloaded) { return 0; }
void RepoCB::fastestMirror(FastestMirrorStage stage, const char * ptr) {}
int RepoCB::handleMirrorFailure(const char * msg, const char * url, const char * metadata) { return 0; }

bool RepoCB::repokeyImport(const std::string & id, const std::string & userId,
                           const std::string & fingerprint, const std::string & url, long int timestamp)
{
    return true;
}


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

class Key {
public:
    Key(gpgme_key_t key, gpgme_subkey_t subkey)
    {
        id = subkey->keyid;
        fingerprint = subkey->fpr;
        timestamp = subkey->timestamp;
        userid = key->uids->uid;
    }

    std::string getId() const { return id; }
    std::string getUserId() const { return userid; }
    std::string getFingerprint() const { return fingerprint; }
    long int getTimestamp() const { return timestamp; }

    std::vector<char> rawKey;
    std::string url;

private:
    std::string id;
    std::string fingerprint;
    std::string userid;
    long int timestamp;
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
    void downloadUrl(const char * url, int fd);

    std::unique_ptr<LrHandle> lrHandleInitBase();
    std::unique_ptr<LrHandle> lrHandleInitLocal();
    std::unique_ptr<LrHandle> lrHandleInitRemote(const char *destdir, bool mirrorSetup = true);

    std::string id;
    std::unique_ptr<ConfigRepo> conf;

    char ** mirrors{nullptr};
    int maxMirrorTries{0}; // try them all
    // 0 forces expiration on the next call to load(), -1 means undefined value
    int timestamp;
    int maxTimestamp;
    std::string repomdFn;
    std::string primaryFn;
    std::string filelistsFn;
    std::string prestoFn;
    std::string updateinfoFn;
    std::string compsFn;
#ifdef MODULEMD
    std::string modulesFn;
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
    std::unique_ptr<LrResult> lrHandlePerform(LrHandle * handle, const std::string & destDirectory,
        bool setGPGHomeEnv);
    bool isMetalinkInSync();
    bool isRepomdInSync();
    void resetMetadataExpired();
    std::vector<Key> retrieve(const std::string & url);
    void importRepoKeys();

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
}

void Repo::Impl::fastestMirrorCB(void * data, LrFastestMirrorStages stage, void *ptr)
{
    if (!data)
        return;
    auto cbObject = static_cast<RepoCB *>(data);
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
{
    auto idx = verifyId(id);
    if (idx >= 0) {
        std::string msg = tfm::format(_("Bad id for repo: %s, byte = %s %d"), id, id[idx], idx);
        throw std::runtime_error(msg);
    }
    pImpl.reset(new Impl(id, std::move(conf)));
}

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
        throw std::runtime_error(tfm::format(_("Repository '%s' has unsupported type: 'type=%s', skipping."),
                                             pImpl->id, type));
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
std::string Repo::getCompsFn() { return pImpl->compsFn; }

#ifdef MODULEMD
std::string Repo::getModulesFn() { return pImpl->modulesFn; }
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

static void gpgImportKey(gpgme_ctx_t context, int keyFd)
{
    auto logger(Log::getLogger());
    gpg_error_t gpgErr;
    gpgme_data_t keyData;

    gpgErr = gpgme_data_new_from_fd(&keyData, keyFd);
    if (gpgErr != GPG_ERR_NO_ERROR) {
        auto msg = tfm::format(_("%s: gpgme_data_new_from_fd(): %s"), __func__, gpgme_strerror(gpgErr));
        logger->debug(msg);
        throw LrException(LRE_GPGERROR, msg);
    }

    gpgErr = gpgme_op_import(context, keyData);
    gpgme_data_release(keyData);
    if (gpgErr != GPG_ERR_NO_ERROR) {
        auto msg = tfm::format(_("%s: gpgme_op_import(): %s"), __func__, gpgme_strerror(gpgErr));
        logger->debug(msg);
        throw LrException(LRE_GPGERROR, msg);
    }
}

static void gpgImportKey(gpgme_ctx_t context, std::vector<char> key)
{
    auto logger(Log::getLogger());
    gpg_error_t gpgErr;
    gpgme_data_t keyData;

    gpgErr = gpgme_data_new_from_mem(&keyData, key.data(), key.size(), 0);
    if (gpgErr != GPG_ERR_NO_ERROR) {
        auto msg = tfm::format(_("%s: gpgme_data_new_from_fd(): %s"), __func__, gpgme_strerror(gpgErr));
        logger->debug(msg);
        throw LrException(LRE_GPGERROR, msg);
    }

    gpgErr = gpgme_op_import(context, keyData);
    gpgme_data_release(keyData);
    if (gpgErr != GPG_ERR_NO_ERROR) {
        auto msg = tfm::format(_("%s: gpgme_op_import(): %s"), __func__, gpgme_strerror(gpgErr));
        logger->debug(msg);
        throw LrException(LRE_GPGERROR, msg);
    }
}

static std::vector<Key> rawkey2infos(int fd) {
    auto logger(Log::getLogger());
    gpg_error_t gpgErr;

    std::vector<Key> keyInfos;
    gpgme_ctx_t ctx;
    gpgme_new(&ctx);
    std::unique_ptr<std::remove_pointer<gpgme_ctx_t>::type> context(ctx);

    // set GPG home dir
    char tmpdir[] = "/tmp/tmpdir.XXXXXX";
    mkdtemp(tmpdir);
    Finalizer tmpDirRemover([&tmpdir](){
        dnf_remove_recursive(tmpdir, NULL);
    });
    gpgErr = gpgme_ctx_set_engine_info(ctx, GPGME_PROTOCOL_OpenPGP, NULL, tmpdir);
    if (gpgErr != GPG_ERR_NO_ERROR) {
        auto msg = tfm::format(_("%s: gpgme_ctx_set_engine_info(): %s"), __func__, gpgme_strerror(gpgErr));
        logger->debug(msg);
        throw LrException(LRE_GPGERROR, msg);
    }

    gpgImportKey(ctx, fd);

    gpgme_key_t key;
    gpgErr = gpgme_op_keylist_start(ctx, NULL, 0);
    while (!gpgErr)
    {
        gpgErr = gpgme_op_keylist_next(ctx, &key);
        if (gpgErr)
            break;

        // _extract_signing_subkey
        auto subkey = key->subkeys;
        while (subkey && !key->subkeys->can_sign) {
            subkey = subkey->next;
        }
        if (subkey)
            keyInfos.emplace_back(key, subkey);
        gpgme_key_release(key);
    }
    if (gpg_err_code(gpgErr) != GPG_ERR_EOF)
    {
        gpgme_op_keylist_end(ctx);
        auto msg = tfm::format(_("can not list keys: %s"), gpgme_strerror(gpgErr));
        logger->debug(msg);
        throw LrException(LRE_GPGERROR, msg);
    }
    gpgme_set_armor(ctx, 1);
    for (auto & keyInfo : keyInfos) {
        gpgme_data_t sink;
        gpgme_data_new(&sink);
        gpgme_op_export(ctx, keyInfo.getId().c_str(), 0, sink);
        gpgme_data_rewind(sink);

        char buf[4096];
        ssize_t readed;
        do {
            readed = gpgme_data_read(sink, buf, sizeof(buf));
            if (readed > 0)
                keyInfo.rawKey.insert(keyInfo.rawKey.end(), buf, buf + sizeof(buf));
        } while (readed == sizeof(buf));
    }
    return keyInfos;
}

static std::vector<std::string> keyidsFromPubring(const std::string & gpgDir)
{
    auto logger(Log::getLogger());
    gpg_error_t gpgErr;

    std::vector<std::string> keyids;

    struct stat sb;
    if (stat(gpgDir.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {

        gpgme_ctx_t ctx;
        gpgme_new(&ctx);
        std::unique_ptr<std::remove_pointer<gpgme_ctx_t>::type> context(ctx);

        // set GPG home dir
        gpgErr = gpgme_ctx_set_engine_info(ctx, GPGME_PROTOCOL_OpenPGP, NULL, gpgDir.c_str());
        if (gpgErr != GPG_ERR_NO_ERROR) {
            auto msg = tfm::format(_("%s: gpgme_ctx_set_engine_info(): %s"), __func__, gpgme_strerror(gpgErr));
            logger->debug(msg);
            throw LrException(LRE_GPGERROR, msg);
        }

        gpgme_key_t key;
        gpgErr = gpgme_op_keylist_start(ctx, NULL, 0);
        while (!gpgErr)
        {
            gpgErr = gpgme_op_keylist_next(ctx, &key);
            if (gpgErr)
                break;

            // _extract_signing_subkey
            auto subkey = key->subkeys;
            while (subkey && !key->subkeys->can_sign) {
                subkey = subkey->next;
            }
            if (subkey)
                keyids.push_back(subkey->keyid);
            gpgme_key_release(key);
        }
        if (gpg_err_code(gpgErr) != GPG_ERR_EOF)
        {
            gpgme_op_keylist_end(ctx);
            auto msg = tfm::format(_("can not list keys: %s"), gpgme_strerror(gpgErr));
            logger->debug(msg);
            throw LrException(LRE_GPGERROR, msg);
        }
    }
    return keyids;
}

// download key from URL
std::vector<Key> Repo::Impl::retrieve(const std::string & url)
{
    auto logger(Log::getLogger());
    char tmpKeyFile[] = "/tmp/repokey.XXXXXX";
    auto fd = mkstemp(tmpKeyFile);
    if (fd == -1) {
        char buf[1024];
        (void)strerror_r(errno, buf, sizeof(buf));
        auto msg = tfm::format("%s: mkstemp(): %s", __func__, buf);
        logger->debug(msg);
        throw LrException(LRE_GPGERROR, msg);
    }
    unlink(tmpKeyFile);
    Finalizer tmpFileCloser([fd](){
        close(fd);
    });

    downloadUrl(url.c_str(), fd);
    lseek(fd, SEEK_SET, 0);
    auto keyInfos = rawkey2infos(fd);
    for (auto & key : keyInfos)
        key.url = url;
    return keyInfos;
}

void Repo::Impl::importRepoKeys()
{
    auto logger(Log::getLogger());

    auto gpgDir = getCachedir() + "/pubring";
    auto knownKeys = keyidsFromPubring(gpgDir);
    for (const auto & gpgkeyUrl : conf->gpgkey().getValue()) {
        auto keyInfos = retrieve(gpgkeyUrl);
        for (auto & keyInfo : keyInfos) {
            if (std::find(knownKeys.begin(), knownKeys.end(), keyInfo.getId()) != knownKeys.end()) {
                logger->debug(tfm::format(_("repo %s: 0x%s already imported"), id, keyInfo.getId()));
                continue;
            }

            if (callbacks) {
                if (!callbacks->repokeyImport(keyInfo.getId(), keyInfo.getUserId(), keyInfo.getFingerprint(),
                                              keyInfo.url, keyInfo.getTimestamp()))
                    continue;
            }

            struct stat sb;
            if (stat(gpgDir.c_str(), &sb) != 0 || !S_ISDIR(sb.st_mode))
                mkdir(gpgDir.c_str(), 0777);
            auto confFd = open((gpgDir + "/gpg.conf").c_str(),
                               O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            if (confFd != -1)
                close(confFd);

            gpgme_ctx_t ctx;
            gpgme_new(&ctx);
            std::unique_ptr<std::remove_pointer<gpgme_ctx_t>::type> context(ctx);

            // set GPG home dir
            auto gpgErr = gpgme_ctx_set_engine_info(ctx, GPGME_PROTOCOL_OpenPGP, NULL, gpgDir.c_str());
            if (gpgErr != GPG_ERR_NO_ERROR) {
                auto msg = tfm::format(_("%s: gpgme_ctx_set_engine_info(): %s"), __func__, gpgme_strerror(gpgErr));
                logger->debug(msg);
                throw LrException(LRE_GPGERROR, msg);
            }

            gpgImportKey(ctx, keyInfo.rawKey);

            logger->debug(tfm::format(_("repo %s: imported key 0x%s."), id, keyInfo.getId()));
        }

    }
}

std::unique_ptr<LrResult> Repo::Impl::lrHandlePerform(LrHandle * handle, const std::string & destDirectory,
    bool setGPGHomeEnv)
{
    bool isOrigGPGHomeEnvSet;
    std::string origGPGHomeEnv;
    if (setGPGHomeEnv) {
            const char * orig = getenv(GPG_HOME_ENV);
            isOrigGPGHomeEnvSet = orig;
            if (isOrigGPGHomeEnvSet)
                origGPGHomeEnv = orig;
            auto pubringdir = getCachedir() + "/pubring";
            setenv(GPG_HOME_ENV, pubringdir.c_str(), 1);
    }
    Finalizer gpgHomeEnvRecover([setGPGHomeEnv, isOrigGPGHomeEnvSet, &origGPGHomeEnv](){
        if (setGPGHomeEnv) {
            if (isOrigGPGHomeEnvSet)
                setenv(GPG_HOME_ENV, origGPGHomeEnv.c_str(), 1);
            else
                unsetenv(GPG_HOME_ENV);
        }
    });

    // Start and end is called only if progress callback is set in handle.
    LrProgressCb progressFunc;
    handleGetInfo(handle, LRI_PROGRESSCB, &progressFunc);

    std::unique_ptr<LrResult> result;
    bool ret;
    bool badGPG = false;
    do {
        if (callbacks && progressFunc)
            callbacks->start(
                !conf->name().getValue().empty() ? conf->name().getValue().c_str() :
                (!id.empty() ? id.c_str() : "unknown")
            );

        GError * errP{nullptr};
        result.reset(lr_result_init());
        ret = lr_handle_perform(handle, result.get(), &errP);
        std::unique_ptr<GError> err(errP);

        if (callbacks && progressFunc)
            callbacks->end();

        if (ret || badGPG || errP->code != LRE_BADGPG) {
            if (!ret) {
                std::string source;
                if (conf->metalink().empty() || (source=conf->metalink().getValue()).empty()) {
                    if (conf->mirrorlist().empty() || (source=conf->mirrorlist().getValue()).empty()) {
                        bool first = true;
                        for (const auto & url : conf->baseurl().getValue()) {
                            if (first)
                                first = false;
                            else
                                source += ", ";
                            source += url;
                        }
                    }
                }
                throw LrExceptionWithSourceUrl(err->code, err->message, source);
            }
            break;
        }
        badGPG = true;
        importRepoKeys();
        dnf_remove_recursive((destDirectory + "/repodata").c_str(), NULL);
    } while (true);

    return result;
}

bool Repo::Impl::loadCache(bool throwExcept)
{
    std::unique_ptr<LrHandle> h(lrHandleInitLocal());
    std::unique_ptr<LrResult> r;

    // Fetch data
    try {
        r = lrHandlePerform(h.get(), getCachedir(), conf->repo_gpgcheck().getValue());
    } catch (std::exception & ex) {
        if (throwExcept)
            throw;
        return false;
    }

    char **mirrors;
    LrYumRepo *yum_repo;
    LrYumRepoMd *yum_repomd;
    handleGetInfo(h.get(), LRI_MIRRORS, &mirrors);
    resultGetInfo(r.get(), LRR_YUM_REPO, &yum_repo);
    resultGetInfo(r.get(), LRR_YUM_REPOMD, &yum_repomd);

    // Populate repo
    repomdFn = yum_repo->repomd;
    primaryFn = lr_yum_repo_path(yum_repo, "primary");
    filelistsFn = lr_yum_repo_path(yum_repo, "filelists");
    auto tmp = lr_yum_repo_path(yum_repo, "prestodelta");
    prestoFn = tmp ? tmp : "";
    tmp = lr_yum_repo_path(yum_repo, "updateinfo");
    updateinfoFn = tmp ? tmp : "";
    tmp = lr_yum_repo_path(yum_repo, "group_gz");
    if (!tmp)
        tmp = lr_yum_repo_path(yum_repo, "group");
    compsFn = tmp ? tmp : "";
#ifdef MODULEMD
    tmp = lr_yum_repo_path(yum_repo, "modules");
    modulesFn = tmp ? tmp : "";
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
    if (auto cRevision = yum_repomd->revision) {
        revision = cRevision;
    }
    maxTimestamp = lr_yum_repomd_get_highest_timestamp(yum_repomd, NULL);

    // Load timestamp unless explicitly expired
    if (timestamp != 0) {
        timestamp = mtime(primaryFn.c_str());
    }
    g_strfreev(this->mirrors);
    this->mirrors = mirrors;
    return true;
}

// Use metalink to check whether our metadata are still current.
bool Repo::Impl::isMetalinkInSync()
{
    auto logger(Log::getLogger());
    char tmpdir[] = "/tmp/tmpdir.XXXXXX";
    mkdtemp(tmpdir);
    Finalizer tmpDirRemover([&tmpdir](){
        dnf_remove_recursive(tmpdir, NULL);
    });

    std::unique_ptr<LrHandle> h(lrHandleInitRemote(tmpdir));

    handleSetOpt(h.get(), LRO_FETCHMIRRORS, 1L);
    auto r = lrHandlePerform(h.get(), tmpdir, false);
    LrMetalink * metalink;
    handleGetInfo(h.get(), LRI_METALINK, &metalink);
    if (!metalink) {
        logger->debug(tfm::format(_("reviving: repo '%s' skipped, no metalink."), id));
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
        logger->debug(tfm::format(_("reviving: repo '%s' skipped, no usable hash."), id));
        return false;
    }

    for (auto & hash : hashes) {
        auto chkType = solv_chksum_str2type(hash.lrMetalinkHash->type);
        hash.chksum.reset(solv_chksum_create(chkType));
    }

    std::ifstream repomd(repomdFn, std::ifstream::binary);
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
            logger->debug(tfm::format(_("reviving: failed for '%s', mismatched %s sum."),
                                      id, hash.lrMetalinkHash->type));
            return false;
        }
    }

    logger->debug(tfm::format(_("reviving: '%s' can be revived - metalink checksums match."), id));
    return true;
}

// Use repomd to check whether our metadata are still current.
bool Repo::Impl::isRepomdInSync()
{
    auto logger(Log::getLogger());
    LrYumRepo *yum_repo;
    char tmpdir[] = "/tmp/tmpdir.XXXXXX";
    mkdtemp(tmpdir);
    Finalizer tmpDirRemover([&tmpdir](){
        dnf_remove_recursive(tmpdir, NULL);
    });

    const char *dlist[] = LR_YUM_REPOMDONLY;

    std::unique_ptr<LrHandle> h(lrHandleInitRemote(tmpdir));

    handleSetOpt(h.get(), LRO_YUMDLIST, dlist);
    auto r = lrHandlePerform(h.get(), tmpdir, false);
    resultGetInfo(r.get(), LRR_YUM_REPO, &yum_repo);

    auto same = haveFilesSameContent(repomdFn.c_str(), yum_repo->repomd);
    if (same)
        logger->debug(tfm::format(_("reviving: '%s' can be revived - repomd matches."), id));
    else
        logger->debug(tfm::format(_("reviving: failed for '%s', mismatched repomd."), id));
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
        throw std::runtime_error(tfm::format(_("Cannot create repo cache directory \"%s\": %s"),
                                             cacheDir, errTxt));
    }
    auto tmpdir = cacheDir + "/tmpdir.XXXXXX";
    if (!mkdtemp(&tmpdir.front())) {
        const char * errTxt = strerror(errno);
        throw std::runtime_error(tfm::format(_("Cannot create repo temporary directory \"%s\": %s"),
                                             tmpdir.c_str(), errTxt));
    }
    Finalizer tmpDirRemover([&tmpdir](){
        dnf_remove_recursive(tmpdir.c_str(), NULL);
    });
    auto tmprepodir = tmpdir + "/repodata";

    std::unique_ptr<LrHandle> h(lrHandleInitRemote(tmpdir.c_str()));
    auto r = lrHandlePerform(h.get(), tmpdir, conf->repo_gpgcheck().getValue());

    dnf_remove_recursive(repodir.c_str(), NULL);
    if (g_mkdir_with_parents(repodir.c_str(), 0755) == -1) {
        const char * errTxt = strerror(errno);
        throw std::runtime_error(tfm::format(_("Cannot create directory \"%s\": %s"),
                                             repodir, errTxt));
    }
    if (rename(tmprepodir.c_str(), repodir.c_str()) == -1) {
        const char * errTxt = strerror(errno);
        throw std::runtime_error(tfm::format(_("Cannot rename directory \"%s\" to \"%s\": %s"),
                                             tmprepodir, repodir, errTxt));
    }

    timestamp = -1;
}

bool Repo::Impl::load()
{
    auto logger(Log::getLogger());
    try {
        if (!primaryFn.empty() || loadCache(false)) {
            if (conf->getMasterConfig().check_config_file_age().getValue() &&
                !repoFilePath.empty() && mtime(repoFilePath.c_str()) > mtime(primaryFn.c_str()))
                expired = true;
            if (!expired || syncStrategy == SyncStrategy::ONLY_CACHE || syncStrategy == SyncStrategy::LAZY) {
                logger->debug(tfm::format(_("repo: using cache for: %s"), id));
                return false;
            }

            if (isInSync()) {
                // the expired metadata still reflect the origin:
                utimes(primaryFn.c_str(), NULL);
                expired = false;
                return true;
            }
        }
        if (syncStrategy == SyncStrategy::ONLY_CACHE) {
            auto msg = tfm::format(_("Cache-only enabled but no cache for '%s'"), id);
            throw std::runtime_error(msg);
        }

        logger->debug(tfm::format(_("repo: downloading from remote: %s"), id));
        fetch();
        loadCache(true);
    } catch (const LrExceptionWithSourceUrl & e) {
        logger->debug(tfm::format(_("Cannot download '%s': %s."), e.getSourceUrl(), e.what()));
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
    return time(NULL) - mtime(primaryFn.c_str());
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

void Repo::Impl::downloadUrl(const char * url, int fd)
{
    if (callbacks)
        callbacks->start(
            !conf->name().getValue().empty() ? conf->name().getValue().c_str() :
            (!id.empty() ? id.c_str() : "unknown")
        );

    GError * errP{nullptr};
    lr_download_url(getCachedHandle(), url, fd, &errP);
    std::unique_ptr<GError> err(errP);

    if (callbacks)
        callbacks->end();

    if (err)
        throwException(std::move(err));
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
    return pImpl->maxTimestamp;
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
    auto logger(Log::getLogger());
    hy_repo_set_string(hrepo, HY_REPO_MD_FN, pImpl->repomdFn.c_str());
    hy_repo_set_string(hrepo, HY_REPO_PRIMARY_FN, pImpl->primaryFn.c_str());
    if (pImpl->filelistsFn.empty())
        logger->debug(tfm::format(_("not found filelist for: %s"), pImpl->conf->name().getValue()));
    else
        hy_repo_set_string(hrepo, HY_REPO_FILELISTS_FN, pImpl->filelistsFn.c_str());
#ifdef MODULEMD
    if (pImpl->modulesFn.empty())
        logger->debug(tfm::format(_("not found modules for: %s"), pImpl->conf->name().getValue()));
    else
        hy_repo_set_string(hrepo, MODULES_FN, pImpl->modulesFn.c_str());
#endif
    hy_repo_set_cost(hrepo, pImpl->conf->cost().getValue());
    hy_repo_set_priority(hrepo, pImpl->conf->priority().getValue());
    if (pImpl->prestoFn.empty())
        logger->debug(tfm::format(_("not found deltainfo for: %s"), pImpl->conf->name().getValue()));
    else
        hy_repo_set_string(hrepo, HY_REPO_PRESTO_FN, pImpl->prestoFn.c_str());
    if (pImpl->updateinfoFn.empty())
        logger->debug(tfm::format(_("not found updateinfo for: %s"), pImpl->conf->name().getValue()));
    else
        hy_repo_set_string(hrepo, HY_REPO_UPDATEINFO_FN, pImpl->updateinfoFn.c_str());
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
    pImpl->downloadUrl(url, fd);
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
    Impl(Repo * repo, const char * relativeUrl, const char * dest, int chksType,
         const char * chksum, int64_t expectedSize, const char * baseUrl, bool resume,
         int64_t byteRangeStart, int64_t byteRangeEnd, PackageTargetCB * callbacks);

    Impl(ConfigMain * cfg, const char * relativeUrl, const char * dest, int chksType,
         const char * chksum, int64_t expectedSize, const char * baseUrl, bool resume,
         int64_t byteRangeStart, int64_t byteRangeEnd, PackageTargetCB * callbacks);

    void download();

    ~Impl();

    PackageTargetCB * callbacks;

    std::unique_ptr<LrPackageTarget> lrPkgTarget;

private:
    void init(LrHandle * handle, const char * relativeUrl, const char * dest, int chksType,
              const char * chksum, int64_t expectedSize, const char * baseUrl, bool resume,
              int64_t byteRangeStart, int64_t byteRangeEnd);

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

PackageTarget::Impl::Impl(Repo * repo, const char * relativeUrl, const char * dest, int chksType,
                          const char * chksum, int64_t expectedSize, const char * baseUrl, bool resume,
                          int64_t byteRangeStart, int64_t byteRangeEnd, PackageTargetCB * callbacks)
: callbacks(callbacks)
{
    init(repo->pImpl->getCachedHandle(), relativeUrl, dest, chksType, chksum, expectedSize,
         baseUrl, resume, byteRangeStart, byteRangeEnd);
}

PackageTarget::Impl::Impl(ConfigMain * cfg, const char * relativeUrl, const char * dest, int chksType,
                          const char * chksum, int64_t expectedSize, const char * baseUrl, bool resume,
                          int64_t byteRangeStart, int64_t byteRangeEnd, PackageTargetCB * callbacks)
: callbacks(callbacks)
{
    lrHandle.reset(newHandle(cfg));
    handleSetOpt(lrHandle.get(), LRO_REPOTYPE, LR_YUMREPO);
    init(lrHandle.get(), relativeUrl, dest, chksType, chksum, expectedSize, baseUrl, resume,
         byteRangeStart, byteRangeEnd);
}

void PackageTarget::Impl::init(LrHandle * handle, const char * relativeUrl, const char * dest, int chksType,
                               const char * chksum, int64_t expectedSize, const char * baseUrl, bool resume,
                               int64_t byteRangeStart, int64_t byteRangeEnd)
{
    LrChecksumType lrChksType = static_cast<LrChecksumType>(chksType);

    if (resume && byteRangeStart) {
        auto msg = _("resume cannot be used simultaneously with the byterangestart param");
        throw std::runtime_error(msg);
    }

    GError * errP{nullptr};
    lrPkgTarget.reset(lr_packagetarget_new_v3(handle, relativeUrl, dest, lrChksType, chksum,  expectedSize,
                                              baseUrl, resume, progressCB, callbacks, endCB, mirrorFailureCB,
                                              byteRangeStart, byteRangeEnd, &errP));
    std::unique_ptr<GError> err(errP);

    if (!lrPkgTarget) {
        auto msg = tfm::format(_("PackageTarget initialization failed: %s"), err->message);
        throw std::runtime_error(msg);
    }
}

PackageTarget::PackageTarget(Repo * repo, const char * relativeUrl, const char * dest, int chksType,
                             const char * chksum, int64_t expectedSize, const char * baseUrl, bool resume,
                             int64_t byteRangeStart, int64_t byteRangeEnd, PackageTargetCB * callbacks)
: pImpl(new Impl(repo, relativeUrl, dest, chksType, chksum, expectedSize, baseUrl, resume,
                 byteRangeStart, byteRangeEnd, callbacks))
{}

PackageTarget::PackageTarget(ConfigMain * cfg, const char * relativeUrl, const char * dest, int chksType,
                             const char * chksum, int64_t expectedSize, const char * baseUrl, bool resume,
                             int64_t byteRangeStart, int64_t byteRangeEnd, PackageTargetCB * callbacks)
: pImpl(new Impl(cfg, relativeUrl, dest, chksType, chksum, expectedSize, baseUrl, resume,
                 byteRangeStart, byteRangeEnd, callbacks))
{}


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

static void librepoLogCB(G_GNUC_UNUSED const gchar *log_domain, GLogLevelFlags log_level,
                         const char *msg, gpointer user_data) noexcept
{
    // Ignore exception during logging. Eg. exception generated during logging of exception is not good.
    try {
        auto data = static_cast<LrHandleLogData *>(user_data);
        auto now = time(NULL);
        struct tm nowTm;
        gmtime_r(&now, &nowTm);

        std::ostringstream ss;
        ss << std::setfill('0');
        ss << std::setw(4) << nowTm.tm_year+1900 << "-" << std::setw(2) << nowTm.tm_mon+1;
        ss << "-" << std::setw(2) << nowTm.tm_mday;
        ss << "T" << std::setw(2) << nowTm.tm_hour << ":" << std::setw(2) << nowTm.tm_min;
        ss << ":" << std::setw(2) << nowTm.tm_sec << "Z ";
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
