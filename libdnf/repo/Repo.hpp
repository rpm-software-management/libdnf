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

#ifndef _LIBDNF_REPO_HPP
#define _LIBDNF_REPO_HPP

#include "../conf/ConfigRepo.hpp"
#include "../hy-types.h"

#include <memory>
#include <glib.h>

namespace libdnf {

/**
* @class RepoCB
*
* @brief Base class for Repo callbacks
*
* User implements repo callbacks by inheriting this class and overriding its methods.
*/
class RepoCB {
public:
    virtual void start(const char *what) {}
    virtual void end() {}
    virtual int progress(double totalToDownload, double downloaded);
    virtual void fastestMirror(int stage, const char *msg);
    virtual int handleMirrorFailure(const char *msg, const char *url, const char *metadata);
    virtual ~RepoCB() = default;
};

/**
* @class Repo
*
* @brief Package repository
*
* Represents a repository used to download packages.
* Remote metadata is cached locally.
*
*/
struct Repo {
public:

    enum class SyncStrategy {
        // use the local cache even if it's expired. download if there's no cache.
        LAZY,
        // use the local cache, even if it's expired, never download.
        ONLY_CACHE,
        // try the cache, if it is expired download new md.
        TRY_CACHE
    };


    /**
    * @brief Verify repo ID
    *
    * @param id repo ID to verify
    * @return   index of the first invalid character in the repo ID (if present) or -1
    */
    static int verifyId(const std::string & id);

    /**
    * @brief Construct the Repo object
    *
    * @param id     repo ID to use
    * @param conf   configuration to use
    */
    Repo(const std::string & id, std::unique_ptr<ConfigRepo> && conf);

    void setCallbacks(std::unique_ptr<RepoCB> && callbacks);

    ConfigRepo * getConfig() noexcept;
    const std::string & getId() const noexcept;
    void enable();
    void disable();
    /**
    * @brief Initialize the repo with metadata
    *
    * Fetches new metadata from the origin or just reuses local cache if still valid.
    *
    * @return true if fresh metadata were downloaded, false otherwise.
    */
    bool load();
    bool loadCache();
    bool getUseIncludes() const;
    void setUseIncludes(bool enabled);
    int getCost() const;
    int getPriority() const;
    std::string getCompsFn();  // this is temporarily made public for DNF compatibility
    std::string getRevision();
    void initHyRepo(HyRepo hrepo);
    int getAge() const;
    void expire();
    bool isExpired() const;
    int getExpiresIn() const;
    bool fresh();
    int getTimestamp();
    int getMaxTimestamp();
    GSList * getContentTags();
    GSList * getDistroTags();

    void setRepoFilePath(const std::string & path);
    const std::string & getRepoFilePath() const noexcept;
    void setSyncStrategy(SyncStrategy strategy);
    SyncStrategy getSyncStrategy() const noexcept;

    ~Repo();
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}

#endif
