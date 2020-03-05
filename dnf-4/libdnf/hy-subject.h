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

#ifndef HY_SUBJECT_H
#define HY_SUBJECT_H

#include <stdlib.h>
#include <solv/util.h>
#include "dnf-types.h"
#include "hy-types.h"

#include <glib.h>
#include <stdbool.h>

G_BEGIN_DECLS

typedef enum _HyForm {
    HY_FORM_NEVRA = 1,
    HY_FORM_NEVR = 2,
    HY_FORM_NEV = 3,
    HY_FORM_NA = 4,
    HY_FORM_NAME = 5,
    _HY_FORM_STOP_ = -1
} HyForm;

typedef enum _HyModuleForm {
    HY_MODULE_FORM_NSVCAP = 1,
    HY_MODULE_FORM_NSVCA = 2,
    HY_MODULE_FORM_NSVAP = 3,
    HY_MODULE_FORM_NSVA = 4,
    HY_MODULE_FORM_NSAP = 5,
    HY_MODULE_FORM_NSA = 6,
    HY_MODULE_FORM_NSVCP = 7,
    HY_MODULE_FORM_NSVP = 8,
    HY_MODULE_FORM_NSVC = 9,
    HY_MODULE_FORM_NSV = 10,
    HY_MODULE_FORM_NSP = 11,
    HY_MODULE_FORM_NS = 12,
    HY_MODULE_FORM_NAP = 13,
    HY_MODULE_FORM_NA = 14,
    HY_MODULE_FORM_NP = 15,
    HY_MODULE_FORM_N = 16,
    _HY_MODULE_FORM_STOP_ = -1
} HyModuleForm;

extern const HyForm HY_FORMS_MOST_SPEC[];
extern const HyModuleForm HY_MODULE_FORMS_MOST_SPEC[];

HySubject hy_subject_create(const char * pattern);
void hy_subject_free(HySubject subject);

HyQuery hy_subject_get_best_solution(HySubject subject, DnfSack *sack, HyForm *forms,
                                     HyNevra *out_nevra, gboolean icase, gboolean with_nevra,
                                     gboolean with_provides, gboolean with_filenames,
                                     gboolean with_src);
HyQuery hy_subject_get_best_query(HySubject subject, DnfSack *sack, gboolean with_provides);

/**
* @brief Returns HySelector with packages that represents subject. Subject can be NEVRA, provide,
* or file provide. Additionally result can be enrich for obsoletes if subject is NEVRA with just
* name.
*
* @param subject subject
* @param sack DnfSack
* @param forms HyForm *forms or NULL
* @param obsoletes If TRUE, obsoletes will be added to result
* @param reponame Id of repo
* @param with_src Include source packages
* @return HySelector
*/
HySelector hy_subject_get_best_selector(HySubject subject, DnfSack *sack, HyForm *forms,
    bool obsoletes, const char *reponame);

G_END_DECLS

G_DEFINE_AUTO_CLEANUP_FREE_FUNC(HySubject, hy_subject_free, NULL)

#endif
