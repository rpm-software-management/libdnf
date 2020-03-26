/*
Copyright (C) 2020 Red Hat, Inc.

This file is part of microdnf: https://github.com/rpm-software-management/libdnf/

Microdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Microdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with microdnf.  If not, see <https://www.gnu.org/licenses/>.
*/

// Jaroslav Rohel
// It is code from my C++ dnf development version. It will be replaced.

#include "utils.hpp"

#include <fcntl.h>
#include <glob.h>
#include <pwd.h>
#include <rpm/rpmdb.h>
#include <rpm/rpmlib.h>
#include <rpm/rpmmacro.h>
#include <rpm/rpmts.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <stdexcept>

namespace microdnf {

bool am_i_root() {
    return geteuid() == 0;
}

bool is_directory(const char * path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return false;
    }
    return S_ISDIR(statbuf.st_mode);
}

void ensure_dir(const std::string & dname) {
    auto ret = mkdir(dname.c_str(), 0755);
    if (ret == -1) {
        auto err_code = errno;
        if (err_code != EEXIST || !is_directory(dname.c_str())) {
            char err_buf[1024];
            strerror_r(err_code, err_buf, sizeof(err_buf));
            throw std::runtime_error(err_buf);
        }
    }
}

std::string normalize_time(time_t timestamp) {
    char buf[128];
    if (strftime(buf, sizeof(buf), "%c", localtime(&timestamp)) == 0)
        return "";
    return buf;
}

// ================================================
// Next utils probably will be move to libdnf

/* return a path to a valid and safe cachedir - only used when not running
   as root or when --tempcache is set */
std::string get_cache_dir() {
    constexpr const char * base_prefix = "dnf";
    constexpr const char * tmpdir = "/var/tmp/";

    uid_t uid = geteuid();
    auto pw = getpwuid(uid);
    std::string prefix = std::string(base_prefix) + "-";
    if (pw) {
        prefix += pw->pw_name;
    } else {
        prefix += std::to_string(uid);
    }
    prefix += "-";

    // check for /var/tmp/prefix-* -
    std::string dirpath = tmpdir + prefix + "*";
    std::string cachedir;

    /*for (auto & dentry : std::filesystem::directory_iterator(tmpdir)) {
        auto & path = dentry.path().string();
        if (path.compare(0, dirpath.size(), dirpath) == 0 && dentry.is_directory() && dentry.status().permissions() == std::perm::owner_all &&
            
        ) {
            cachedir = path;
            break;
        }
    }*/

    glob_t glob_buf;
    glob(dirpath.c_str(), GLOB_MARK, nullptr, &glob_buf);
    for (size_t i = 0; i < glob_buf.gl_pathc; ++i) {
        auto path = glob_buf.gl_pathv[i];
        struct stat stat_buf;
        if (lstat(path, &stat_buf) == 0) {
            if (S_ISDIR(stat_buf.st_mode) && ((stat_buf.st_mode & 0x1FF) == 448) && stat_buf.st_uid == uid)
                cachedir = path;
        }
    }
    globfree(&glob_buf);

    // make the dir (tempfile.mkdtemp())
    if (cachedir.empty()) {
        cachedir = tmpdir + prefix + "XXXXXX";
        mkdtemp(&cachedir.front());
        // std::cout << "Generate new: " << cachedir << std::endl;
    }

    return cachedir;
}

#define MAX_NATIVE_ARCHES 12

