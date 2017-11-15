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

GPtrArray *hy_packagelist_create(void);
int hy_packagelist_has(GPtrArray *plist, DnfPackage *pkg);

#endif
