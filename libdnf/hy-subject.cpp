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
#include "hy-iutil-private.hpp"
#include "nevra.hpp"
#include "nsvcap.hpp"
#include "hy-types.h"
#include "hy-query-private.hpp"
#include "hy-selector.h"
#include "hy-util-private.hpp"
#include "sack/packageset.hpp"

// most specific to least
const HyForm HY_FORMS_MOST_SPEC[] = {
    HY_FORM_NEVRA, HY_FORM_NA, HY_FORM_NAME, HY_FORM_NEVR, HY_FORM_NEV, _HY_FORM_STOP_ };

const HyModuleForm HY_MODULE_FORMS_MOST_SPEC[] = {
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

/* Given a subject, attempt to create a query choose the first one, and update
 * the query to try to match it.
 *
 * This code is based on rpm-software-management/dnf/subject.py:get_best_query() at git
 * revision: 1d83fdc0280ca4202281ef489afe600e2f51a32a
 *
 * Since then, it was amended to add a `with_src` flag to avoid finding source packages
 * from provides.
 */
HyQuery
hy_subject_get_best_solution(HySubject subject, DnfSack *sack, HyForm *forms, HyNevra *out_nevra,
                             gboolean icase, gboolean with_nevra, gboolean with_provides,
                             gboolean with_filenames, gboolean with_src)
{
    if (with_nevra) {
        libdnf::Nevra nevraObj;
        const auto tryForms = !forms ? HY_FORMS_MOST_SPEC : forms;
        for (std::size_t i = 0; tryForms[i] != _HY_FORM_STOP_; ++i) {
            if (nevraObj.parse(subject, tryForms[i])) {
                auto query = hy_query_from_nevra(&nevraObj, sack, icase);
                if (!with_src)
                    hy_query_filter(query, HY_PKG_ARCH, HY_NEQ, "src");
                if (!hy_query_is_empty(query)) {
                    *out_nevra = new libdnf::Nevra(std::move(nevraObj));
                    return query;
                }
                hy_query_free(query);
            }
        }
        *out_nevra = nullptr;
        if (!forms) {
            auto query = hy_query_create(sack);
            if (!with_src)
                hy_query_filter(query, HY_PKG_ARCH, HY_NEQ, "src");
            hy_query_filter(query, HY_PKG_NEVRA, HY_GLOB, subject);
            if (!hy_query_is_empty(query))
                return query;
            hy_query_free(query);
        }
    }

    if (with_provides) {
        auto query = hy_query_create(sack);
        if (!with_src)
            hy_query_filter(query, HY_PKG_ARCH, HY_NEQ, "src");
        hy_query_filter(query, HY_PKG_PROVIDES, HY_GLOB, subject);
        if (!hy_query_is_empty(query))
            return query;
        hy_query_free(query);
    }

    if (with_filenames && hy_is_file_pattern(subject)) {
        auto query = hy_query_create(sack);
        if (!with_src)
            hy_query_filter(query, HY_PKG_ARCH, HY_NEQ, "src");
        hy_query_filter(query, HY_PKG_FILE, HY_GLOB, subject);
        return query;
    }

    auto query = hy_query_create(sack);
    hy_query_filter_empty(query);
    return query;
}


HySelector
hy_subject_get_best_sltr(HySubject subject, DnfSack *sack, HyForm *forms, bool obsoletes,
                         const char *reponame,
                         bool with_src)
{
    HyNevra nevra{nullptr};
    HyQuery query = hy_subject_get_best_solution(subject, sack, forms, &nevra, FALSE, TRUE, TRUE,
                                                 TRUE, with_src);
    if (!hy_query_is_empty(query)) {
        // This inversion is because in the !with_src case, we already
        // filtered above.
        if (with_src)
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
hy_subject_get_best_selector(HySubject subject, DnfSack *sack, bool with_src)
{
    return hy_subject_get_best_sltr(subject, sack, NULL, FALSE, NULL, with_src);
}
