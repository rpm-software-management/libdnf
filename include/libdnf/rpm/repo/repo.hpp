/*
Copyright (C) 2020 Red Hat, Inc.

This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

Libdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Libdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with libdnf.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef LIBDNF_RPM_REPO_REPO_HPP
#define LIBDNF_RPM_REPO_REPO_HPP

#define MODULEMD

#include "config_repo.hpp"

#include <memory>
#include <stdexcept>

namespace libdnf {

class Base;

}  // namespace libdnf

namespace libdnf::rpm {

class LrException : public RuntimeError {
public:
    LrException(int code, const char * msg) : RuntimeError(msg), code(code) {}
    LrException(int code, const std::string & msg) : RuntimeError(msg), code(code) {}
    const char * get_domain_name() const noexcept override { return "librepo"; }
    const char * get_name() const noexcept override { return "LrException"; }
    const char * get_description() const noexcept override { return "Librepo exception"; }
    int get_code() const noexcept { return code; }

private:
    int code;
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
    enum class FastestMirrorStage {
        INIT, /*!<
            Fastest mirror detection just started.
            ptr is NULL*/

        CACHELOADING, /*!<
            ptr is (char *) pointer to string with path to the cache file.
            (Do not modify or free the string). */

        CACHELOADINGSTATUS, /*!<
            if cache was loaded successfully, ptr is NULL, otherwise
            ptr is (char *) string with error message.
            (Do not modify or free the string) */

        DETECTION, /*!<
            Detection (pinging) in progress.
            If all data was loaded from cache, this stage is skiped.
            ptr is pointer to long. This is the number of how much
            mirrors have to be "pinged" */

        FINISHING, /*!<
            Detection is done, sorting mirrors, updating cache, etc.
            ptr is NULL */

        STATUS, /*!<
            The very last invocation of fastest mirror callback.
            If fastest mirror detection was successful ptr is NULL,
            otherwise ptr contain (char *) string with error message.
            (Do not modify or free the string) */
    };

    virtual void start(const char * /*what*/) {}
    virtual void end() {}
    virtual int progress(double total_to_download, double downloaded);
    virtual void fastest_mirror(FastestMirrorStage stage, const char * msg);
    virtual int handle_mirror_failure(const char * msg, const char * url, const char * metadata);
    virtual bool repokey_import(
        const std::string & id,
        const std::string & user_id,
        const std::string & fingerprint,
        const std::string & url,
        long int timestamp);
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
class Repo {
public:
    enum class Type { AVAILABLE, SYSTEM, COMMANDLINE };

    enum class SyncStrategy {
        // use the local cache even if it's expired. download if there's no cache.
        LAZY = 1,
        // use the local cache, even if it's expired, never download.
        ONLY_CACHE = 2,
        // try the cache, if it is expired download new md.
        TRY_CACHE = 3
    };


    /**
    * @brief Verify repo ID
    *
    * @param id repo ID to verify
    * @return   index of the first invalid character in the repo ID (if present) or -1
    */
    static int verify_id(const std::string & id);

    /**
    * @brief Construct the Repo object
    *
    * @param id     repo ID to use
    * @param conf   configuration to use
    */
    Repo(
        const std::string & id,
        std::unique_ptr<ConfigRepo> && conf,
        Base & base,
        Repo::Type type = Repo::Type::AVAILABLE);

    Repo & operator=(Repo && repo) = delete;

    void set_callbacks(std::unique_ptr<RepoCB> && callbacks);

    /**
    * @brief Verify repo object configuration
    *
    * Will throw exception if Repo has no mirror or baseurl set or if Repo type is unsupported.
    */
    void verify() const;
    ConfigRepo * get_config() noexcept;
    const std::string & get_id() const noexcept;
    void enable();
    void disable();
    bool is_enabled() const;
    bool is_local() const;
    /**
    * @brief Initialize the repo with metadata
    *
    * Fetches new metadata from the origin or just reuses local cache if still valid.
    *
    * @return true if fresh metadata were downloaded, false otherwise.
    */
    bool load();
    bool load_cache(bool throw_except);
    void download_metadata(const std::string & destdir);
    bool get_use_includes() const;
    void set_use_includes(bool enabled);
    bool get_load_metadata_other() const;
    void set_load_metadata_other(bool value);
    int get_cost() const;
    int get_priority() const;
    std::string get_comps_fn();  // this is temporarily made public for DNF compatibility
#ifdef MODULEMD
    std::string get_modules_fn();  // temporary made public
#endif
    const std::string & get_revision() const;
    int64_t get_age() const;

    /**
    * @brief Ask for additional repository metadata type to download
    *
    * given metadata are appended to the default metadata set when repository is downloaded
    *
    * @param metadataType metadata type (filelists, other, productid...)
    */
    void add_metadata_type_to_download(const std::string & metadata_type);

    /**
    * @brief Stop asking for this additional repository metadata type
    *
    * given metadata_type is no longer downloaded by default
    * when this repository is downloaded.
    *
    * @param metadataType metadata type (filelists, other, productid...)
    */
    void remove_metadata_type_from_download(const std::string & metadata_type);

