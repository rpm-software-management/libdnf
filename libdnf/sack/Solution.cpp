/*
 * Copyright (C) 2018 Red Hat, Inc.
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

#include "Solution.hpp"
#include "../hy-util-private.hpp"

namespace libdnf {

bool
Solution::getBestSolution(const char * subject, DnfSack* sack, HyForm * forms, bool icase,
    bool with_nevra, bool with_provides, bool with_filenames, bool with_src)
{
    nevra.reset();
    Query baseQuery(sack);
    if (!with_src) {
        baseQuery.addFilter(HY_PKG_ARCH, HY_NEQ, "src");
    }
    baseQuery.apply();
    std::unique_ptr<Query> queryCandidate(new Query(baseQuery));
    if (with_nevra) {
        Nevra nevraObj;
        const HyForm * tryForms = !forms ? HY_FORMS_MOST_SPEC : forms;
        for (std::size_t i = 0; tryForms[i] != _HY_FORM_STOP_; ++i) {
            if (nevraObj.parse(subject, tryForms[i])) {
                queryCandidate->queryUnion(baseQuery);
                queryCandidate->addFilter(&nevraObj, icase);
                if (!queryCandidate->empty()) {
                    nevra.reset(new Nevra(std::move(nevraObj)));
                    query = std::move(queryCandidate);
                    return true;
                }
            }
        }
        if (!forms) {
            queryCandidate->queryUnion(baseQuery);
            queryCandidate->addFilter(HY_PKG_NEVRA, HY_GLOB, subject);
            if (!queryCandidate->empty()) {
                query = std::move(queryCandidate);
                return true;
            }
        }
    }

    if (with_provides) {
        queryCandidate->queryUnion(baseQuery);
        queryCandidate->addFilter(HY_PKG_PROVIDES, HY_GLOB, subject);
        if (!queryCandidate->empty()) {
            query = std::move(queryCandidate);
            return true;
        }
    }

    if (with_filenames && hy_is_file_pattern(subject)) {
        queryCandidate->queryUnion(baseQuery);
        queryCandidate->addFilter(HY_PKG_FILE, HY_GLOB, subject);
        if (!queryCandidate->empty()) {
            query = std::move(queryCandidate);
            return true;
        }
    }
    queryCandidate->addFilter(HY_PKG_EMPTY, HY_EQ, 1);
    query = std::move(queryCandidate);
    return false;
}

}
