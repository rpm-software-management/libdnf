/*
 * Copyright (C) 2017 Red Hat, Inc.
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

#ifndef LIBDNF_SWDB_REPO_HPP
#define LIBDNF_SWDB_REPO_HPP

#include <memory>
#include <string>
#include <unordered_map>

#include "libdnf/utils/sqlite3/sqlite3.hpp"

class Repo {
public:
    static std::shared_ptr<Repo> getCached(SQLite3 & conn, const std::string & repoid);
    static std::unordered_map<std::string, std::shared_ptr<Repo> > cache;

    Repo(SQLite3 & conn);

    int64_t getId() const noexcept { return id; }
    void setId(int64_t value) { id = value; }

    const std::string & getRepoId() const noexcept { return repoId; }
    void setRepoId(const std::string & value) { repoId = value; }

    void save();
    SQLite3 & conn;

protected:
    void dbInsert();
    void dbSelectOrInsert();

    int64_t id = 0;
    std::string repoId;

private:
};

#endif // LIBDNF_SWDB_REPO_HPP