    /**
    * @brief Return path to the particular downloaded repository metadata in cache
    *
    * @param metadataType metadata type (filelists, other, productid...)
    *
    * @return file path or empty string in case the requested metadata does not exist
    */
    std::string get_metadata_path(const std::string & metadata_type);

    /**
    * @brief Return content of the particular downloaded repository metadata
    *
    * Content of compressed metadata file is returned uncompressed
    *
    * @param metadataType metadata type (filelists, other, productid...)
    *
    * @return content of metadata file or empty string in case the requested metadata does not exist
    */
    //std::string get_metadata_content(const std::string & metadataType);

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
    bool is_expired() const;

    /**
    * @brief Get the number of seconds after which the cached metadata will expire.
    *
    * Negative number means the metadata has expired already.
    *
    * @return Seconds to expiration
    */
    int get_expires_in() const;
    bool fresh();
    void set_max_mirror_tries(int max_mirror_tries);
    int64_t get_timestamp() const;
    int get_max_timestamp();

    /**
    * @brief Try to preserve remote side timestamps
    *
    * When set to true the underlying librepo is asked to make an attempt to set the timestamps
    * of the local downloaded files (repository metadata and packages) to match those from
    * the remote files.
    * This feature is by default switched off.
    *
    * @param preserveRemoteTime true - use remote file timestamp, false - use the current time
    */
    void set_preserve_remote_time(bool preserve_remote_time);
    bool get_preserve_remote_time() const;

    const std::vector<std::string> & get_content_tags();
    const std::vector<std::pair<std::string, std::string>> & get_distro_tags();

    /**
    * @brief Get list of relative locations of metadata files inside the repo
    *
    * e.g. [('primary', 'repodata/primary.xml.gz'), ('filelists', 'repodata/filelists.xml.gz')...]
    *
    * @return vector of (metadata_type, location) string pairs
    */
    const std::vector<std::pair<std::string, std::string>> get_metadata_locations() const;

    std::string get_cachedir() const;
    void set_repo_file_path(const std::string & path);
    const std::string & get_repo_file_path() const noexcept;
    void set_sync_strategy(SyncStrategy strategy);
    SyncStrategy get_sync_strategy() const noexcept;
    void download_url(const char * url, int fd);

    /**
    * @brief Set http headers.
    *
    * Example:
    * {"User-Agent: Agent007", "MyMagicHeader: I'm here", nullptr}
    *
    * @param headers nullptr terminated array of C strings
    */
    void set_http_headers(const char * headers[]);

    /**
    * @brief Get array of added/changed/removed http headers.
    *
    * @return nullptr terminated array of C strings
    */
    const char * const * get_http_headers() const;
    std::vector<std::string> get_mirrors() const;

    void set_substitutions(const std::map<std::string, std::string> & substitutions);

    ~Repo();

    class Impl;

private:
    friend struct PackageTarget;
    std::unique_ptr<Impl> p_impl;
};

struct Downloader {
public:
    static void download_url(ConfigMain * cfg, const char * url, int fd);
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
    /** Transfer status codes */
    enum class TransferStatus { SUCCESSFUL, ALREADYEXISTS, ERROR };

    virtual int end(TransferStatus status, const char * msg);
    virtual int progress(double total_to_download, double downloaded);
    virtual int mirror_failure(const char * msg, const char * url);
    virtual ~PackageTargetCB() = default;
};

/**
* @class PackageTarget
*
* @brief Wraps librepo PackageTarget
*/
struct PackageTarget {
public:
    /** Enum of supported checksum types.
    * NOTE! This enum guarantee to be sorted by "hash quality"
    */
    enum class ChecksumType {
        UNKNOWN,
        MD5,    /*    The most weakest hash */
        SHA1,   /*  |                       */
        SHA224, /*  |                       */
        SHA256, /*  |                       */
        SHA384, /* \|/                      */
        SHA512, /*    The most secure hash  */
    };

    static ChecksumType checksum_type(const std::string & name);
    static void download_packages(std::vector<PackageTarget *> & targets, bool fail_fast);

    PackageTarget(
        Repo * repo,
        const char * relative_url,
        const char * dest,
        int chks_type,
        const char * chksum,
        int64_t expected_size,
        const char * base_url,
        bool resume,
        int64_t byte_range_start,
        int64_t byte_range_end,
        PackageTargetCB * callbacks);
    PackageTarget(
        ConfigMain * cfg,
        const char * relative_url,
        const char * dest,
        int chks_type,
        const char * chksum,
        int64_t expected_size,
        const char * base_url,
        bool resume,
        int64_t byte_range_start,
        int64_t byte_range_end,
        PackageTargetCB * callbacks,
        const char * http_headers[] = nullptr);
    ~PackageTarget();

    PackageTargetCB * get_callbacks();
    const char * get_err();

private:
    class Impl;
    std::unique_ptr<Impl> p_impl;
};

struct LibrepoLog {
public:
    static long add_handler(const std::string & file_path, bool debug = false);
    static void remove_handler(long uid);
    static void remove_all_handlers();
};

}  // namespace libdnf::rpm

#endif
