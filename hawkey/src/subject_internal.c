/*
 * Copyright (C) 2013 Red Hat, Inc.
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

#include <regex.h>
#include <stdlib.h>
#include "nevra.h"
#include "nevra_internal.h"
#include "iutil.h"
#include "subject_internal.h"

const char *nevra_form_regex[] = {
    "^([^:]+)" "-(([0-9]+):)?" "([^-]*)" "-(.+)" "\\.([^.]+)$",
    "^([^:]+)" "-(([0-9]+):)?" "([^-]*)" "-(.+)" "()$",
    "^([^:]+)" "-(([0-9]+):)?" "([^-]*)" "()" "()$",
    "^([^:]+)" "()()" "()" "()" "\\.([^.]+)$",
    "^([^:]+)()()()()()$"
};

#define MATCH_EMPTY(i) (matches[i].rm_so >= matches[i].rm_eo)

int
nevra_possibility(char *nevra_str, int form, HyNevra nevra)
{
    enum { NAME = 1, EPOCH = 3, VERSION = 4, RELEASE = 5, ARCH = 6 };
    regex_t reg;
    char *epoch = NULL;
    
    regmatch_t matches[10];

    regcomp(&reg, nevra_form_regex[form - 1], REG_EXTENDED);
    int ret = regexec(&reg, nevra_str, 10, matches, 0);
    regfree(&reg);
    if (ret != 0)
    return -1;
    if (!MATCH_EMPTY(EPOCH)) {
    copy_str_from_subexpr(&epoch, nevra_str, matches, EPOCH);
    nevra->epoch = atoi(epoch);
    free(epoch);
    }
    if (copy_str_from_subexpr(&(nevra->name), nevra_str, matches, NAME) == -1)
    return -1;
    copy_str_from_subexpr(&(nevra->version), nevra_str, matches, VERSION);
    copy_str_from_subexpr(&(nevra->release), nevra_str, matches, RELEASE);
    copy_str_from_subexpr(&(nevra->arch), nevra_str, matches, ARCH);
    return 0;
}

#undef MATCH_EMPTY
