/*
 * Copyright (C) 2013-2017 Red Hat, Inc.
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

#include <stdio.h>
#include <regex.h>
#include <stdlib.h>
#include "hy-nevra.hpp"
#include "hy-module-form.h"
#include "hy-module-form-private.hpp"
#include "hy-iutil-private.hpp"
#include "hy-subject-private.hpp"

#define MODULE_NAME "([-a-zA-Z0-9\\._]+)"
#define MODULE_STREAM MODULE_NAME
#define MODULE_VERSION "([0-9]+)"
#define MODULE_CONTEXT "([0-9a-f]+)"
#define MODULE_ARCH MODULE_NAME
#define MODULE_PROFILE MODULE_NAME

const char *module_form_regex[] = {
    "^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION ":" MODULE_CONTEXT "::?" MODULE_ARCH "\\/"  MODULE_PROFILE "$",
    "^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION ":" MODULE_CONTEXT "::?" MODULE_ARCH "\\/?" "()"           "$",
    "^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION     "()"           "::"  MODULE_ARCH "\\/"  MODULE_PROFILE "$",
    "^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION     "()"           "::"  MODULE_ARCH "\\/?" "()"           "$",
    "^" MODULE_NAME ":" MODULE_STREAM     "()"               "()"           "::"  MODULE_ARCH "\\/"  MODULE_PROFILE "$",
    "^" MODULE_NAME ":" MODULE_STREAM     "()"               "()"           "::"  MODULE_ARCH "\\/?" "()"           "$",
    "^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION ":" MODULE_CONTEXT       "()"        "\\/"  MODULE_PROFILE "$",
    "^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION     "()"                 "()"        "\\/"  MODULE_PROFILE "$",
    "^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION ":" MODULE_CONTEXT       "()"        "\\/?" "()"           "$",
    "^" MODULE_NAME ":" MODULE_STREAM ":" MODULE_VERSION     "()"                 "()"        "\\/?" "()"           "$",
    "^" MODULE_NAME ":" MODULE_STREAM     "()"               "()"                 "()"        "\\/"  MODULE_PROFILE "$",
    "^" MODULE_NAME ":" MODULE_STREAM     "()"               "()"                 "()"        "\\/?" "()"           "$",
    "^" MODULE_NAME     "()"              "()"               "()"           "::"  MODULE_ARCH "\\/"  MODULE_PROFILE "$",
    "^" MODULE_NAME     "()"              "()"               "()"           "::"  MODULE_ARCH "\\/?" "()"           "$",
    "^" MODULE_NAME     "()"              "()"               "()"                 "()"        "\\/"  MODULE_PROFILE "$",
    "^" MODULE_NAME     "()"              "()"               "()"                 "()"        "\\/?" "()"           "$"
};

#define MATCH_EMPTY(i) (matches[i].rm_so >= matches[i].rm_eo)

int module_form_possibility(char *module_form_str, int form, HyModuleForm module_form)
{
    enum {
        NAME = 1,
        STREAM = 2,
        VERSION = 3,
        CONTEXT = 4,
        ARCH = 5,
        PROFILE = 6
    };

    regex_t regex;
    regmatch_t matches[10];
    char *version = NULL;

    regcomp(&regex, module_form_regex[form - 1], REG_EXTENDED);

    if (regexec(&regex, module_form_str, 10, matches, 0) != 0) {
        regfree(&regex);
        return -1;
    }

    if (copy_str_from_subexpr(&(module_form->name), module_form_str, matches, NAME) == -1)
        return -1;

    if (!MATCH_EMPTY(VERSION)) {
        copy_str_from_subexpr(&version, module_form_str, matches, VERSION);
        module_form->version = atoll(version);
        free(version);
    }

    copy_str_from_subexpr(&(module_form->stream), module_form_str, matches, STREAM);
    copy_str_from_subexpr(&(module_form->context), module_form_str, matches, CONTEXT);
    copy_str_from_subexpr(&(module_form->arch), module_form_str, matches, ARCH);
    copy_str_from_subexpr(&(module_form->profile), module_form_str, matches, PROFILE);

    regfree(&regex);
    return 0;
}

#undef MATCH_EMPTY
