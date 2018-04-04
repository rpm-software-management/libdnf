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
#include "dnf-sack-private.hpp"
#include "hy-subject.h"
#include "hy-subject-private.hpp"
#include "hy-module-form.h"
#include "hy-iutil-private.hpp"
#include "hy-nevra.hpp"
#include "hy-module-form-private.hpp"
#include "hy-types.h"
#include "hy-query-private.hpp"
#include "hy-selector.h"
#include "hy-util-private.hpp"
#include "sack/packageset.hpp"

// most specific to least
const HyForm HY_FORMS_MOST_SPEC[] = {
    HY_FORM_NEVRA, HY_FORM_NA, HY_FORM_NAME, HY_FORM_NEVR, HY_FORM_NEV, _HY_FORM_STOP_ };

const HyModuleFormEnum HY_MODULE_FORMS_MOST_SPEC[] = {
        HY_MODULE_FORM_NSVCAP,
        HY_MODULE_FORM_NSVCA,
        HY_MODULE_FORM_NSVAP,
        HY_MODULE_FORM_NSVA,
        HY_MODULE_FORM_NSAP,
        HY_MODULE_FORM_NSA,
        HY_MODULE_FORM_NSVCP,
        HY_MODULE_FORM_NSVP,
        HY_MODULE_FORM_NSVC,
        HY_MODULE_FORM_NSV,
        HY_MODULE_FORM_NSP,
        HY_MODULE_FORM_NS,
        HY_MODULE_FORM_NAP,
        HY_MODULE_FORM_NA,
        HY_MODULE_FORM_NP,
        HY_MODULE_FORM_N,
        _HY_MODULE_FORM_STOP_};

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
    g_free(iter->module_forms);
    g_free(iter);
}

static HyForm *
forms_dup(const HyForm *forms)
{
    if (forms == NULL)
        return NULL;
    HyForm *res = NULL;
    const int BLOCK_SIZE = 6;
    HyForm form;
    int i = 0;
    do {
        res = static_cast<HyForm *>(solv_extend(res, i, 1, sizeof(HyForm), BLOCK_SIZE));
        form = forms[i];
        res[i++] = form;
    } while (form != _HY_FORM_STOP_);
    return res;
}

static HyModuleFormEnum *
module_forms_dup(const HyModuleFormEnum *forms)
{
    if (forms == NULL)
        return NULL;
    HyModuleFormEnum *res = NULL;
    const int BLOCK_SIZE = 17;
    HyModuleFormEnum form;
    int i = 0;
    do {
        res = static_cast<HyModuleFormEnum *>(solv_extend(res, i, 1, sizeof(HyModuleFormEnum), BLOCK_SIZE));
        form = forms[i];
        res[i++] = form;
    } while (form != _HY_MODULE_FORM_STOP_);
    return res;
}

static HyPossibilities
possibilities_create(HySubject subject, const HyForm *forms, const HyModuleFormEnum *module_forms,
                     enum poss_type type)
{
    HyPossibilities poss = static_cast<HyPossibilities>(g_malloc0(sizeof(*poss)));
    poss->subject = hy_subject_create(subject);
    poss->forms = forms_dup(forms);
    poss->module_forms = module_forms_dup(module_forms);
    poss->type = type;
    if (forms == NULL && module_forms == NULL)
        poss->current = -1;
    else
        poss->current = 0;
    return poss;
}

HyPossibilities
hy_subject_nevra_possibilities(HySubject subject, HyForm *forms)
{
    const HyForm *default_forms = forms == NULL ? HY_FORMS_MOST_SPEC : forms;
    return possibilities_create(subject, default_forms, NULL, TYPE_NEVRA);
}

HyPossibilities
hy_subject_module_form_possibilities(HySubject subject, HyModuleFormEnum *forms)
{
    const HyModuleFormEnum *default_forms = forms == NULL ? HY_MODULE_FORMS_MOST_SPEC : forms;
    return possibilities_create(subject, NULL, default_forms, TYPE_MODULE_FORM);
}

int
hy_possibilities_next_nevra(HyPossibilities iter, HyNevra *out_nevra)
{
    if (iter->type != TYPE_NEVRA || iter->current == -1)
        return -1;
    HyForm form = iter->forms[iter->current];
    libdnf::Nevra nevra;
    while (form != _HY_FORM_STOP_) {
        iter->current++;
        if (nevra.parse(iter->subject, form)) {
            *out_nevra = new libdnf::Nevra(std::move(nevra));
            return 0;
        }
        form = iter->forms[iter->current];
    }
    *out_nevra = nullptr;
    return -1;
}

