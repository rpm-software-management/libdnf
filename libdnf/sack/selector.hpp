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

#ifndef __SELECTOR_HPP
#define __SELECTOR_HPP

#include <memory>

#include "../hy-selector.h"
#include "query.hpp"

namespace libdnf {

struct Selector {
public:
    Selector(DnfSack* sack);
    Selector(Selector && src);
    ~Selector();
    DnfSack * getSack();
    const Filter * getFilterArch() const;
    const Filter * getFilterEvr() const;
    const Filter * getFilterFile() const;
    const Filter * getFilterName() const;
    Id getPkgs() const;
    const Filter * getFilterProvides() const;
    const Filter * getFilterReponame() const;
    int set(const DnfPackageSet * pset);
    int set(int keyname, int cmp_type, const char *match);
    GPtrArray * matches();
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}

#endif /* __SELECTOR_HPP */
