/*
 * Copyright (C) 2012-2013 Red Hat, Inc.
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

#ifndef HY_STRINGARRAY_H
#define HY_STRINGARRAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "types.h"

void hy_stringarray_free(HyStringArray strs);
int hy_stringarray_length(HyStringArray strs);

#define FOR_STRINGARRAY(str, strs, i)							\
    for (i = 0, str = strs[0]; str; str = strs[++i])

#ifdef __cplusplus
}
#endif

#endif /* HY_STRINGARRAY_H */