int
hy_possibilities_next_module_form(HyPossibilities iter, HyModuleForm *out_module_form)
{
    if (iter->type != TYPE_MODULE_FORM || iter->current == -1)
        return -1;
    HyModuleFormEnum form = iter->module_forms[iter->current];
    while (form != _HY_MODULE_FORM_STOP_) {
        iter->current++;
        *out_module_form = hy_module_form_create();
        if (module_form_possibility(iter->subject, form, *out_module_form) == 0) {
            return 0;
        }
        form = iter->module_forms[iter->current];
        g_clear_pointer(out_module_form, hy_module_form_free);
    }
    return -1;
}

/* Given a subject, attempt to create a query choose the first one, and update
 * the query to try to match it.
 *
 * This code is based on rpm-software-management/dnf/subject.py:get_best_query() at git
 * revision: 1d83fdc0280ca4202281ef489afe600e2f51a32a
 */
HyQuery
hy_subject_get_best_solution(HySubject subject, DnfSack *sack, HyForm *forms, HyNevra *nevra,
                             gboolean icase, gboolean with_nevra, gboolean with_provides,
                             gboolean with_filenames)
{
    int ret = 0;
    HyQuery query = NULL;

    if (with_nevra) {
        HyPossibilities iter = hy_subject_nevra_possibilities(subject, forms);
        while (ret != -1) {
            ret = hy_possibilities_next_nevra(iter, nevra);
            if (ret != -1) {
                query = hy_query_from_nevra(*nevra, sack, icase);
                if (!hy_query_is_empty(query)) {
                    hy_possibilities_free(iter);
                    return query;
                }
                hy_query_free(query);
            }
        }
        hy_possibilities_free(iter);
        delete *nevra;
        *nevra = nullptr;
        if (!forms) {
            query = hy_query_create(sack);
            hy_query_filter(query, HY_PKG_NEVRA, HY_GLOB, subject);
            if (!hy_query_is_empty(query))
                return query;
            hy_query_free(query);
        }
    }
    if (with_provides) {
        query = hy_query_create(sack);
        hy_query_filter(query, HY_PKG_PROVIDES, HY_GLOB, subject);
        if (!hy_query_is_empty(query))
            return query;
        hy_query_free(query);
    }

    if (with_filenames && hy_is_file_pattern(subject)) {
        query = hy_query_create(sack);
        hy_query_filter(query, HY_PKG_FILE, HY_GLOB, subject);
        return query;
    }

    query = hy_query_create(sack);
    hy_query_filter_empty(query);
    return query;
}


HySelector
hy_subject_get_best_sltr(HySubject subject, DnfSack *sack, HyForm *forms, bool obsoletes,
                         const char *reponame)
{
    HyNevra nevra{nullptr};
    HyQuery query = hy_subject_get_best_solution(subject, sack, forms, &nevra, FALSE, TRUE, TRUE,
                                                 TRUE);
    if (!hy_query_is_empty(query)) {
        hy_query_filter(query, HY_PKG_ARCH, HY_NEQ, "src");
        if (obsoletes && nevra && nevra->hasJustName()) {
            DnfPackageSet *pset;
            pset = hy_query_run_set(query);
            HyQuery query_obsoletes = hy_query_clone(query);
            hy_query_filter_package_in(query_obsoletes, HY_PKG_OBSOLETES, HY_EQ, pset);
            delete pset;
            hy_query_union(query, query_obsoletes);
            hy_query_free(query_obsoletes);
        }
        if (reponame != NULL) {
            HyQuery installed_query = hy_query_clone(query);
            hy_query_filter(installed_query, HY_PKG_REPONAME, HY_EQ, HY_SYSTEM_REPO_NAME);
            hy_query_filter(query, HY_PKG_REPONAME, HY_EQ, reponame);
            hy_query_union(query, installed_query);
            hy_query_free(installed_query);
        }
    }
    delete nevra;
    HySelector selector = hy_query_to_selector(query);
    hy_query_free(query);
    return selector;
}

/* Given a subject, attempt to create a "selector".
 *
 */
HySelector
hy_subject_get_best_selector(HySubject subject, DnfSack *sack)
{
    return hy_subject_get_best_sltr(subject, sack, NULL, FALSE, NULL);
}
