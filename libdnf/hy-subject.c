/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-

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
#include "dnf-reldep.h"
#include "dnf-sack-private.h"
#include "hy-subject.h"
#include "hy-subject-private.h"
#include "hy-nevra.h"
#include "hy-iutil.h"
#include "hy-nevra-private.h"
#include "hy-types.h"
#include "hy-query.h"
#include "hy-selector.h"

// most specific to least
HyForm HY_FORMS_MOST_SPEC[] = {
    HY_FORM_NEVRA, HY_FORM_NEVR, HY_FORM_NEV, HY_FORM_NA, HY_FORM_NAME, _HY_FORM_STOP_ };

// what the user most probably means
HyForm HY_FORMS_REAL[] = {
    HY_FORM_NA, HY_FORM_NAME, HY_FORM_NEVRA, HY_FORM_NEV, HY_FORM_NEVR, _HY_FORM_STOP_ };

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
is_real_name(HyNevra nevra, DnfSack *sack, int flags)
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
    if (dnf_sack_knows(sack, nevra->name, version, flags) == 0)
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
is_real_arch(HyNevra nevra, DnfSack *sack, int flags)
{
    int check_glob = (flags & HY_GLOB) && is_glob_pattern(nevra->arch);
    if (nevra->arch == NULL)
        return 1;
    if (arch_exist(nevra->arch, "src", check_glob))
        return 1;
    const char **existing_arches = dnf_sack_list_arches(sack);
    int ret = 0;
    for (int i = 0; existing_arches[i] != NULL; ++i) {
        if ((ret = arch_exist(nevra->arch, existing_arches[i], check_glob)))
            break;
    }
    g_free(existing_arches);
    return ret;
}

static inline int
filter_real(HyNevra nevra, DnfSack *sack, int flags)
{
    return is_real_name(nevra, sack, flags) &&
        is_real_arch(nevra, sack, flags);
}

HySubject
hy_subject_create(const char * pattern)
{
    return g_strdup(pattern);
}

void
hy_subject_free(HySubject subject)
{
    g_free(subject);
}

void
hy_possibilities_free(HyPossibilities iter)
{
    g_free(iter->subject);
    g_free(iter->forms);
    g_free(iter);
}

static HyForm *
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
    } while (form != _HY_FORM_STOP_);
    return res;
}

