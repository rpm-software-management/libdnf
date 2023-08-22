/*
 * Copyright (C) 2023 Red Hat, Inc.
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

#include "Crypto.hpp"
#include "Repo.hpp"

#include "../dnf-utils.h"
#include "bgettext/bgettext-lib.h"
#include "tinyformat/tinyformat.hpp"
#include "utils.hpp"

#include <librepo/librepo.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <glib.h>

#include <utility>

namespace libdnf {

static void throwException(GError * err)
{
    LrException ex(err->code, err->message);
    g_error_free(err);
    throw ex;
}

Key::Key(const void * key, const void * subkey)
    : id{lr_gpg_subkey_get_id(static_cast<const LrGpgSubkey *>(subkey))},
      fingerprint{lr_gpg_subkey_get_fingerprint(static_cast<const LrGpgSubkey *>(subkey))},
      timestamp{lr_gpg_subkey_get_timestamp(static_cast<const LrGpgSubkey *>(subkey))},
      asciiArmoredKey{lr_gpg_key_get_raw_key(static_cast<const LrGpgKey *>(key))}
{
    auto * userid_c = lr_gpg_key_get_userids(static_cast<const LrGpgKey *>(key))[0];
    userid = userid_c ? userid_c : "";
}

const std::string & Key::getId() const noexcept
{
    return id;
}

const std::string & Key::getUserId() const noexcept
{
    return userid;
}

const std::string & Key::getFingerprint() const noexcept
{
    return fingerprint;
}

long int Key::getTimestamp() const noexcept
{
    return timestamp;
}

const std::string & Key::getAsciiArmoredKey() const noexcept
{
    return asciiArmoredKey;
}

const std::string & Key::getUrl() const noexcept
{
    return url;
}

void Key::setUrl(std::string url)
{
    this->url = std::move(url);
}

std::vector<Key> Key::keysFromFd(int fileDescriptor)
{
    std::vector<Key> keyInfos;

    char tmpdir[] = "/tmp/tmpdir.XXXXXX";
    if (!mkdtemp(tmpdir)) {
        const char * errTxt = strerror(errno);
        throw RepoError(tfm::format(_("Cannot create repo temporary directory \"%s\": %s"),
                                      tmpdir, errTxt));
    }
    Finalizer tmpDirRemover([&tmpdir](){
        dnf_remove_recursive(tmpdir, NULL);
    });

    GError * err = NULL;
    if (!lr_gpg_import_key_from_fd(fileDescriptor, tmpdir, &err)) {
        throwException(err);
    }

    std::unique_ptr<LrGpgKey, decltype(&lr_gpg_keys_free)> lr_keys{
        lr_gpg_list_keys(TRUE, tmpdir, &err), &lr_gpg_keys_free};
    if (err) {
        throwException(err);
    }

    for (const auto * lr_key = lr_keys.get(); lr_key; lr_key = lr_gpg_key_get_next(lr_key)) {
        for (const auto * lr_subkey = lr_gpg_key_get_subkeys(lr_key); lr_subkey;
             lr_subkey = lr_gpg_subkey_get_next(lr_subkey)) {
            // get first signing subkey
            if (lr_gpg_subkey_get_can_sign(lr_subkey)) {
                keyInfos.emplace_back(Key(lr_key, lr_subkey));
                break;
            }
        }
    }

    return keyInfos;
}

void importKeyToPubring(const std::string & asciiArmoredKey, const std::string & pubringDir)
{
    GError * err = NULL;
    if (!lr_gpg_import_key_from_memory(asciiArmoredKey.c_str(), asciiArmoredKey.size(), pubringDir.c_str(), &err)) {
        throwException(err);
    }
}

std::vector<std::string> keyidsFromPubring(const std::string & pubringDir)
{
    std::vector<std::string> keyids;

    struct stat sb;
    if (stat(pubringDir.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
        GError * err = NULL;
        std::unique_ptr<LrGpgKey, decltype(&lr_gpg_keys_free)> lr_keys{
            lr_gpg_list_keys(FALSE, pubringDir.c_str(), &err), &lr_gpg_keys_free};
        if (err) {
            throwException(err);
        }

        for (const auto * lr_key = lr_keys.get(); lr_key; lr_key = lr_gpg_key_get_next(lr_key)) {
            for (const auto * lr_subkey = lr_gpg_key_get_subkeys(lr_key); lr_subkey;
                 lr_subkey = lr_gpg_subkey_get_next(lr_subkey)) {
                // get first signing subkey
                if (lr_gpg_subkey_get_can_sign(lr_subkey)) {
                    keyids.emplace_back(lr_gpg_subkey_get_id(lr_subkey));
                    break;
                }
            }
        }
    }

    return keyids;
}

}
