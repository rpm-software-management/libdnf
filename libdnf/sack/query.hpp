/*
 * Copyright (C) 2017 Red Hat, Inc.
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

#include <memory>
#include <vector>
#include "../hy-query-private.hpp"
#include "../dnf-types.h"

struct Filter {
public:
    Filter(int keyname, int cmp_type, int match);
    Filter(int keyname, int cmp_type, int nmatches, const int *matches);
    Filter(int keyname, int cmp_type, DnfPackageSet *pset);
    Filter(int keyname, int cmp_type, DnfReldep *reldep);
    Filter(int keyname, int cmp_type, DnfReldepList *reldeplist);
    Filter(int keyname, int cmp_type, const char *match);
    Filter(int keyname, int cmp_type, const char **matches);
    ~Filter();
    const int getKeyname() const noexcept;
    const int getCmpType() const noexcept;
    const int getMatchType() const noexcept;
    const std::vector<_Match> & getMatches() const noexcept;
private:
    class Impl;
    std::shared_ptr<Impl> pImpl;
};

struct Query {
public:
    Query(const Query & query_src);
    Query(Query && query_src) = delete;
    Query(DnfSack* sack, int flags = 0);
    ~Query();
    Query & operator=(const Query& query_src);
    Query & operator=(Query && src_query) = delete;
    Map * getResult() noexcept;
    const Map * getResult() const noexcept;
    DnfSack * getSack();
    bool getApplied();
    void clear();
    size_t size();
    int addFilter(int keyname, int cmp_type, int match);
    int addFilter(int keyname, int cmp_type, int nmatches, const int *matches);
    int addFilter(int keyname, int cmp_type, DnfPackageSet *pset);
    int addFilter(int keyname, DnfReldep *reldep);
    int addFilter(int keyname, DnfReldepList *reldeplist);
    int addFilter(int keyname, int cmp_type, const char *match);
    int addFilter(int keyname, int cmp_type, const char **matches);
    int addFilter(HyNevra nevra, gboolean icase);
    void apply();
    GPtrArray * run();
    DnfPackageSet * runSet();
    Id getIndexItem(int index);
    void queryUnion(Query other);
    void queryIntersection(Query other);
    void queryDifference(Query other);
    bool empty();
    void filterExtras();
    void filterRecent(const long unsigned int recent_limit);
    void filterDuplicated();
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};


