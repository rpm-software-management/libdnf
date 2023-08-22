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

#ifndef _LIBDNF_CRYPTO_HPP
#define _LIBDNF_CRYPTO_HPP

#include <string>
#include <vector>

namespace libdnf {

/**
* @class Key
*
* @brief The class represents information about an OpenPGP key
*/
class Key {
public:
    /** Loads keys from a file descriptor. Expects ASCII Armored format. */
    static std::vector<Key> keysFromFd(int fileDescriptor);

    /** Returns the key ID */
    const std::string & getId() const noexcept;

    /** Returns the user ID of the key */
    const std::string & getUserId() const noexcept;

    /** Returns key fingerprint */
    const std::string & getFingerprint() const noexcept;

    /** Returns the key timestamp */
    long int getTimestamp() const noexcept;

    /** Returns ASCII Armored OpenPGP key */
    const std::string & getAsciiArmoredKey() const noexcept;

    /** Returns the key source url */
    const std::string & getUrl() const noexcept;

    /** Sets the key source url */
    void setUrl(std::string url);

private:
    Key(const void * key, const void * subkey);

    std::string id;
    std::string fingerprint;
    std::string userid;
    long int timestamp;
    std::string asciiArmoredKey;
    std::string url;
};

/**
* @brief Imports ASCII Armored OpenPGP key to pubring
*
* @param asciiArmoredKey   ASCII Armored OpenPGP key
* @param pubringDir        path to the pubring directory
*/
void importKeyToPubring(const std::string & asciiArmoredKey, const std::string & pubringDir);

/**
* @brief Retrieves the IDs of the keys in the pubring
*
* @param pubringDir   path to the pubring directory
*
* @return             List of key IDs in the pubrig
*/
std::vector<std::string> keyidsFromPubring(const std::string & pubringDir);

}

#endif
