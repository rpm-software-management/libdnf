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

#ifndef __SOLUTION_HPP
#define __SOLUTION_HPP

#include <memory>
#include "query.hpp"
#include "../nevra.hpp"
#include "../hy-subject.h"

namespace libdnf {

struct Solution {
public:
    const Nevra * getNevra() const noexcept;
    const Query * getQuery() const noexcept;
    Nevra * releaseNevra();
    Query * releaseQuery();
    /**
    * @brief Return true if solution found
    */
    bool getBestSolution(const char * subject, DnfSack* sack, HyForm * forms, bool icase,
        bool with_nevra, bool with_provides, bool with_filenames, bool with_src);
private:
    std::unique_ptr<Query> query;
    std::unique_ptr<Nevra> nevra;
};

inline const Nevra * Solution::getNevra() const noexcept { return nevra.get(); }
inline const Query * Solution::getQuery() const noexcept { return query.get(); }
inline Nevra * Solution::releaseNevra() { return nevra.release(); }
inline Query * Solution::releaseQuery() { return query.release(); }

}

#endif /* __SOLUTION_HPP */
