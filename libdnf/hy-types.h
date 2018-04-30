/*
 * Copyright (C) 2012-2014 Red Hat, Inc.
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

#ifndef HY_TYPES_H
#define HY_TYPES_H

#ifdef __cplusplus
namespace libdnf {
    struct Nevra;
    struct Query;
    struct Selector;
}
typedef struct libdnf::Nevra * HyNevra;
typedef struct libdnf::Query * HyQuery;
typedef struct libdnf::Selector * HySelector;
#else
typedef struct Nevra * HyNevra;
typedef struct Query * HyQuery;
typedef struct Selector * HySelector;
#endif

typedef struct _HyRepo * HyRepo;
typedef struct _HyGoal * HyGoal;
typedef char * HySubject;

typedef const unsigned char HyChecksum;

typedef int (*hy_solution_callback)(HyGoal goal, void *callback_data);

#define HY_SYSTEM_REPO_NAME "@System"
#define HY_CMDLINE_REPO_NAME "@commandline"
#define HY_EXT_FILENAMES "-filenames"
#define HY_EXT_UPDATEINFO "-updateinfo"
#define HY_EXT_PRESTO "-presto"

enum _hy_key_name_e {
    HY_PKG = 0,
    HY_PKG_ALL = 1, /* DEPRECATED, used only to make empty query. Replaced by HY_PKG_EMPTY */
    HY_PKG_ARCH = 2,
    HY_PKG_CONFLICTS = 3,
    HY_PKG_DESCRIPTION = 4,
    HY_PKG_EPOCH = 5,
    HY_PKG_EVR = 6,
    HY_PKG_FILE = 7,
    HY_PKG_NAME = 8,
    HY_PKG_NEVRA = 9,
    HY_PKG_OBSOLETES = 10,
    HY_PKG_PROVIDES = 11,
    HY_PKG_RELEASE = 12,
    HY_PKG_REPONAME = 13,
    HY_PKG_REQUIRES = 14,
    HY_PKG_SOURCERPM = 15,
    HY_PKG_SUMMARY = 16,
    HY_PKG_URL = 17,
    HY_PKG_VERSION = 18,
    HY_PKG_LOCATION = 19,
    HY_PKG_ENHANCES = 20,
    HY_PKG_RECOMMENDS = 21,
    HY_PKG_SUGGESTS = 22,
    HY_PKG_SUPPLEMENTS = 23,
    HY_PKG_ADVISORY = 24,
    HY_PKG_ADVISORY_BUG = 25,
    HY_PKG_ADVISORY_CVE = 26,
    HY_PKG_ADVISORY_SEVERITY = 27,
    HY_PKG_ADVISORY_TYPE = 28,
    HY_PKG_DOWNGRADABLE = 29,
    HY_PKG_DOWNGRADES = 30,
    HY_PKG_EMPTY = 31,
    HY_PKG_LATEST_PER_ARCH = 32,
    HY_PKG_LATEST = 33,
    HY_PKG_UPGRADABLE = 34,
    HY_PKG_UPGRADES = 35,
    /**
    * @brief Use for strings of whole NEVRA (missing epoch is handled as epoch 0)
    * Allowed compare types - only HY_EQ or HY_NEQ
    */
    HY_PKG_NEVRA_STRICT = 36
};

enum _hy_comparison_type_e {
    /* part 1: flags that mix with all types */
    HY_ICASE  = 1 << 0,
    HY_NOT    = 1 << 1,
    HY_COMPARISON_FLAG_MASK = HY_ICASE | HY_NOT,

    /* part 2: comparison types that mix with each other */
    HY_EQ        = (1 << 8),
    HY_LT        = (1 << 9),
    HY_GT        = (1 << 10),

    /* part 3: comparison types that only make sense for strings */
    HY_SUBSTR        = (1 << 11),
    HY_GLOB     = (1 << 12),

    /* part 4: frequently used combinations */
    HY_NEQ        = HY_EQ | HY_NOT,

    /* part 5: additional flags, not necessarily used for queries */
    HY_NAME_ONLY = (1 << 16),
};

#endif /* HY_TYPES_H */