static HyPossibilities
possibilities_create(HySubject subject, HyForm *forms,DnfSack *sack, int flags,
    enum poss_type type)
{
    HyPossibilities poss = g_malloc0(sizeof(*poss));
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
hy_subject_reldep_possibilities_real(HySubject subject, DnfSack *sack, int flags)
{
    return possibilities_create(subject, NULL, sack, flags, TYPE_RELDEP_NEW);
}

int hy_possibilities_next_reldep(HyPossibilities iter, DnfReldep **out_reldep)
{
    if (iter->type != TYPE_RELDEP_NEW)
        return -1;
    iter->type = TYPE_RELDEP_END;
    char *name, *evr = NULL;
    int cmp_type = 0;
    if (parse_reldep_str(iter->subject, &name, &evr, &cmp_type) == -1)
        return -1;
    if (dnf_sack_knows(iter->sack, name, NULL, iter->flags)) {
        *out_reldep = dnf_reldep_new (iter->sack, name, cmp_type, evr);
        g_free(name);
        g_free(evr);
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
    DnfSack *sack, int flags)
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
    while (form != _HY_FORM_STOP_) {
        iter->current++;
        *out_nevra = hy_nevra_create();
        if (nevra_possibility(iter->subject, form, *out_nevra) == 0) {
            if (iter->sack == NULL)
                return 0;
            if (filter_real(*out_nevra, iter->sack, iter->flags))
                return 0;
        }
        form = iter->forms[iter->current];
        g_clear_pointer(out_nevra, hy_nevra_free);
    }
    return -1;
}

struct NevraToQuery {
    int nevra_type;
    int query_type;
};

/* Given a subject, attempt to create a query choose the first one, and update
 * the query to try to match it.
 * 
 * This code is based on rpm-software-management/dnf/subject.py:get_best_query() at git
 * revision: 1d83fdc0280ca4202281ef489afe600e2f51a32a
 */
HyQuery
hy_subject_get_best_query(HySubject subject, DnfSack *sack, gboolean with_provides)
{
#if 0
    int ret = -1;
    HyNevra nevra = NULL;
    HyPossibilities iter = NULL;
    HyQuery query = NULL;
    DnfReldep *reldep = NULL;

    iter = hy_subject_nevra_possibilities_real(subject, NULL, sack, 0);

    if ((ret = hy_possibilities_next_nevra(iter, &nevra)) < 0)
        goto out;

    if (nevra != NULL) {
        struct NevraToQuery nevraquerymap[] =
            { { HY_NEVRA_NAME, HY_PKG_NAME },
              { HY_NEVRA_VERSION, HY_PKG_VERSION },
              { HY_NEVRA_RELEASE, HY_PKG_RELEASE },
              { HY_NEVRA_ARCH, HY_PKG_ARCH }};

        query = hy_query_create(sack);

        for (guint i = 0; i < G_N_ELEMENTS(nevraquerymap); i++) {
            const char *str = hy_nevra_get_string(nevra, nevraquerymap[i].nevra_type);
            if (str)
                hy_query_filter(query, nevraquerymap[i].query_type, HY_EQ, str);
        }
    } else if (with_provides) {
        hy_possibilities_free(iter);
        iter = hy_subject_reldep_possibilities_real(subject, sack, 0);

        if ((ret = hy_possibilities_next_reldep(iter, &reldep)) < 0)
            goto out;

        query = hy_query_create(sack);
        /* TODO(walters) Do we need to split EVR here? */
        hy_query_filter_provides(query, HY_EQ, dnf_reldep_to_string(reldep), NULL);
    }

    if (query == NULL) {
        query = hy_query_create(sack);
        hy_query_filter_empty(query);
    }
 out:
    if (iter)
        hy_possibilities_free(iter);
    if (nevra)
        hy_nevra_free(nevra);
    g_clear_object(&reldep);
    return query;
#else
    /* At some point, we may use this */
    g_assert_not_reached ();
#endif
}

static HySelector
nevra_to_selector(HyNevra nevra, DnfSack *sack)
{
    HySelector selector = hy_selector_create(sack);

    if (hy_nevra_has_just_name(nevra)) {
        hy_selector_set(selector, HY_PKG_PROVIDES, HY_EQ,
                        hy_nevra_get_string(nevra, HY_NEVRA_NAME));
    } else {
        const char *str = hy_nevra_get_string(nevra, HY_NEVRA_NAME);
        int epoch;

        if (str)
            hy_selector_set(selector, HY_PKG_NAME, HY_EQ, str);

        epoch = hy_nevra_get_epoch(nevra);
        str = hy_nevra_get_string(nevra, HY_NEVRA_VERSION);
        if (str != NULL) {
            g_autoptr(GString) evrbuf = g_string_new (str);

            if (epoch > 0) {
                char buf[G_ASCII_DTOSTR_BUF_SIZE];

                g_ascii_dtostr(buf, sizeof(buf), epoch);
                g_string_prepend(evrbuf, buf);
                g_string_prepend_c(evrbuf, ':');
            }

            str = hy_nevra_get_string(nevra, HY_NEVRA_RELEASE);
            if (!str) {
                hy_selector_set(selector, HY_PKG_VERSION, HY_EQ, evrbuf->str);
            } else {
                g_string_append_c(evrbuf, '-');
                g_string_append(evrbuf, str);
                hy_selector_set(selector, HY_PKG_EVR, HY_EQ, evrbuf->str);
            }
        }
    }

    if (hy_selector_has_matches(selector)) {
        return g_steal_pointer(&selector);
    } else {
        hy_selector_free(selector);
        return NULL;
    }
}

/* Given a subject, attempt to create a "selector".
 * 
 * This code is based on rpm-software-management/dnf/subject.py:get_best_selector() at git
 * revision: 1d83fdc0280ca4202281ef489afe600e2f51a32a
 */
HySelector
hy_subject_get_best_selector(HySubject subject, DnfSack *sack)
{
    HyNevra nevra = NULL;
    HyPossibilities iter = NULL;
    HySelector selector = NULL;
    DnfReldep *reldep = NULL;

    iter = hy_subject_nevra_possibilities_real(subject, NULL, sack, 0);

    if (hy_possibilities_next_nevra(iter, &nevra) == 0 && nevra != NULL) {
        selector = nevra_to_selector(nevra, sack);
    }

    if (selector == NULL) {
        hy_possibilities_free(iter);
        iter = hy_subject_reldep_possibilities_real(subject, sack, 0);

        if (hy_possibilities_next_reldep(iter, &reldep) == 0 && reldep != NULL) {
            selector = hy_selector_create(sack);
            hy_selector_set(selector, HY_PKG_PROVIDES, HY_EQ, dnf_reldep_to_string(reldep));
            if (!hy_selector_has_matches(selector))
                g_clear_pointer(&selector, hy_selector_free);
        }
    }

    /* We don't do globs yet */
    if (g_str_has_prefix (subject, "/")) {
        selector = hy_selector_create(sack);
        hy_selector_set(selector, HY_PKG_FILE, HY_EQ, subject);
    }

    if (selector == NULL)
        selector = hy_selector_create(sack);
    if (iter)
        hy_possibilities_free(iter);
    if (nevra)
        hy_nevra_free(nevra);
    g_clear_object(&reldep);
    return selector;
}

