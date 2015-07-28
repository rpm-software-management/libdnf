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

#ifndef HY_REPO_H
#define HY_REPO_H

#ifdef __cplusplus
extern "C" {
#endif

// hawkey
#include "types.h"

enum _hy_repo_param_e {
    HY_REPO_NAME = 0,
    HY_REPO_MD_FN = 1,
    HY_REPO_PRESTO_FN = 2,
    HY_REPO_PRIMARY_FN = 3,
    HY_REPO_FILELISTS_FN = 4,
    HY_REPO_UPDATEINFO_FN = 5
};

HyRepo hy_repo_create(const char *name);
int hy_repo_get_cost(HyRepo repo);
int hy_repo_get_priority(HyRepo repo);
void hy_repo_set_cost(HyRepo repo, int value);
void hy_repo_set_priority(HyRepo repo, int value);
void hy_repo_set_string(HyRepo repo, int which, const char *str_val);
const char *hy_repo_get_string(HyRepo repo, int which);
void hy_repo_free(HyRepo repo);

#ifdef __cplusplus
}
#endif

#endif /* HY_REPO_H */