/* data taken from DNF */
static const struct {
    const char * base;
    const char * native[MAX_NATIVE_ARCHES];
} ARCH_MAP[] = {{"aarch64", {"aarch64", nullptr}},
                {"alpha",
                 {"alpha",
                  "alphaev4",
                  "alphaev45",
                  "alphaev5",
                  "alphaev56",
                  "alphaev6",
                  "alphaev67",
                  "alphaev68",
                  "alphaev7",
                  "alphapca56",
                  nullptr}},
                {"arm", {"armv5tejl", "armv5tel", "armv5tl", "armv6l", "armv7l", "armv8l", nullptr}},
                {"armhfp", {"armv7hl", "armv7hnl", "armv8hl", "armv8hnl", "armv8hcnl", nullptr}},
                {"i386", {"i386", "athlon", "geode", "i386", "i486", "i586", "i686", nullptr}},
                {"ia64", {"ia64", nullptr}},
                {"mips", {"mips", nullptr}},
                {"mipsel", {"mipsel", nullptr}},
                {"mips64", {"mips64", nullptr}},
                {"mips64el", {"mips64el", nullptr}},
                {"noarch", {"noarch", nullptr}},
                {"ppc", {"ppc", nullptr}},
                {"ppc64", {"ppc64", "ppc64iseries", "ppc64p7", "ppc64pseries", nullptr}},
                {"ppc64le", {"ppc64le", nullptr}},
                {"riscv32", {"riscv32", nullptr}},
                {"riscv64", {"riscv64", nullptr}},
                {"riscv128", {"riscv128", nullptr}},
                {"s390", {"s390", nullptr}},
                {"s390x", {"s390x", nullptr}},
                {"sh3", {"sh3", nullptr}},
                {"sh4", {"sh4", "sh4a", nullptr}},
                {"sparc", {"sparc", "sparc64", "sparc64v", "sparcv8", "sparcv9", "sparcv9v", nullptr}},
                {"x86_64", {"x86_64", "amd64", "ia32e", nullptr}},
                {nullptr, {nullptr}}};

/* find the base architecture */
const char * get_base_arch(const char * arch) {
    for (int i = 0; ARCH_MAP[i].base; ++i) {
        for (int j = 0; ARCH_MAP[i].native[j]; ++j) {
            if (strcmp(ARCH_MAP[i].native[j], arch) == 0) {
                return ARCH_MAP[i].base;
            }
        }
    }
    return nullptr;
}

static void init_lib_rpm() {
    static bool lib_rpm_initiated{false};
    if (!lib_rpm_initiated) {
        if (rpmReadConfigFiles(nullptr, nullptr) != 0) {
            throw std::runtime_error("failed to read rpm config files\n");
        }
        lib_rpm_initiated = true;
    }
}

//#define RELEASEVER_PROV "system-release(releasever)"

static constexpr const char * DISTROVERPKGS[] = {"system-release(releasever)",
                                                 "system-release",
                                                 "distribution-release(releasever)",
                                                 "distribution-release",
                                                 "redhat-release",
                                                 "suse-release"};

std::string detect_arch() {
    init_lib_rpm();
    const char * value;
    rpmGetArchInfo(&value, nullptr);
    return value;
}

/**
 * dnf_context_set_os_release:
 **/
std::string detect_release(const std::string & install_root_path) {
    init_lib_rpm();
    std::string release_ver;

    bool found_in_rpmdb{false};
    auto ts = rpmtsCreate();
    rpmtsSetRootDir(ts, install_root_path.c_str());
    for (auto distroverpkg : DISTROVERPKGS) {
        auto mi = rpmtsInitIterator(ts, RPMTAG_PROVIDENAME, distroverpkg, 0);
        while (auto hdr = rpmdbNextIterator(mi)) {
            auto version = headerGetString(hdr, RPMTAG_VERSION);
            rpmds ds = rpmdsNew(hdr, RPMTAG_PROVIDENAME, 0);
            while (rpmdsNext(ds) >= 0) {
                if (strcmp(rpmdsN(ds), distroverpkg) == 0 && rpmdsFlags(ds) == RPMSENSE_EQUAL) {
                    version = rpmdsEVR(ds);
                }
            }
            release_ver = version;
            found_in_rpmdb = true;
            rpmdsFree(ds);
            break;
        }
        rpmdbFreeIterator(mi);
        if (found_in_rpmdb) {
            break;
        }
    }
    rpmtsFree(ts);
    if (found_in_rpmdb) {
        return release_ver;
    }
    return "";
}

}  // namespace microdnf
