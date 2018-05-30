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

#include "hy-util.h"
#include <glib.h>

#ifndef HY_UTIL_PRIVATE_HPP
#define HY_UTIL_PRIVATE_HPP

gboolean hy_is_glob_pattern(const char *pattern);

/**
 * @brief Test if pattern is file path
 *
 * @param pattern Strig to analyze
 * @return gboolean Return TRUE if pattern start with "/" or pattern[0] == '*' && pattern[1] == '/'
 */
static inline gboolean hy_is_file_pattern(const char *pattern)
{
    return pattern[0] == '/' || (pattern[0] == '*' && pattern[1] == '/');
}

// see http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetTable
static const unsigned char _BitCountLookup[256] =
{
#   define B2(n) n,     n+1,     n+1,     n+2
#   define B4(n) B2(n), B2(n+1), B2(n+1), B2(n+2)
#   define B6(n) B4(n), B4(n+1), B4(n+1), B4(n+2)
    B6(0), B6(1), B6(1), B6(2)
};

inline size_t
map_count(Map *m)
{
    unsigned char *ti = m->map;
    unsigned char *end = ti + m->size;
    unsigned c = 0;

    while (ti < end)
        c += _BitCountLookup[*ti++];

    return c;
}

GPtrArray *hy_packagelist_create(void);
int hy_packagelist_has(GPtrArray *plist, DnfPackage *pkg);
int mtime(const char *filename);

#endif
