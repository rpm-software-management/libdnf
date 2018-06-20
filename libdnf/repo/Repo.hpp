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
#include <stdexcept>

namespace libdnf {

class LrException : public std::runtime_error {
public:
    LrException(const char * msg) : runtime_error(msg) {}
    LrException(const std::string & msg) : runtime_error(msg) {}
};

class LrIO : public LrException {
public:
    LrIO(const std::string & msg) : LrException(msg) {}
};

class LrCannotCreateDir : public LrException {
public:
    LrCannotCreateDir(const std::string & msg) : LrException(msg) {}
};

class LrCannotCreateTmp : public LrException {
public:
    LrCannotCreateTmp(const std::string & msg) : LrException(msg) {}
};

class LrMemory : public LrException {
public:
    LrMemory(const std::string & msg) : LrException(msg) {}
};

class LrBadFuncArg : public LrException {
public:
    LrBadFuncArg(const std::string & msg) : LrException(msg) {}
};

class LrBadOptArg : public LrException {
public:
    LrBadOptArg(const std::string & msg) : LrException(msg) {}
};

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

    /**
    * @brief Verify repo object configuration
    *
    * Will throw exception if Repo has no mirror or baseurl set or if Repo type is unsupported.
    */
    void verify() const;
    ConfigRepo * getConfig() noexcept;
    const std::string & getId() const noexcept;
    void enable();
    void disable();
    bool isEnabled() const;
    bool isLocal() const;
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
    const std::string & getRevision() const;
    void initHyRepo(HyRepo hrepo);
    int getAge() const;

    /**
    * @brief Mark whatever is in the current cache expired.
    *
    * This repo instance will alway try to fetch a fresh metadata after this
    * method is called.
    */
    void expire();

    /**
    * @brief Return whether the cached metadata is expired.
    *
    * @return bool
    */
    bool isExpired() const;

    /**
    * @brief Get the number of seconds after which the cached metadata will expire.
    *
    * Negative number means the metadata has expired already.
    *
    * @return Seconds to expiration
    */
    int getExpiresIn() const;
    bool fresh();
    void setMaxMirrorTries(int maxMirrorTries);
    int getTimestamp() const;
    int getMaxTimestamp();
    const std::vector<std::string> & getContentTags();
    const std::vector<std::pair<std::string, std::string>> & getDistroTags();

    std::string getCachedir() const;
    void setRepoFilePath(const std::string & path);
    const std::string & getRepoFilePath() const noexcept;
    void setSyncStrategy(SyncStrategy strategy);
    SyncStrategy getSyncStrategy() const noexcept;
    void downloadUrl(const char * url, int fd);
    std::vector<std::string> getMirrors() const;

    ~Repo();
private:
    friend struct PackageTarget;
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

struct Downloader {
public:
    static void downloadURL(ConfigMain * cfg, const char * url, int fd);
};

/**
* @class PackageTargetCB
*
* @brief Base class for PackageTarget callbacks
*
* User implements PackageTarget callbacks by inheriting this class and overriding its methods.
*/
class PackageTargetCB {
public:
    virtual int end(int status, const char * msg);
    virtual int progress(double totalToDownload, double downloaded);
    virtual int mirrorFailure(const char *msg, const char *url);
    virtual ~PackageTargetCB() = default;
};

/**
* @class PackageTarget
*
* @brief Wraps librepo PackageTarget
*/
struct PackageTarget {
public:
    static void downloadPackages(std::vector<PackageTarget *> & targets, bool failFast);

    PackageTarget(Repo * repo, const char * relativeUrl, const char * dest, int chksType, const char * chksum, int64_t expectedSize, const char * baseUrl, bool resume, int64_t byteRangeStart, int64_t byteRangeEnd, PackageTargetCB * callbacks);
    PackageTarget(ConfigMain * cfg, const char * relativeUrl, const char * dest, int chksType, const char * chksum, int64_t expectedSize, const char * baseUrl, bool resume, int64_t byteRangeStart, int64_t byteRangeEnd, PackageTargetCB * callbacks);
    ~PackageTarget();

    PackageTargetCB * getCallbacks();
    const char * getErr();
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}

#endif
