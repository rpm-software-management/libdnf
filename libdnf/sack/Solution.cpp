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
    query.reset(new Query(sack));
    nevra.reset();

    if (!with_src) {
        query->addFilter(HY_PKG_ARCH, HY_NEQ, "src");
    }

    Query baseQuery(*query);
    baseQuery.apply();

    if (with_nevra) {
        Nevra nevraObj;
        const HyForm * tryForms = !forms ? HY_FORMS_MOST_SPEC : forms;
        for (std::size_t i = 0; tryForms[i] != _HY_FORM_STOP_; ++i) {
            if (nevraObj.parse(subject, tryForms[i])) {
                query->queryUnion(baseQuery);
                query->addFilter(&nevraObj, icase);
                if (!query->empty()) {
                    nevra.reset(new Nevra(std::move(nevraObj)));
                    return true;
                }
            }
        }
        if (!forms) {
            query->queryUnion(baseQuery);
            query->addFilter(HY_PKG_NEVRA, HY_GLOB, subject);
            if (!query->empty()) {
                return true;
            }
        }
    }

    if (with_provides) {
        query->queryUnion(baseQuery);
        query->addFilter(HY_PKG_PROVIDES, HY_GLOB, subject);
        if (!query->empty()) {
            return true;
        }
    }

    if (with_filenames && hy_is_file_pattern(subject)) {
        query->queryUnion(baseQuery);
        query->addFilter(HY_PKG_FILE, HY_GLOB, subject);
        if (!query->empty()) {
            return true;
        }
    }

    query->addFilter(HY_PKG_EMPTY, HY_EQ, 1);
    return false;
}

}
