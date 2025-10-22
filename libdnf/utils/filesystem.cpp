/*
 * Libdnf helper functions for file system operations
 *
 * Copyright (C) 2017-2018 Red Hat, Inc.
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
 * License along with this library; if not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <sys/stat.h>
#include <cerrno>
#include <cstring>

#include "bgettext/bgettext-lib.h"
#include "tinyformat/tinyformat.hpp"
#include "filesystem.hpp"
#include "../error.hpp"

namespace libdnf {

/**
 * Verify if path exists
 * \param path file or directory path
 * \return true if path exists
 */
bool
pathExists(const char *path)
{
    struct stat buffer {
    };
    return stat(path, &buffer) == 0;
}

bool
pathExistsOrException(const std::string & path)
{
    struct stat buffer;
    if (stat(path.c_str(), &buffer) == 0) {
        return true;
    }

    if (errno != ENOENT) {
        throw Error("Failed to access \"" + path + "\": " + strerror(errno));
    }

    return false;
}

/**
 * Create a directory tree for a file.
 * When a path without a file is specified,
 * make sure to include trailing '/'.
 * \param path path to the directory
 */
void
makeDirPath(std::string filePath)
{
    size_t position = 0, previous = 1; // skip leading slash
    while ((position = filePath.find_first_of('/', previous)) != filePath.npos) {
        std::string directory = filePath.substr(0, position++);
        previous = position;
        // create directory if necessary
        if (!pathExists(directory.c_str())) {
            int res = mkdir(directory.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
            if (res != 0 && errno != EEXIST) {
                auto msg = tfm::format(_("Failed to create directory \"%s\": %d - %s"),
                                       directory, errno, strerror(errno));
                throw Error(msg);
            }
        }
    }
}

}
