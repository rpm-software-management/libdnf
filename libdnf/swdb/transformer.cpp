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

#include "transformer.hpp"
#include "swdb.hpp"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <stdexcept>
#include <string>
#include <vector>

Transformer::Transformer (const std::string &outputFile, const std::string &inputDir)
  : inputDir (inputDir)
  , outputFile (outputFile)
  , transformFile (outputFile + ".transform")
{
}

void
Transformer::transform ()
{
    // perform transformation
    // (block limits lifetime of the database objects)
    {
        SQLite3 swdb (transformFile.c_str ());
        SQLite3 history (historyPath ().c_str ());

        // create a new database file
        SwdbCreateDatabase (swdb);

        // transform objects
        transformRPMItems (swdb, history);
        transformTrans (swdb, history);
        transformTransWith (swdb, history);
        transformOutput (swdb, history);
        transformTransItems (swdb, history);
    }

    // rename temporary file to the final one
    int result = rename (transformFile.c_str (), outputFile.c_str ());
    if (result != 0) {
        throw Exception (std::strerror (errno));
    }
}

void
Transformer::transformRPMItems (SQLite3 &swdb, SQLite3 &history)
{
    // history.pkgtups
}

void
Transformer::transformTrans (SQLite3 &swdb, SQLite3 &history)
{
    // history.(trans_beg, trans_end, trans_cmdline)
}

void
Transformer::transformTransWith (SQLite3 &swdb, SQLite3 &history)
{
    // history.trans_with_pkgs
}

void
Transformer::transformOutput (SQLite3 &swdb, SQLite3 &history)
{
    // history.trans_script_stdout
    // history.trans_error
}

void
Transformer::transformTransItems (SQLite3 &swdb, SQLite3 &history)
{
    // history.trans_data_pkgs
    // probably pkg_yumdb for 'reason'
}

std::string
Transformer::historyPath ()
{
    std::string historyDir (inputDir);

    // construct the history directory path
    if (historyDir.back () != '/') {
        historyDir += '/';
    }
    historyDir += "history";

    // vector for possible history DB files
    std::vector<std::string> possibleFiles;

    // open history directory
    struct dirent *dp;
    DIR *dirp = opendir (historyDir.c_str ());

    // iterate over history directory
    while ((dp = readdir (dirp)) != nullptr) {
        std::string fileName (dp->d_name);

        // push the possible history DB files into vector
        if (fileName.find ("history") != std::string::npos) {
            possibleFiles.push_back (fileName);
        }
    }

    if (possibleFiles.empty ()) {
        throw Exception ("Couldn't find a history database");
    }

    // find the latest DB file
    std::sort (possibleFiles.begin (), possibleFiles.end ());

    // return the path
    return historyDir + "/" + possibleFiles.back ();
}
