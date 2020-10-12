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


#include "db.hpp"

#include "libdnf/base/base.hpp"
#include "libdnf/utils/fs.hpp"

#include <filesystem>


namespace libdnf::transaction {


static const char * SQL_1_1_CREATE_TABLES =
#include "sql/1.1_create_tables.sql"
    ;


static const char * SQL_1_2_COMMENT =
#include "sql/1.2_comment.sql"
    ;


static const char * SQL_TABLE_CONFIG_EXISTS = R"**(
    SELECT
        name
    FROM
        sqlite_master
    WHERE
        type='table'
        AND name='config'
)**";


static const char * SQL_GET_SCHEMA_VERSION = R"**(
    SELECT
        value
    FROM
        config
    WHERE
        key = 'version'
)**";


std::string transaction_get_schema_version(libdnf::utils::SQLite3 & conn) {
    std::string result;
    libdnf::utils::SQLite3::Statement query_get_schema_version(conn, SQL_GET_SCHEMA_VERSION);
    if (query_get_schema_version.step() == libdnf::utils::SQLite3::Statement::StepResult::ROW) {
        result = query_get_schema_version.get<std::string>(0);
    } else {
        throw std::runtime_error("Unable to get 'version' from table 'config'");
    }
    return result;
}


void transaction_db_create(libdnf::utils::SQLite3 & conn) {
    // check if table 'config' exists; if not, assume an empty database and create the tables
    libdnf::utils::SQLite3::Statement query_table_config_exists(conn, SQL_TABLE_CONFIG_EXISTS);
    if (query_table_config_exists.step() != libdnf::utils::SQLite3::Statement::StepResult::ROW) {
        conn.exec(SQL_1_1_CREATE_TABLES);
    }

    std::string version = transaction_get_schema_version(conn);

    if (version == "1.1") {
        conn.exec(SQL_1_2_COMMENT);
        version = transaction_get_schema_version(conn);
    }

    // if (version == "1.2") {
    //    conn.exec(SQL_1_3_...);
    //    version = transaction_get_schema_version(conn);
    // }
}


std::unique_ptr<libdnf::utils::SQLite3> transaction_db_connect(libdnf::Base & base) {
    std::string db_path;

    // use db_path based on persistdir
    auto persistdir = base.get_config().persistdir().get_value();
    std::filesystem::path path(persistdir);
    path /= "history.sqlite";
    db_path = path.native();
    libdnf::utils::fs::makedirs_for_file(db_path);

    auto conn = std::make_unique<libdnf::utils::SQLite3>(db_path);
    transaction_db_create(*conn);
    return conn;
}


}  // namespace libdnf::transaction
