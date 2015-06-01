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

#include <stdlib.h>
#include <fnmatch.h>
#include "reldep.h"
#include "sack_internal.h"
#include "subject.h"
#include "subject_internal.h"
#include "nevra.h"
#include "iutil.h"
#include "nevra_internal.h"
#include "types.h"

// most specific to least
HyForm HY_FORMS_MOST_SPEC[] = {
    HY_FORM_NEVRA, HY_FORM_NEVR, HY_FORM_NEV, HY_FORM_NA, HY_FORM_NAME, -1 };

// what the user most probably means
HyForm HY_FORMS_REAL[] = {
    HY_FORM_NA, HY_FORM_NAME, HY_FORM_NEVRA, HY_FORM_NEV, HY_FORM_NEVR, -1 };

static inline int
is_glob_pattern(char *str)
{
    if (str == NULL)
	return 0;
    while (*str != '\0') {
	if (*str == '*' || *str == '[' || *str == '?')
	    return 1;
	str++;
    }
    return 0;
}

static inline int
is_real_name(HyNevra nevra, HySack sack, int flags)
{
    flags |= HY_NAME_ONLY;
    int glob_version = (flags & HY_GLOB) && is_glob_pattern(nevra->version);
    char *version = nevra->version;
    if (nevra->name == NULL && !glob_version)
	return 1;
    if (glob_version)
	version = NULL;
    if (!is_glob_pattern(nevra->name))
	flags &= ~HY_GLOB;
    if (sack_knows(sack, nevra->name, version, flags) == 0)
	return 0;
    return 1;
}

static inline int
arch_exist(char *arch, const char *existing_arch, int is_glob)
{
    if (is_glob) {
	if (fnmatch(arch, existing_arch, 0) == 0)
	    return 1;
	return 0;
    }
    if (strcmp(arch, existing_arch) == 0)
	return 1;
    return 0;
}

static inline int
is_real_arch(HyNevra nevra, HySack sack, int flags)
{
    int check_glob = (flags & HY_GLOB) && is_glob_pattern(nevra->arch);
    if (nevra->arch == NULL)
	return 1;
    if (arch_exist(nevra->arch, "src", check_glob))
	return 1;
    const char **existing_arches = hy_sack_list_arches(sack);
    int ret = 0;
    for (int i = 0; existing_arches[i] != NULL; ++i) {
	if ((ret = arch_exist(nevra->arch, existing_arches[i], check_glob)))
	    break;
    }
    solv_free(existing_arches);
    return ret;
}

static inline int
filter_real(HyNevra nevra, HySack sack, int flags)
{
    return is_real_name(nevra, sack, flags) &&
	is_real_arch(nevra, sack, flags);
}

HySubject
hy_subject_create(const char * pattern)
{
    return solv_strdup(pattern);
}

void
hy_subject_free(HySubject subject)
{
    solv_free(subject);
}

void
hy_possibilities_free(HyPossibilities iter)
{
    solv_free(iter->subject);
    solv_free(iter->forms);
    solv_free(iter);
}

HyForm *
forms_dup(HyForm *forms)
{
    if (forms == NULL)
	return NULL;
    HyForm *res = NULL;
    const int BLOCK_SIZE = 6;
    HyForm form;
    int i = 0;
    do {
	res = solv_extend(res, i, 1, sizeof(HyForm), BLOCK_SIZE);
	form = forms[i];
	res[i++] = form;
    } while (form != -1);
    return res;
}

static HyPossibilities
possibilities_create(HySubject subject, HyForm *forms,HySack sack, int flags,
    enum poss_type type)
{
    HyPossibilities poss = solv_calloc(1, sizeof(*poss));
    poss->subject = hy_subject_create(subject);
    poss->forms = forms_dup(forms);
    poss->sack = sack;
    poss->flags = flags;
    poss->type = type;
    if (forms == NULL)
	poss->current = -1;
    else
	poss->current = 0;
    return poss;
}

HyPossibilities
hy_subject_reldep_possibilities_real(HySubject subject, HySack sack, int flags)
{
    return possibilities_create(subject, NULL, sack, flags, TYPE_RELDEP_NEW);
}

int hy_possibilities_next_reldep(HyPossibilities iter, HyReldep *out_reldep)
{
    if (iter->type != TYPE_RELDEP_NEW)
	return -1;
    iter->type = TYPE_RELDEP_END;
    char *name, *evr = NULL;
    int cmp_type = 0;
    if (parse_reldep_str(iter->subject, &name, &evr, &cmp_type) == -1)
	return -1;
    if (sack_knows(iter->sack, name, NULL, iter->flags)) {
	*out_reldep = hy_reldep_create(iter->sack, name, cmp_type, evr);
	solv_free(name);
	solv_free(evr);
	if (*out_reldep == NULL)
	    return -1;
	return 0;
    }
    return -1;
}

HyPossibilities
hy_subject_nevra_possibilities(HySubject subject, HyForm *forms)
{
    HyForm *default_forms = forms == NULL ? HY_FORMS_MOST_SPEC : forms;
    return possibilities_create(subject, default_forms, NULL, 0, TYPE_NEVRA);
}

HyPossibilities
hy_subject_nevra_possibilities_real(HySubject subject, HyForm *forms,
    HySack sack, int flags)
{
    HyForm *default_forms = forms == NULL ? HY_FORMS_REAL : forms;
    return possibilities_create(subject, default_forms, sack, flags, TYPE_NEVRA);
}

int
hy_possibilities_next_nevra(HyPossibilities iter, HyNevra *out_nevra)
{
    if (iter->type != TYPE_NEVRA || iter->current == -1)
	return -1;
    HyForm form = iter->forms[iter->current];
    while (form != -1) {
	iter->current++;
	*out_nevra = hy_nevra_create();
	if (nevra_possibility(iter->subject, form, *out_nevra) == 0) {
	    if (iter->sack == NULL)
		return 0;
	    if (filter_real(*out_nevra, iter->sack, iter->flags))
		return 0;
	}
	form = iter->forms[iter->current];
	hy_nevra_free(*out_nevra);
    }
    return -1;
}
