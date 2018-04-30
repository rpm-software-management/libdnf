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

#include <librepo/librepo.h>
#include <solv/repo.h>
#include <utime.h>

#include <map>

namespace libdnf {

class Repo::Impl {
public:
    Impl(const std::string & id, std::unique_ptr<ConfigRepo> && config);
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
    void resetTimestamp();
    LrHandle * lrHandleInitBase();
    LrHandle * lrHandleInitLocal();
    LrHandle * lrHandleInitRemote(const char *destdir);

    std::string id;
    std::unique_ptr<ConfigRepo> conf;

    char ** mirrors;
    // 0 forces expiration on the next call to load(), -1 means undefined value
    int timestamp;
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

Repo::Impl::Impl(const std::string & id, std::unique_ptr<ConfigRepo> && config)
: id(id), conf(std::move(config)), timestamp(-1) {}

Repo::Impl::~Impl()
{
}

Repo::Repo(const std::string & id, std::unique_ptr<ConfigRepo> && config)
: pImpl(new Impl(id, std::move(config))) {}

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
bool Repo::getUseIncludes() const { return pImpl->useIncludes; }
void Repo::setUseIncludes(bool enabled) { pImpl->useIncludes = enabled; }
int Repo::getCost() const { return pImpl->conf->cost().getValue(); }
int Repo::getPriority() const { return pImpl->conf->priority().getValue(); }
std::string Repo::getCompsFn() { return pImpl->comps_fn; }

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
    lr_handle_setopt(h, NULL, LRO_MAXMIRRORTRIES, 0);
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

    std::string cachedir = getCachedir();
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

    LrUrlVars * vars = NULL;
    for (const auto & item : substitutions)
        vars = lr_urlvars_set(vars, item.first.c_str(), item.second.c_str());
    lr_handle_setopt(h, NULL, LRO_VARSUB, vars);

    const char *urls[] = {conf->baseurl().getValue()[0].c_str(), NULL};
    lr_handle_setopt(h, NULL, LRO_URLS, urls);
    lr_handle_setopt(h, NULL, LRO_DESTDIR, destdir);
    return h;
}

bool Repo::Impl::loadCache()
{
    GError *err = NULL;
    char **mirrors;
    LrYumRepo *yum_repo;
    LrYumRepoMd *yum_repomd;

    std::unique_ptr<LrHandle, decltype(&lr_handle_free)> h(lrHandleInitLocal(), &lr_handle_free);
    std::unique_ptr<LrResult, decltype(&lr_result_free)> r(lr_result_init(),
                                                           &lr_result_free);

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
    // FIXME this crashes:
    /* presto_fn = lr_yum_repo_path(yum_repo, "prestodelta"); */
    /* updateinfo_fn = lr_yum_repo_path(yum_repo, "updateinfo"); */
    comps_fn = lr_yum_repo_path(yum_repo, "group_gz");  // FIXME: or "group"
    content_tags = yum_repomd->content_tags;
    distro_tags = yum_repomd->distro_tags;
    revision = yum_repomd->revision;

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
    // FIXME
    return conf->basecachedir().getValue() + "/test";
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

void Repo::Impl::resetTimestamp()
{
    time_t now = time(NULL);
    struct utimbuf unow;
    unow.actime = now;
    unow.modtime = now;
    timestamp = now;
    utime(primary_fn.c_str(), &unow);
}

}
