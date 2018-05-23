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

#include <librepo/librepo.h>
#include <utime.h>

#include <solv/chksum.h>
#include <solv/repo.h>

#include <map>
#include <glib.h>

#include <iostream>

namespace libdnf {

class Repo::Impl {
public:
    Impl(const std::string & id, std::unique_ptr<ConfigRepo> && conf);
    ~Impl();

    bool load();
    bool loadCache();
    bool canReuse();
    void fetch();
    std::string getCachedir();
    char ** getMirrors();
    int getAge() const;
    void expire();
    bool expired() const;
    int expiresIn();
    void resetTimestamp();
    LrHandle * lrHandleInitBase();
    LrHandle * lrHandleInitLocal();
    LrHandle * lrHandleInitRemote(const char *destdir);

    std::string id;
    std::unique_ptr<ConfigRepo> conf;

    char ** mirrors;
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
    GSList * content_tags;
    GSList * distro_tags;
    unsigned char checksum[CHKSUM_BYTES];
    bool useIncludes;
    std::map<std::string, std::string> substitutions;
};

Repo::Impl::Impl(const std::string & id, std::unique_ptr<ConfigRepo> && conf)
: id(id), conf(std::move(conf)), timestamp(-1) {}

Repo::Impl::~Impl()
{
}

Repo::Repo(const std::string & id, std::unique_ptr<ConfigRepo> && conf)
: pImpl(new Impl(id, std::move(conf))) {}

Repo::~Repo() = default;

int Repo::verifyId(const std::string & id)
{
    auto idx = id.find_first_not_of(REPOID_CHARS);
    return idx == id.npos ? -1 : idx;
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

bool Repo::load() { return pImpl->load(); }
bool Repo::loadCache() { return pImpl->loadCache(); }
bool Repo::getUseIncludes() const { return pImpl->useIncludes; }
void Repo::setUseIncludes(bool enabled) { pImpl->useIncludes = enabled; }
int Repo::getCost() const { return pImpl->conf->cost().getValue(); }
int Repo::getPriority() const { return pImpl->conf->priority().getValue(); }
std::string Repo::getCompsFn() { return pImpl->comps_fn; }
int Repo::getAge() const { return pImpl->getAge(); }
void Repo::expire() { pImpl->expire(); }
bool Repo::expired() const { return pImpl->expired(); }
int Repo::expiresIn() { return pImpl->expiresIn(); }

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
    std::cout << "cachedir: " << cachedir << std::endl;
    const char *urls[] = {cachedir.c_str(), NULL};
    lr_handle_setopt(h, NULL, LRO_URLS, urls);
    lr_handle_setopt(h, NULL, LRO_DESTDIR, cachedir.c_str());
    lr_handle_setopt(h, NULL, LRO_LOCAL, 1L);
    return h;
}

LrHandle *
Repo::Impl::lrHandleInitRemote(const char *destdir)
{
    LrHandle *h = lrHandleInitBase();
    lr_handle_setopt(h, NULL, LRO_MAXMIRRORTRIES, 0);
    lr_handle_setopt(h, NULL, LRO_MAXPARALLELDOWNLOADS,
                     conf->max_parallel_downloads().getValue());

    LrUrlVars * vars = NULL;
    for (const auto & item : substitutions)
        vars = lr_urlvars_set(vars, item.first.c_str(), item.second.c_str());
    lr_handle_setopt(h, NULL, LRO_VARSUB, vars);

    lr_handle_setopt(h, NULL, LRO_DESTDIR, destdir);

    enum class Source {NONE, METALINK, MIRRORLIST} source{Source::NONE};
    std::string tmp;
    if (!conf->metalink().empty() && !(tmp=conf->metalink().getValue()).empty())
        source = Source::METALINK;
    else if (!conf->mirrorlist().empty() && !(tmp=conf->mirrorlist().getValue()).empty())
        source = Source::MIRRORLIST;
    if (source != Source::NONE) {
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
    } else if (!conf->baseurl().getValue().empty()) {
        size_t len = conf->baseurl().getValue().size();
        const char * urls[len + 1];
        for (size_t idx = 0; idx < len; ++idx)
            urls[idx] = conf->baseurl().getValue()[idx].c_str();
        urls[len] = nullptr;
        lr_handle_setopt(h, nullptr, LRO_URLS, urls);
    } else
        throw std::runtime_error(tfm::format(_("Cannot find a valid baseurl for repo: %s"), id));
    return h;
}

bool Repo::Impl::loadCache()
{
    GError *err = NULL;
    char **mirrors;
    LrYumRepo *yum_repo;
    LrYumRepoMd *yum_repomd;

    std::unique_ptr<LrHandle, decltype(&lr_handle_free)> h(lrHandleInitLocal(), &lr_handle_free);
    std::unique_ptr<LrResult, decltype(&lr_result_free)> r(lr_result_init(), &lr_result_free);

    // Fetch data
    lr_handle_perform(h.get(), r.get(), &err);
    if (err) {
        return false;
    }
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
    content_tags = yum_repomd->content_tags;
    distro_tags = yum_repomd->distro_tags;
    revision = yum_repomd->revision;
    max_timestamp = lr_yum_repomd_get_highest_timestamp(yum_repomd, NULL);

    // Load timestamp unless explicitly expired
    if (timestamp != 0) {
        timestamp = mtime(primary_fn.c_str());
    }
    this->mirrors = mirrors;
    return true;
}

bool Repo::Impl::canReuse()
{
    LrYumRepo *yum_repo;
    GError *err = NULL;
    char tmpdir[] = "/tmp/tmpdir.XXXXXX";
    mkdtemp(tmpdir);
    const char *dlist[] = LR_YUM_REPOMDONLY;

    std::unique_ptr<LrHandle, decltype(&lr_handle_free)> h(lrHandleInitRemote(tmpdir),
                                                           &lr_handle_free);
    std::unique_ptr<LrResult, decltype(&lr_result_free)> r(lr_result_init(),
                                                           &lr_result_free);

    lr_handle_setopt(h.get(), NULL, LRO_YUMDLIST, dlist);

    lr_handle_perform(h.get(), r.get(), &err);
    lr_result_getinfo(r.get(), NULL, LRR_YUM_REPO, &yum_repo);

    const char *ock = cksum(repomd_fn.c_str(), G_CHECKSUM_SHA256);
    const char *nck = cksum(yum_repo->repomd, G_CHECKSUM_SHA256);

    dnf_remove_recursive(tmpdir, NULL);

    return strcmp(ock, nck) == 0;
}

void Repo::Impl::fetch()
{
    GError *err = NULL;
    char tmpdir[] = "/var/tmp/tmpdir.XXXXXX";
    mkdtemp(tmpdir);
    std::string tmprepodir = tmpdir + std::string("/repodata");
    std::string repodir = getCachedir() + "/repodata";

    std::unique_ptr<LrHandle, decltype(&lr_handle_free)> h(lrHandleInitRemote(tmpdir),
                                                           &lr_handle_free);
    std::unique_ptr<LrResult, decltype(&lr_result_free)> r(lr_result_init(),
                                                           &lr_result_free);
    lr_handle_perform(h.get(), r.get(), &err);

    dnf_remove_recursive(repodir.c_str(), NULL);
    g_mkdir_with_parents(repodir.c_str(), 0);
    rename(tmprepodir.c_str(), repodir.c_str());
    dnf_remove_recursive(tmpdir, NULL);

    timestamp = -1;
}

bool Repo::Impl::load()
{
    printf("check if cache present\n");
    if (loadCache()) {
        if (!expired()) {
            printf("using cache, age: %is\n", getAge());
            return false;
        }
        printf("try to reuse\n");
        if (canReuse()) {
            printf("reusing expired cache\n");
            resetTimestamp();
            return false;
        }
    }

    printf("fetch\n");
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

char ** Repo::Impl::getMirrors()
{
    return mirrors;
}

int Repo::Impl::getAge() const
{
    return time(NULL) - timestamp;
}

void Repo::Impl::expire()
{
    timestamp = 0;
}

bool Repo::Impl::expired() const
{
    int maxAge = conf->metadata_expire().getValue();
    return timestamp == 0 || (maxAge >= 0 && getAge() > maxAge);
}

int Repo::Impl::expiresIn()
{
    return conf->metadata_expire().getValue() - getAge();
}

bool Repo::fresh()
{
    return pImpl->timestamp >= 0;
}

void Repo::Impl::resetTimestamp()
{
    time_t now = time(NULL);
    struct utimbuf unow;
    unow.actime = now;
    unow.modtime = now;
    timestamp = now;
    utime(primary_fn.c_str(), &unow);
}

int Repo::getTimestamp()
{
    return pImpl->timestamp;
}

int Repo::getMaxTimestamp()
{
    return pImpl->max_timestamp;
}

GSList * Repo::getContentTags()
{
    return pImpl->content_tags;
}

GSList * Repo::getDistroTags()
{
    return pImpl->distro_tags;
}

std::string Repo::getRevision()
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

}
