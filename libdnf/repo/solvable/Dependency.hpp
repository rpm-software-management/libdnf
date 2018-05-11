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

#ifndef LIBDNF_DEPENDENCY_HPP
#define LIBDNF_DEPENDENCY_HPP

#include <memory>
#include <string>
#include <vector>
#include <solv/knownid.h>

#include "libdnf/dnf-sack.h"

struct Dependency
{
public:
    Dependency(DnfSack *sack, Id id);
    Dependency(DnfSack *sack, const char *name, const char *version, int cmpType);
    Dependency(DnfSack *sack, const std::string &dependency);
    Dependency(const Dependency &dependency);
    ~Dependency();

    const char *getName() const;
    const char *getRelation() const;
    const char *getVersion() const;
    const char *toString() const;
    Id getId() const noexcept;

private:
    friend DependencyContainer;
    static Id getReldepId(DnfSack *sack, const char *name, const char *version, int cmpType);
    static Id getReldepId(DnfSack *sack, const char * reldepStr);

    DnfSack *sack;
    Id id;
};

inline Id Dependency::getId() const noexcept { return id; }

#endif //LIBDNF_DEPENDENCY_HPP
