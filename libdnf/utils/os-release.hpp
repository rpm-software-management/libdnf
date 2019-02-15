/*
 * Copyright (C) 2019 Red Hat, Inc.
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

#include <map>
#include <string>
#include <vector>

/**
 * @brief Parse the os-release file and return its contents in a map.
 *
 * @param  paths a vector of os-release file locations to try (in the given order)
 * @return       a map containing os-release data
 */
std::map<std::string, std::string>
getOsReleaseData(const std::vector<std::string> & paths);

/**
 * @brief Construct and return the User-Agent string for libdnf to send in HTTP headers.
 *
 * The format returned is as follows:
 *
 *   libdnf/VERSION (NAME VERSION_ID; [VARIANT_ID; ]OS[.BASEARCH])
 *
 * where VERSION is the libdnf version, NAME, VERSION_ID and VARIANT_ID (if available) are OS
 * identifiers read from the passed os-release data and OS and BASEARCH (if found) are the
 * canonical OS name and base architecture, respectively, detected using RPM.
 *
 * @param  osReleaseData a map containing os-release data
 * @return               a User-Agent string
 */
std::string
getUserAgent(const std::map<std::string, std::string> & osReleaseData);
