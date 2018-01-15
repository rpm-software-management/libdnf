/*
 * Copyright (C) 2012-2014 Red Hat, Inc.
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

#include <glib.h>
#include <assert.h>
#include <fnmatch.h>
#include <string.h>
#include <solv/evr.h>
#include <solv/repo.h>
#include <solv/solver.h>
#include <solv/util.h>

#include "dnf-types.h"
#include "dnf-advisorypkg.h"
#include "dnf-advisory-private.hpp"
#include "hy-goal-private.hpp"
#include "hy-iutil-private.hpp"
#include "hy-nevra.hpp"
#include "hy-query-private.hpp"
#include "hy-package-private.hpp"
#include "hy-packageset-private.hpp"
#include "hy-selector.h"
#include "dnf-reldep-private.hpp"
#include "dnf-sack-private.hpp"
#include "hy-util-private.hpp"

#define BLOCK_SIZE 15

static int
match_type_num(int keyname) {
    switch (keyname) {
    case HY_PKG_EMPTY:
    case HY_PKG_EPOCH:
    case HY_PKG_LATEST:
    case HY_PKG_LATEST_PER_ARCH:
    case HY_PKG_UPGRADABLE:
    case HY_PKG_UPGRADES:
    case HY_PKG_DOWNGRADABLE:
    case HY_PKG_DOWNGRADES:
        return 1;
    default:
        return 0;
    }
}

static int
match_type_pkg(int keyname) {
    switch (keyname) {
    case HY_PKG:
    case HY_PKG_OBSOLETES:
        return 1;
    default:
        return 0;
    }
}

static int
match_type_reldep(int keyname) {
    switch (keyname) {
    case HY_PKG_CONFLICTS:
    case HY_PKG_ENHANCES:
    case HY_PKG_OBSOLETES:
    case HY_PKG_PROVIDES:
    case HY_PKG_RECOMMENDS:
    case HY_PKG_REQUIRES:
    case HY_PKG_SUGGESTS:
    case HY_PKG_SUPPLEMENTS:
        return 1;
    default:
        return 0;
    }
}

static int
match_type_str(int keyname) {
    switch (keyname) {
    case HY_PKG_ADVISORY:
    case HY_PKG_ADVISORY_BUG:
    case HY_PKG_ADVISORY_CVE:
    case HY_PKG_ADVISORY_SEVERITY:
    case HY_PKG_ADVISORY_TYPE:
    case HY_PKG_ARCH:
    case HY_PKG_DESCRIPTION:
    case HY_PKG_ENHANCES:
    case HY_PKG_EVR:
    case HY_PKG_FILE:
    case HY_PKG_LOCATION:
    case HY_PKG_NAME:
    case HY_PKG_NEVRA:
    case HY_PKG_PROVIDES:
    case HY_PKG_RECOMMENDS:
    case HY_PKG_RELEASE:
    case HY_PKG_REPONAME:
    case HY_PKG_REQUIRES:
    case HY_PKG_SOURCERPM:
    case HY_PKG_SUGGESTS:
    case HY_PKG_SUMMARY:
    case HY_PKG_SUPPLEMENTS:
    case HY_PKG_OBSOLETES:
    case HY_PKG_CONFLICTS:
    case HY_PKG_URL:
    case HY_PKG_VERSION:
        return 1;
    default:
        return 0;
    }
}


static Id
di_keyname2id(int keyname)
{
    switch(keyname) {
    case HY_PKG_DESCRIPTION:
        return SOLVABLE_DESCRIPTION;
    case HY_PKG_NAME:
        return SOLVABLE_NAME;
    case HY_PKG_URL:
        return SOLVABLE_URL;
    case HY_PKG_ARCH:
        return SOLVABLE_ARCH;
    case HY_PKG_EVR:
        return SOLVABLE_EVR;
    case HY_PKG_SUMMARY:
        return SOLVABLE_SUMMARY;
    case HY_PKG_FILE:
        return SOLVABLE_FILELIST;
    default:
        assert(0);
        return 0;
    }
}

static Id
reldep_keyname2id(int keyname)
{
    switch(keyname) {
    case HY_PKG_CONFLICTS:
        return SOLVABLE_CONFLICTS;
    case HY_PKG_ENHANCES:
        return SOLVABLE_ENHANCES;
    case HY_PKG_OBSOLETES:
        return SOLVABLE_OBSOLETES;
    case HY_PKG_REQUIRES:
        return SOLVABLE_REQUIRES;
    case HY_PKG_RECOMMENDS:
        return SOLVABLE_RECOMMENDS;
    case HY_PKG_SUGGESTS:
        return SOLVABLE_SUGGESTS;
    case HY_PKG_SUPPLEMENTS:
        return SOLVABLE_SUPPLEMENTS;
    default:
        assert(0);
        return 0;
    }
}

static int
type2flags(int type, int keyname)
{
    int ret = 0;
    if (keyname == HY_PKG_FILE)
        ret |= SEARCH_FILES | SEARCH_COMPLETE_FILELIST;
    if (type & HY_ICASE)
        ret |= SEARCH_NOCASE;

    type &= ~HY_COMPARISON_FLAG_MASK;
    switch (type) {
    case HY_EQ:
        return ret | SEARCH_STRING;
    case HY_SUBSTR:
        return ret | SEARCH_SUBSTRING;
    case HY_GLOB:
        return ret | SEARCH_GLOB;
    default:
        assert(0); // not implemented
        return 0;
    }
}

static int
valid_filter_str(int keyname, int cmp_type)
{
    if (!match_type_str(keyname))
        return 0;

    cmp_type &= ~HY_NOT; // hy_query_run always handles NOT
    switch (keyname) {
    case HY_PKG_LOCATION:
    case HY_PKG_SOURCERPM:
        return cmp_type == HY_EQ;
    case HY_PKG_ARCH:
        return cmp_type & HY_EQ || cmp_type & HY_GLOB;
    case HY_PKG_NAME:
        return cmp_type & HY_EQ || cmp_type & HY_GLOB || cmp_type & HY_SUBSTR;
    default:
        return 1;
    }
}

static int
valid_filter_num(int keyname, int cmp_type)
{
    if (!match_type_num(keyname))
        return 0;

    cmp_type &= ~HY_NOT; // hy_query_run always handles NOT
    if (cmp_type & (HY_ICASE | HY_SUBSTR | HY_GLOB))
        return 0;
    switch (keyname) {
    case HY_PKG:
        return cmp_type == HY_EQ;
    default:
        return 1;
    }
}

static int
valid_filter_pkg(int keyname, int cmp_type)
{
    if (!match_type_pkg(keyname))
        return 0;
    return cmp_type == HY_EQ || cmp_type == HY_NEQ;
}

static int
valid_filter_reldep(int keyname)
{
    if (!match_type_reldep(keyname))
        return 0;
    return 1;
}

static char *
pool_solvable_epoch_optional_2str(Pool *pool, const Solvable *s, gboolean with_epoch)
{
    const char *e;
    const char *name = pool_id2str(pool, s->name);
    const char *evr = pool_id2str(pool, s->evr);
    const char *arch = pool_id2str(pool, s->arch);
    gboolean present_epoch = FALSE;

    for (e = evr + 1; *e != '-' && *e != '\0'; ++e) {
        if (*e == ':') {
            present_epoch = TRUE;
            break;
        }
    }
    char *output_string;
    int evr_length, arch_length;
    int extra_epoch_length = 0;
    int name_length = strlen(name);
    evr_length = strlen(evr);
    arch_length = strlen(arch);
    if (!present_epoch && with_epoch) {
        extra_epoch_length = 2;
    } else if (present_epoch && !with_epoch) {
        extra_epoch_length = evr - e - 1;
    }

    output_string = pool_alloctmpspace(
        pool, name_length + evr_length + extra_epoch_length + arch_length + 3);

    strcpy(output_string, name);

    if (evr_length || extra_epoch_length > 0) {
        output_string[name_length++] = '-';

        if (extra_epoch_length > 0) {
            output_string[name_length++] = '0';
            output_string[name_length++] = ':';
            output_string[name_length] = '\0';
        }
    }

    if (evr_length) {
        if (extra_epoch_length >= 0) {
            strcpy(output_string + name_length, evr);
        } else {
            strcpy(output_string + name_length, evr - extra_epoch_length);
            evr_length = evr_length + extra_epoch_length;
        }
    }

    if (arch_length) {
        output_string[name_length + evr_length] = '.';
        strcpy(output_string + name_length + evr_length + 1, arch);
    }
    return output_string;
}

struct _Filter *
filter_create(int nmatches)
{
    auto f = static_cast<_Filter *>(g_malloc0(sizeof(struct _Filter)));
    filter_reinit(f, nmatches);
    return f;
}

void
filter_reinit(struct _Filter *f, int nmatches)
{
    for (int m = 0; m < f->nmatches; ++m)
        switch (f->match_type) {
        case _HY_PKG:
            g_object_unref(f->matches[m].pset);
            break;
        case _HY_STR:
            g_free(f->matches[m].str);
            break;
        case _HY_RELDEP:
            if (f->matches[m].reldep)
                g_object_unref (f->matches[m].reldep);
            break;
        default:
            break;
        }
    g_free(f->matches);
    f->match_type = _HY_VOID;
    if (nmatches > 0)
        f->matches = static_cast<_Match *>(g_malloc0(nmatches * sizeof(union _Match *)));
    else
        f->matches = NULL;
    f->nmatches = nmatches;
}

void
filter_free(struct _Filter *f)
{
    if (f) {
        filter_reinit(f, 0);
        g_free(f);
    }
}

static struct _Filter *
query_add_filter(HyQuery q, int nmatches)
{
    struct _Filter filter;
    memset(&filter, 0, sizeof(filter));
    filter_reinit(&filter, nmatches);
    q->filters = static_cast<_Filter *>(solv_extend(q->filters, q->nfilters,
                                                    1, sizeof(filter), BLOCK_SIZE));
    q->filters[q->nfilters] = filter; /* structure assignment */
    return q->filters + q->nfilters++;
}

Id
query_get_index_item(HyQuery query, int index)
{
    Pool *pool = dnf_sack_get_pool(query->sack);
    int index_counter = 0;

    hy_query_apply(query);

    for (Id id = 1; id < pool->nsolvables; ++id) {
        if (!MAPTST(query->result, id))
            continue;
        if (index_counter == index)
            return id;
        ++index_counter;
    }
    return 0;
}

static void
filter_dataiterator(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = dnf_sack_get_pool(q->sack);
    Dataiterator di;
    Id keyname = di_keyname2id(f->keyname);
    int flags = type2flags(f->cmp_type, f->keyname);

    assert(f->match_type == _HY_STR);

    for (int mi = 0; mi < f->nmatches; ++mi) {
        const char *match = f->matches[mi].str;
        for (Id id = 1; id < pool->nsolvables; ++id) {
            if (!MAPTST(q->result, id))
                continue;
            dataiterator_init(&di, pool, 0, id, keyname, match, flags);
            while (dataiterator_step(&di))
                MAPSET(m, di.solvid);
            dataiterator_free(&di);
        }
    }
}

static void
filter_pkg(HyQuery q, struct _Filter *f, Map *m)
{
    assert(f->nmatches == 1);
    assert(f->match_type == _HY_PKG);

    map_free(m);
    map_init_clone(m, dnf_packageset_get_map(f->matches[0].pset));
}

static void
filter_name(HyQuery q, const struct _Filter *f, Map *m)
{
    Pool *pool = dnf_sack_get_pool(q->sack);
    Id match_name_id = 0;
    for (int mi = 0; mi < f->nmatches; ++mi) {
        const char *match = f->matches[mi].str;
        if ((f->cmp_type & HY_EQ) && !(f->cmp_type & HY_ICASE)) {
            match_name_id = pool_str2id(pool, match, 0);
            if (match_name_id == 0)
                continue;
        }

        for (Id id = 1; id < pool->nsolvables; ++id) {
            if (!MAPTST(q->result, id))
                continue;

            Solvable *s = pool_id2solvable(pool, id);

            if (f->cmp_type & HY_ICASE) {
                const char *name = pool_id2str(pool, s->name);
                if (f->cmp_type & HY_SUBSTR) {
                    if (strcasestr(name, match) != NULL)
                        MAPSET(m, id);
                    continue;
                }
                if (f->cmp_type & HY_EQ) {
                    if (strcasecmp(name, match) == 0)
                        MAPSET(m, id);
                    continue;
                }
                if (f->cmp_type & HY_GLOB) {
                    if (fnmatch(match, name, FNM_CASEFOLD) == 0)
                        MAPSET(m, id);
                    continue;
                }
                continue;
            }

            if (f->cmp_type & HY_EQ) {
                if (match_name_id == s->name)
                    MAPSET(m, id);
                continue;
            }
            const char *name = pool_id2str(pool, s->name);
            if (f->cmp_type & HY_GLOB) {
                if (fnmatch(match, name, 0) == 0)
                    MAPSET(m, id);
                continue;
            }
            if (f->cmp_type & HY_SUBSTR) {
                if (strstr(name, match) != NULL)
                    MAPSET(m, id);
                continue;
            }
        }
    }
}

static void
filter_epoch(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = dnf_sack_get_pool(q->sack);

    for (int mi = 0; mi < f->nmatches; ++mi) {
        unsigned long epoch = f->matches[mi].num;

        for (Id id = 1; id < pool->nsolvables; ++id) {
            if (!MAPTST(q->result, id))
                continue;
            Solvable *s = pool_id2solvable(pool, id);
            if (s->evr == ID_EMPTY)
                continue;

            const char *evr = pool_id2str(pool, s->evr);
            unsigned long pkg_epoch = pool_get_epoch(pool, evr);

            int cmp_type = f->cmp_type;
            if ((pkg_epoch > epoch && cmp_type & HY_GT) ||
                (pkg_epoch < epoch && cmp_type & HY_LT) ||
                (pkg_epoch == epoch && cmp_type & HY_EQ))
                MAPSET(m, id);
        }
    }
}

static void
filter_evr(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = dnf_sack_get_pool(q->sack);

    for (int mi = 0; mi < f->nmatches; ++mi) {
        Id match_evr = pool_str2id(pool, f->matches[mi].str, 1);

        for (Id id = 1; id < pool->nsolvables; ++id) {
            if (!MAPTST(q->result, id))
                continue;
            Solvable *s = pool_id2solvable(pool, id);
            int cmp = pool_evrcmp(pool, s->evr, match_evr, EVRCMP_COMPARE);

            if ((cmp > 0 && f->cmp_type & HY_GT) ||
                (cmp < 0 && f->cmp_type & HY_LT) ||
                (cmp == 0 && f->cmp_type & HY_EQ)) {
                MAPSET(m, id);
            }
        }
    }
}

static void
filter_version(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = dnf_sack_get_pool(q->sack);
    int cmp_type = f->cmp_type;

    for (int mi = 0; mi < f->nmatches; ++mi) {
        const char *match = f->matches[mi].str;
        char *filter_vr = solv_dupjoin(match, "-0", NULL);

        for (Id id = 1; id < pool->nsolvables; ++id) {
            if (!MAPTST(q->result, id))
                continue;
            char *e, *v, *r;
            Solvable *s = pool_id2solvable(pool, id);
            if (s->evr == ID_EMPTY)
                continue;
            const char *evr = pool_id2str(pool, s->evr);

            pool_split_evr(pool, evr, &e, &v, &r);

            if (cmp_type == HY_GLOB) {
                if (fnmatch(match, v, 0))
                    continue;
                MAPSET(m, id);
            }

            char *vr = pool_tmpjoin(pool, v, "-0", NULL);
            int cmp = pool_evrcmp_str(pool, vr, filter_vr, EVRCMP_COMPARE);
            if ((cmp > 0 && cmp_type & HY_GT) ||
                (cmp < 0 && cmp_type & HY_LT) ||
                (cmp == 0 && cmp_type & HY_EQ)) {
                MAPSET(m, id);
            }
        }
        g_free(filter_vr);
    }
}

static void
filter_release(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = dnf_sack_get_pool(q->sack);
    int cmp_type = f->cmp_type;

    for (int mi = 0; mi < f->nmatches; ++mi) {
        const char *match = f->matches[mi].str;
        char *filter_vr = solv_dupjoin("0-", f->matches[mi].str, NULL);

        for (Id id = 1; id < pool->nsolvables; ++id) {
            if (!MAPTST(q->result, id))
                continue;
            char *e, *v, *r;
            Solvable *s = pool_id2solvable(pool, id);
            if (s->evr == ID_EMPTY)
                continue;
            const char *evr = pool_id2str(pool, s->evr);

            pool_split_evr(pool, evr, &e, &v, &r);

            if (cmp_type & HY_GLOB) {
                if (fnmatch(match, r, 0))
                    continue;
                MAPSET(m, id);
            }

            char *vr = pool_tmpjoin(pool, "0-", r, NULL);

            int cmp = pool_evrcmp_str(pool, vr, filter_vr, EVRCMP_COMPARE);

            if ((cmp > 0 && f->cmp_type & HY_GT) ||
                (cmp < 0 && f->cmp_type & HY_LT) ||
                (cmp == 0 && f->cmp_type & HY_EQ)) {
                MAPSET(m, id);
            }
        }
        g_free(filter_vr);
    }
}

static void
filter_arch(HyQuery q, const struct _Filter *f, Map *m)
{
    Pool *pool = dnf_sack_get_pool(q->sack);
    Id match_arch_id = 0;
    for (int mi = 0; mi < f->nmatches; ++mi) {
        const char *match = f->matches[mi].str;
        if (f->cmp_type & HY_EQ) {
            match_arch_id = pool_str2id(pool, match, 0);
            if (match_arch_id == 0)
                continue;
        }

        for (Id id = 1; id < pool->nsolvables; ++id) {
            if (!MAPTST(q->result, id))
                continue;
            Solvable *s = pool_id2solvable(pool, id);
            if (f->cmp_type & HY_EQ) {
                if (match_arch_id == s->arch)
                    MAPSET(m, id);
                continue;
            }
            const char *arch = pool_id2str(pool, s->arch);
            if (f->cmp_type & HY_GLOB) {
                if (fnmatch(match, arch, 0) == 0)
                    MAPSET(m, id);
                continue;
            }
        }
    }
}

static void
filter_sourcerpm(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = dnf_sack_get_pool(q->sack);

    for (int mi = 0; mi < f->nmatches; ++mi) {
        const char *match = f->matches[mi].str;

        for (Id id = 1; id < pool->nsolvables; ++id) {
            if (!MAPTST(q->result, id))
                continue;
            Solvable *s = pool_id2solvable(pool, id);

            const char *name = solvable_lookup_str(s, SOLVABLE_SOURCENAME);
            if (name == NULL)
                name = pool_id2str(pool, s->name);
            if (!g_str_has_prefix(match, name)) // early check
                continue;

            DnfPackage *pkg = dnf_package_new(q->sack, id);
            const char *srcrpm = dnf_package_get_sourcerpm(pkg);
            if (srcrpm && !strcmp(match, srcrpm))
                MAPSET(m, id);
            g_object_unref(pkg);
        }
    }
}

static void
filter_obsoletes(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = dnf_sack_get_pool(q->sack);
    int obsprovides = pool_get_flag(pool, POOL_FLAG_OBSOLETEUSESPROVIDES);
    Map *target;

    assert(f->match_type == _HY_PKG);
    assert(f->nmatches == 1);
    target = dnf_packageset_get_map(f->matches[0].pset);
    dnf_sack_make_provides_ready(q->sack);
    for (Id p = 1; p < pool->nsolvables; ++p) {
        if (!MAPTST(q->result, p))
            continue;
        Solvable *s = pool_id2solvable(pool, p);
        if (!s->repo)
            continue;
        for (Id *r_id = s->repo->idarraydata + s->obsoletes; *r_id; ++r_id) {
            Id r, rr;

            FOR_PROVIDES(r, rr, *r_id) {
                if (!MAPTST(target, r))
                    continue;
                assert(r != SYSTEMSOLVABLE);
                Solvable *so = pool_id2solvable(pool, r);
                if (!obsprovides && !pool_match_nevr(pool, so, *r_id))
                    continue; /* only matching pkg names */
                MAPSET(m, p);
                break;
            }
        }
    }
}

static void
filter_provides_reldep(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = dnf_sack_get_pool(q->sack);
    Id p, pp;

    dnf_sack_make_provides_ready(q->sack);
    for (int i = 0; i < f->nmatches; ++i) {
        Id r_id = dnf_reldep_get_id (f->matches[i].reldep);
        FOR_PROVIDES(p, pp, r_id)
            MAPSET(m, p);
    }
}

static void
filter_rco_reldep(HyQuery q, struct _Filter *f, Map *m)
{
    assert(f->match_type == _HY_RELDEP);

    Pool *pool = dnf_sack_get_pool(q->sack);
    Id rco_key = reldep_keyname2id(f->keyname);
    Queue rco;

    queue_init(&rco);
    for (int i = 0; i < f->nmatches; ++i) {
        Id r_id = dnf_reldep_get_id (f->matches[i].reldep);

        for (Id s_id = 1; s_id < pool->nsolvables; ++s_id) {
            if (!MAPTST(q->result, s_id))
                continue;

            Solvable *s = pool_id2solvable(pool, s_id);

            queue_empty(&rco);
            solvable_lookup_idarray(s, rco_key, &rco);
            for (int j = 0; j < rco.count; ++j) {
                Id r_id2 = rco.elements[j];

                if (pool_match_dep(pool, r_id, r_id2)) {
                    MAPSET(m, s_id);
                    break;
                }
            }
        }
    }
    queue_free(&rco);
}

static void
filter_reponame(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = dnf_sack_get_pool(q->sack);
    int i;
    Solvable *s;
    Repo *r;
    Id id, ourids[pool->nrepos];

    for (id = 0; id < pool->nrepos; ++id)
        ourids[id] = 0;
    FOR_REPOS(id, r) {
        for (i = 0; i < f->nmatches; i++) {
            if (!strcmp(r->name, f->matches[i].str)) {
                ourids[id] = 1;
                break;
            }
        }
    }

    for (i = 1; i < pool->nsolvables; ++i) {
        if (!MAPTST(q->result, i))
            continue;
        s = pool_id2solvable(pool, i);
        switch (f->cmp_type & ~HY_COMPARISON_FLAG_MASK) {
        case HY_EQ:
            if (s->repo && ourids[s->repo->repoid])
                MAPSET(m, i);
            break;
        default:
            assert(0);
        }
    }
}

static void
filter_location(HyQuery q, struct _Filter *f, Map *m)
{
    Pool *pool = dnf_sack_get_pool(q->sack);

    for (int mi = 0; mi < f->nmatches; ++mi) {
        const char *match = f->matches[mi].str;

        for (Id id = 1; id < pool->nsolvables; ++id) {
            if (!MAPTST(q->result, id))
                continue;
            Solvable *s = pool_id2solvable(pool, id);

            const char *location = solvable_get_location(s, NULL);
            if (location == NULL)
                continue;
            if (!strcmp(match, location))
                MAPSET(m, id);
        }
    }
}

static void
filter_nevra(HyQuery q, const struct _Filter *f, Map *m)
{
    Pool *pool = dnf_sack_get_pool(q->sack);
    int fn_flags = (HY_ICASE & f->cmp_type) ? FNM_CASEFOLD : 0;

    for (int mi = 0; mi < f->nmatches; ++mi) {
        const char *nevra_pattern = f->matches[mi].str;
        if (strpbrk(nevra_pattern, "(/=<> "))
            continue;

        gboolean present_epoch = strchr(nevra_pattern, ':') != NULL;

        for (Id id = 1; id < pool->nsolvables; ++id) {
            if (!MAPTST(q->result, id))
                continue;
            Solvable* s = pool_id2solvable(pool, id);

            char* nevra = pool_solvable_epoch_optional_2str(pool, s, present_epoch);
            if (!(HY_GLOB & f->cmp_type)) {
                if (HY_ICASE & f->cmp_type) {
                    if (strcasecmp(nevra_pattern, nevra) == 0)
                        MAPSET(m, id);
                } else {
                    if (strcmp(nevra_pattern, nevra) == 0)
                        MAPSET(m, id);
                }
            } else if (fnmatch(nevra_pattern, nevra, fn_flags) == 0) {
                MAPSET(m, id);
            }
        }
    }
}

static void
filter_updown(HyQuery q, const struct _Filter *f, Map *m)
{
    DnfSack *sack = q->sack;
    Pool *pool = dnf_sack_get_pool(sack);

    dnf_sack_make_provides_ready(q->sack);

    if (!pool->installed) {
        return;
    }

    for (int mi = 0; mi < f->nmatches; ++mi) {
        if (f->matches[mi].num == 0)
            continue;

        for (Id id = 1; id < pool->nsolvables; ++id) {
            if (!MAPTST(q->result, id))
                continue;
            Solvable *s = pool_id2solvable(pool, id);
            if (s->repo == pool->installed)
                continue;
            if (f->keyname == HY_PKG_DOWNGRADES) {
                if (what_downgrades(pool, id) > 0)
                    MAPSET(m, id);
            } else if (what_upgrades(pool, id) > 0)
                MAPSET(m, id);
        }
    }
}

static void
filter_updown_able(HyQuery q, const struct _Filter *f, Map *m)
{
    Id p, what;
    Solvable *s;
    Pool *pool = dnf_sack_get_pool(q->sack);

    dnf_sack_make_provides_ready(q->sack);

    if (!pool->installed) {
        return;
    }
    for (int mi = 0; mi < f->nmatches; ++mi) {
        if (f->matches[mi].num == 0)
            continue;

        FOR_PKG_SOLVABLES(p) {
            s = pool_id2solvable(pool, p);
            if (s->repo == pool->installed)
                continue;

            what = (f->keyname == HY_PKG_DOWNGRADABLE) ? what_downgrades(pool, p) :
                what_upgrades(pool, p);
            if (what != 0 && map_tst(q->result, what))
                map_set(m, what);
        }
    }
}

static int
filter_latest_sortcmp(const void *ap, const void *bp, void *dp)
{
    auto pool = static_cast<Pool *>(dp);
    Solvable *sa = pool->solvables + *(Id *)ap;
    Solvable *sb = pool->solvables + *(Id *)bp;
    int r;
    r = sa->name - sb->name;
    if (r)
        return r;
    r = pool_evrcmp(pool, sb->evr, sa->evr, EVRCMP_COMPARE);
    if (r)
        return r;
    return *(Id *)ap - *(Id *)bp;
}

static int
filter_latest_sortcmp_byarch(const void *ap, const void *bp, void *dp)
{
    auto pool = static_cast<Pool *>(dp);
    Solvable *sa = pool->solvables + *(Id *)ap;
    Solvable *sb = pool->solvables + *(Id *)bp;
    int r;
    r = sa->name - sb->name;
    if (r)
        return r;
    r = sa->arch - sb->arch;
    if (r)
        return r;
    r = pool_evrcmp(pool, sb->evr, sa->evr, EVRCMP_COMPARE);
    if (r)
        return r;
    return *(Id *)ap - *(Id *)bp;
}

static void
add_latest_to_map(const Pool *pool, Map *m, Queue *samename,
                  int start_block, int stop_block, int latest)
{
    Solvable *solv_element, *solv_previous_element;
    int version_counter = 0;
    solv_previous_element = pool->solvables + samename->elements[start_block];
    Id id_previous_evr = solv_previous_element->evr;
    for (int pos = start_block; pos < stop_block; ++pos) {
        Id id_element = samename->elements[pos];
        solv_element = pool->solvables + id_element;
        Id id_current_evr = solv_element->evr;
        if (id_previous_evr != id_current_evr) {
            version_counter += 1;
            id_previous_evr = id_current_evr;
        }
        if (latest > 0) {
            if (!(version_counter < latest)) {
                return;
            }
        } else {
            if (version_counter < -latest) {
                continue;
            }
        }
        MAPSET(m, id_element);
    }
}

static void
filter_latest(HyQuery q, const struct _Filter *f, Map *m)
{
    Pool *pool = dnf_sack_get_pool(q->sack);

    for (int mi = 0; mi < f->nmatches; ++mi) {
        int latest = f->matches[mi].num;
        if (latest == 0)
            continue;
        Queue samename;

        queue_init(&samename);
        for (int i = 1; i < pool->nsolvables; ++i)
            if (MAPTST(q->result, i))
                queue_push(&samename, i);

        if (f->keyname == HY_PKG_LATEST_PER_ARCH) {
            solv_sort(samename.elements, samename.count, sizeof(Id),
                      filter_latest_sortcmp_byarch, pool);
        } else {
            solv_sort(samename.elements, samename.count, sizeof(Id),
                      filter_latest_sortcmp, pool);
        }

        Solvable *considered, *highest = 0;
        int start_block = -1;
        int i;
        for (i = 0; i < samename.count; ++i) {
            Id p = samename.elements[i];
            considered = pool->solvables + p;
            if (!highest || highest->name != considered->name ||
                ((f->keyname == HY_PKG_LATEST_PER_ARCH) && highest->arch != considered->arch)) {
                /* start of a new block */
                if (start_block == -1) {
                    highest = considered;
                    start_block = i;
                    continue;
                }
                add_latest_to_map(pool, m, &samename, start_block, i, latest);
                highest = considered;
                start_block = i;
            }
        }
        if (start_block != -1) {
            add_latest_to_map(pool, m, &samename, start_block, i, latest);
        }
        queue_free(&samename);
    }
}

static void
filter_advisory(HyQuery q, struct _Filter *f, Map *m, int keyname)
{
    Pool *pool = dnf_sack_get_pool(q->sack);
    DnfAdvisory *advisory;
    g_autoptr(GPtrArray) pkgs = g_ptr_array_new_with_free_func((GDestroyNotify) g_object_unref);
    Dataiterator di;
    gboolean eq;

    // iterate over advisories
    dataiterator_init(&di, pool, 0, 0, 0, 0, 0);
    dataiterator_prepend_keyname(&di, UPDATE_COLLECTION);
    while (dataiterator_step(&di)) {
        dataiterator_setpos_parent(&di);
        advisory = dnf_advisory_new(pool, di.solvid);

        for (int mi = 0; mi < f->nmatches; ++mi) {
            const char *match = f->matches[mi].str;
            switch(keyname) {
            case HY_PKG_ADVISORY:
                eq = dnf_advisory_match_id(advisory, match);
                break;
            case HY_PKG_ADVISORY_BUG:
                eq = dnf_advisory_match_bug(advisory, match);
                break;
            case HY_PKG_ADVISORY_CVE:
                eq = dnf_advisory_match_cve(advisory, match);
                break;
            case HY_PKG_ADVISORY_TYPE:
                eq = dnf_advisory_match_kind(advisory, match);
                break;
            case HY_PKG_ADVISORY_SEVERITY:
                eq = dnf_advisory_match_severity(advisory, match);
                break;
            default:
                eq = FALSE;
            }
            if (eq) {
                // remember package nevras for matched advisories
                GPtrArray *apkgs = dnf_advisory_get_packages(advisory);
                for (unsigned int p = 0; p < apkgs->len; ++p) {
                    auto apkg = g_ptr_array_index(apkgs, p);
                    g_ptr_array_add(pkgs, g_object_ref(apkg));
                }
                g_ptr_array_unref(apkgs);
            }
        }
        g_object_unref(advisory);
        dataiterator_skip_solvable(&di);
    }

    // convert nevras (from DnfAdvisoryPkg) to pool ids
    for (Id id = 1; id < pool->nsolvables; ++id) {
        if (pkgs->len == 0)
            break;
        if (!MAPTST(q->result, id))
            continue;
        Solvable* s = pool_id2solvable(pool, id);
        for (unsigned int p = 0; p < pkgs->len; ++p) {
            auto apkg = static_cast<DnfAdvisoryPkg *>(g_ptr_array_index(pkgs, p));
            if (dnf_advisorypkg_compare_solvable(apkg, pool, s) == 0) {
                MAPSET(m, id);
                // found it, now remove it from the list to speed up rest of query
                g_ptr_array_remove_index(pkgs, p);
                break;
            }
        }
    }
    dataiterator_free(&di);
}

static void
clear_filters(HyQuery q)
{
    for (int i = 0; i < q->nfilters; ++i) {
        struct _Filter *filterp = q->filters + i;
        filter_reinit(filterp, 0);
    }
    g_free(q->filters);
    q->filters = NULL;
    q->nfilters = 0;
}

static void
init_result(HyQuery q)
{
    Pool *pool = dnf_sack_get_pool(q->sack);
    Id solvid;

    int sack_pool_nsolvables = dnf_sack_get_pool_nsolvables(q->sack);
    if (sack_pool_nsolvables != 0 && sack_pool_nsolvables == pool->nsolvables)
        q->result = dnf_sack_get_pkg_solvables(q->sack);
    else {
        q->result = static_cast<Map *>(g_malloc(sizeof(Map)));
        map_init(q->result, pool->nsolvables);
        FOR_PKG_SOLVABLES(solvid)
            map_set(q->result, solvid);
        dnf_sack_set_pkg_solvables(q->sack, q->result, pool->nsolvables);
    }
    if (!(q->flags & HY_IGNORE_EXCLUDES)) {
        dnf_sack_recompute_considered(q->sack);
        if (pool->considered)
            map_and(q->result, pool->considered);
    }
}

void
hy_query_apply(HyQuery q)
{
    Pool *pool = dnf_sack_get_pool(q->sack);
    Map m;

    if (q->applied)
        return;
    if (!q->result)
        init_result(q);
    map_init(&m, pool->nsolvables);
    assert(m.size == q->result->size);
    for (int i = 0; i < q->nfilters; ++i) {
        struct _Filter *f = q->filters + i;
        map_empty(&m);
        switch (f->keyname) {
        case HY_PKG:
            filter_pkg(q, f, &m);
            break;
        case HY_PKG_ALL:
        case HY_PKG_EMPTY:
            /* used to set query empty by keeping Map m empty */
            break;
        case HY_PKG_CONFLICTS:
            filter_rco_reldep(q, f, &m);
            break;
        case HY_PKG_NAME:
            filter_name(q, f, &m);
            break;
        case HY_PKG_EPOCH:
            filter_epoch(q, f, &m);
            break;
        case HY_PKG_EVR:
            filter_evr(q, f, &m);
            break;
        case HY_PKG_NEVRA:
            filter_nevra(q, f, &m);
            break;
        case HY_PKG_VERSION:
            filter_version(q, f, &m);
            break;
        case HY_PKG_RELEASE:
            filter_release(q, f, &m);
            break;
        case HY_PKG_ARCH:
            filter_arch(q, f, &m);
            break;
        case HY_PKG_SOURCERPM:
            filter_sourcerpm(q, f, &m);
            break;
        case HY_PKG_OBSOLETES:
            if (f->match_type == _HY_RELDEP)
                filter_rco_reldep(q, f, &m);
            else {
                assert(f->match_type == _HY_PKG);
                filter_obsoletes(q, f, &m);
            }
            break;
        case HY_PKG_PROVIDES:
            assert(f->match_type == _HY_RELDEP);
            filter_provides_reldep(q, f, &m);
            break;
        case HY_PKG_ENHANCES:
        case HY_PKG_RECOMMENDS:
        case HY_PKG_REQUIRES:
        case HY_PKG_SUGGESTS:
        case HY_PKG_SUPPLEMENTS:
            assert(f->match_type == _HY_RELDEP);
            filter_rco_reldep(q, f, &m);
            break;
        case HY_PKG_REPONAME:
            filter_reponame(q, f, &m);
            break;
        case HY_PKG_LOCATION:
            filter_location(q, f, &m);
            break;
        case HY_PKG_ADVISORY:
        case HY_PKG_ADVISORY_BUG:
        case HY_PKG_ADVISORY_CVE:
        case HY_PKG_ADVISORY_SEVERITY:
        case HY_PKG_ADVISORY_TYPE:
            filter_advisory(q, f, &m, f->keyname);
            break;
        case HY_PKG_LATEST:
        case HY_PKG_LATEST_PER_ARCH:
            filter_latest(q, f, &m);
            break;
        case HY_PKG_DOWNGRADABLE:
        case HY_PKG_UPGRADABLE:
            filter_updown_able(q, f, &m);
            break;
        case HY_PKG_DOWNGRADES:
        case HY_PKG_UPGRADES:
            filter_updown(q, f, &m);
            break;
        default:
            filter_dataiterator(q, f, &m);
        }
        if (f->cmp_type & HY_NOT)
            map_subtract(q->result, &m);
        else
            map_and(q->result, &m);
    }
    map_free(&m);

    q->applied = 1;
    clear_filters(q);
}

HyQuery
hy_query_create(DnfSack *sack)
{
    return hy_query_create_flags(sack, 0);
}

HyQuery
hy_query_create_flags(DnfSack *sack, int flags)
{
    HyQuery q = static_cast<HyQuery>(g_malloc0(sizeof(*q)));
    q->sack = sack;
    q->flags = flags;
    return q;
}

HyQuery
hy_query_from_nevra(HyNevra nevra, DnfSack *sack, gboolean icase)
{
    HyQuery query = hy_query_create(sack);
    hy_add_filter_nevra_object(query, nevra, icase);
    return query;
}

void
hy_query_free(HyQuery q)
{
    hy_query_clear(q);
    g_free(q);
}

void
hy_query_clear(HyQuery q)
{
    if (q->result) {
        map_free(q->result);
        g_free(q->result);
        q->result = NULL;
    }
    clear_filters(q);
}

HyQuery
hy_query_clone(HyQuery q)
{
    HyQuery qn = hy_query_create(q->sack);

    qn->flags = q->flags;
    qn->applied = q->applied;

    for (int i = 0; i < q->nfilters; ++i) {
        struct _Filter *filterp = query_add_filter(qn, q->filters[i].nmatches);

        filterp->cmp_type = q->filters[i].cmp_type;
        filterp->keyname = q->filters[i].keyname;
        filterp->match_type = q->filters[i].match_type;
        for (int j = 0; j < q->filters[i].nmatches; ++j) {
            char *str_copy;
            DnfPackageSet *pset;
            DnfReldep *reldep;

            switch (filterp->match_type) {
            case _HY_NUM:
                filterp->matches[j].num = q->filters[i].matches[j].num;
                break;
            case _HY_PKG:
                pset = q->filters[i].matches[j].pset;
                filterp->matches[j].pset = dnf_packageset_clone(pset);
                break;
            case _HY_RELDEP:
                reldep = q->filters[i].matches[j].reldep;
                filterp->matches[j].reldep = static_cast<DnfReldep *>(g_object_ref(reldep));
                break;
            case _HY_STR:
                str_copy = g_strdup(q->filters[i].matches[j].str);
                filterp->matches[j].str = str_copy;
                break;
            default:
                assert(0);
            }
        }
    }
    assert(qn->nfilters == q->nfilters);
    if (q->result) {
        qn->result = static_cast<Map *>(g_malloc0(sizeof(Map)));
        map_init_clone(qn->result, q->result);
    }

    return qn;
}

int
hy_query_filter(HyQuery q, int keyname, int cmp_type, const char *match)
{
    if ((cmp_type & HY_GLOB) && !hy_is_glob_pattern(match))
        cmp_type = (cmp_type & ~HY_GLOB) | HY_EQ;

    if (!valid_filter_str(keyname, cmp_type))
        return DNF_ERROR_BAD_QUERY;
    q->applied = 0;

    switch (keyname) {
        case HY_PKG_CONFLICTS:
        case HY_PKG_ENHANCES:
        case HY_PKG_OBSOLETES:
        case HY_PKG_PROVIDES:
        case HY_PKG_RECOMMENDS:
        case HY_PKG_REQUIRES:
        case HY_PKG_SUGGESTS:
        case HY_PKG_SUPPLEMENTS: {
            DnfSack *sack = query_sack(q);

            if (cmp_type == HY_GLOB) {
                DnfReldepList *reldeplist = reldeplist_from_str (sack, match);
                if (reldeplist == NULL)
                    return hy_query_filter_empty(q);
                hy_query_filter_reldep_in(q, keyname, reldeplist);
                g_object_unref (reldeplist);
                return 0;
            } else {
                DnfReldep *reldep = reldep_from_str (sack, match);
                if (reldep == NULL)
                    return hy_query_filter_empty(q);
                int ret = hy_query_filter_reldep(q, keyname, reldep);
                g_object_unref (reldep);
                return ret;
            }
        }
        default: {
            struct _Filter *filterp = query_add_filter(q, 1);
            filterp->cmp_type = cmp_type;
            filterp->keyname = keyname;
            filterp->match_type = _HY_STR;
            filterp->matches[0].str = g_strdup(match);
            return 0;
        }
    }
}

int
hy_query_filter_empty(HyQuery q)
{
    hy_query_filter_num(q, HY_PKG_EMPTY, HY_EQ, 1);
    return 0;
}

int
hy_query_filter_in(HyQuery q, int keyname, int cmp_type,
                   const char **matches)
{
    if (cmp_type == HY_GLOB) {
        gboolean is_glob = FALSE;
        for (const char **match = matches; *match != NULL; match++) {
            if (hy_is_glob_pattern(*match)) {
                is_glob = TRUE;
                break;
            }
        }
        if (!is_glob) cmp_type = HY_EQ;
    }

    if (!valid_filter_str(keyname, cmp_type))
        return DNF_ERROR_BAD_QUERY;
    q->applied = 0;

    const unsigned count = g_strv_length((gchar**)matches);
    struct _Filter *filterp = query_add_filter(q, count);

    filterp->cmp_type = cmp_type;
    filterp->keyname = keyname;
    filterp->match_type = _HY_STR;
    for (guint i = 0; i < count; ++i)
        filterp->matches[i].str = g_strdup(matches[i]);
    return 0;
}

int
hy_query_filter_num(HyQuery q, int keyname, int cmp_type, int match)
{
    if (!valid_filter_num(keyname, cmp_type))
        return DNF_ERROR_BAD_QUERY;
    q->applied = 0;

    struct _Filter *filterp = query_add_filter(q, 1);
    filterp->cmp_type = cmp_type;
    filterp->keyname = keyname;
    filterp->match_type = _HY_NUM;
    filterp->matches[0].num = match;
    return 0;
}

int
hy_query_filter_num_in(HyQuery q, int keyname, int cmp_type, int nmatches,
                       const int *matches)
{
    if (!valid_filter_num(keyname, cmp_type))
        return DNF_ERROR_BAD_QUERY;
    q->applied = 0;

    struct _Filter *filterp = query_add_filter(q, nmatches);

    filterp->cmp_type = cmp_type;
    filterp->keyname = keyname;
    filterp->match_type = _HY_NUM;
    for (int i = 0; i < nmatches; ++i)
        filterp->matches[i].num = matches[i];
    return 0;
}

int
hy_query_filter_package_in(HyQuery q, int keyname, int cmp_type,
                           DnfPackageSet *pset)
{
    if (!valid_filter_pkg(keyname, cmp_type))
        return DNF_ERROR_BAD_QUERY;
    q->applied = 0;

    struct _Filter *filterp = query_add_filter(q, 1);
    filterp->cmp_type = cmp_type;
    filterp->keyname = keyname;
    filterp->match_type = _HY_PKG;
    filterp->matches[0].pset = dnf_packageset_clone(pset);
    return 0;
}

int
hy_query_filter_reldep(HyQuery q, int keyname, DnfReldep *reldep)
{
    if (!valid_filter_reldep(keyname))
        return DNF_ERROR_BAD_QUERY;
    q->applied = 0;

    struct _Filter *filterp = query_add_filter(q, 1);
    filterp->cmp_type = HY_EQ;
    filterp->keyname = keyname;
    filterp->match_type = _HY_RELDEP;
    filterp->matches[0].reldep = static_cast<DnfReldep *>(g_object_ref(reldep));
    return 0;
}

int
hy_query_filter_reldep_in(HyQuery q, int keyname, DnfReldepList *reldeplist)
{
    if (!valid_filter_reldep(keyname))
        return DNF_ERROR_BAD_QUERY;
    q->applied = 0;

    const int nmatches = dnf_reldep_list_count (reldeplist);
    struct _Filter *filterp = query_add_filter(q, nmatches);
    filterp->cmp_type = HY_EQ;
    filterp->keyname = keyname;
    filterp->match_type = _HY_RELDEP;

    for (int i = 0; i < nmatches; ++i)
        filterp->matches[i].reldep = dnf_reldep_list_index (reldeplist, i);
    return 0;
}

int
hy_query_filter_provides(HyQuery q, int cmp_type, const char *name,
                         const char *evr)
{
    DnfReldep *reldep = dnf_reldep_new(query_sack(q), name,
                                       static_cast<DnfComparisonKind>(cmp_type), evr);
    assert(reldep);
    int ret = hy_query_filter_reldep(q, HY_PKG_PROVIDES, reldep);
    g_object_unref (reldep);
    return ret;
}

int
hy_query_filter_provides_in(HyQuery q, char **reldep_strs)
{
    int cmp_type;
    char *name = NULL;
    char *evr = NULL;
    DnfReldep *reldep;
    DnfReldepList *reldeplist = dnf_reldep_list_new (q->sack);
    for (int i = 0; reldep_strs[i] != NULL; ++i) {
        if (parse_reldep_str(reldep_strs[i], &name, &evr, &cmp_type) == -1) {
            g_object_unref(reldeplist);
            return DNF_ERROR_BAD_QUERY;
        }
        reldep = dnf_reldep_new(q->sack, name, static_cast<DnfComparisonKind>(cmp_type), evr);
        if (reldep) {
            dnf_reldep_list_add (reldeplist, reldep);
            g_object_unref (reldep);
        }
        g_free(name);
        g_free(evr);
    }
    hy_query_filter_reldep_in(q, HY_PKG_PROVIDES, reldeplist);
    g_object_unref (reldeplist);
    return 0;
}


/**
 * Narrows to only those installed packages for which there is a downgrading package.
 */
void
hy_query_filter_downgradable(HyQuery q, int val)
{
    hy_query_filter_num(q, HY_PKG_DOWNGRADABLE, HY_EQ, val);
}

/**
 * Narrows to only packages downgrading installed packages.
 */
void
hy_query_filter_downgrades(HyQuery q, int val)
{
    hy_query_filter_num(q, HY_PKG_DOWNGRADES, HY_EQ, val);
}

/**
 * Narrows to only those installed packages for which there is an updating package.
 */
void
hy_query_filter_upgradable(HyQuery q, int val)
{
    hy_query_filter_num(q, HY_PKG_UPGRADABLE, HY_EQ, val);
}

/**
 * Narrows to only packages updating installed packages.
 */
void
hy_query_filter_upgrades(HyQuery q, int val)
{
    hy_query_filter_num(q, HY_PKG_UPGRADES, HY_EQ, val);
}

/**
 * Narrows to only the highest version of a package per arch.
 */
void
hy_query_filter_latest_per_arch(HyQuery q, int val)
{
    hy_query_filter_num(q, HY_PKG_LATEST_PER_ARCH, HY_EQ, val);
}

/**
 * Narrows to only the highest version of a package.
 */
void
hy_query_filter_latest(HyQuery q, int val)
{
    hy_query_filter_num(q, HY_PKG_LATEST, HY_EQ, val);
}

GPtrArray *
hy_query_run(HyQuery q)
{
    Pool *pool = dnf_sack_get_pool(q->sack);
    GPtrArray *plist = hy_packagelist_create();

    hy_query_apply(q);
    for (int i = 1; i < pool->nsolvables; ++i)
        if (MAPTST(q->result, i))
            g_ptr_array_add(plist, dnf_package_new(q->sack, i));
    return plist;
}

DnfPackageSet *
hy_query_run_set(HyQuery q)
{
    hy_query_apply(q);
    return dnf_packageset_from_bitmap(q->sack, q->result);
}

/**
 * hy_query_union:
 * @q:     a #HyQuery instance
 * @other: other #HyQuery instance
 *
 * Unites query with other query (aka logical or).
 *
 * Returns: Nothing.
 *
 * Since: 0.7.0
 */
void
hy_query_union(HyQuery q, HyQuery other)
{
    hy_query_apply(q);
    hy_query_apply(other);
    map_or(q->result, other->result);
}

/**
 * hy_query_intersection:
 * @q:     a #HyQuery instance
 * @other: other #HyQuery instance
 *
 * Intersects query with other query (aka logical and).
 *
 * Returns: Nothing.
 *
 * Since: 0.7.0
 */
void
hy_query_intersection(HyQuery q, HyQuery other)
{
    hy_query_apply(q);
    hy_query_apply(other);
    map_and(q->result, other->result);
}

/**
 * hy_query_difference:
 * @q:     a #HyQuery instance
 * @other: other #HyQuery instance
 *
 * Computes difference between query and other query (aka q and not other).
 *
 * Returns: Nothing.
 *
 * Since: 0.7.0
 */
void
hy_query_difference(HyQuery q, HyQuery other)
{
    hy_query_apply(q);
    hy_query_apply(other);
    map_subtract(q->result, other->result);
}

gboolean
hy_query_is_not_empty(HyQuery query)
{
    hy_query_apply(query);
    const unsigned char *res = query->result->map;
    const unsigned char *end = res + query->result->size;

    while (res < end) {
        if (*res++)
            return TRUE;
    }
    return FALSE;
}

gboolean
hy_query_is_applied(const HyQuery query)
{
    return query->applied ? TRUE : FALSE;
}

const Map *
hy_query_get_result(const HyQuery query)
{
    return query->result;
}

DnfSack *
hy_query_get_sack(HyQuery query)
{
    return query->sack;
}

void
hy_query_to_name_ordered_queue(HyQuery query, Queue *samename)
{
    hy_query_apply(query);
    Pool *pool = dnf_sack_get_pool(query->sack);

    queue_init(samename);
    for (int i = 1; i < pool->nsolvables; ++i)
        if (MAPTST(query->result, i))
            queue_push(samename, i);

    solv_sort(samename->elements, samename->count, sizeof(Id), filter_latest_sortcmp, pool);
}

void
hy_query_to_name_arch_ordered_queue(HyQuery query, Queue *samename)
{
    hy_query_apply(query);
    Pool *pool = dnf_sack_get_pool(query->sack);

    queue_init(samename);
    for (int i = 1; i < pool->nsolvables; ++i)
        if (MAPTST(query->result, i))
            queue_push(samename, i);

    solv_sort(samename->elements, samename->count, sizeof(Id), filter_latest_sortcmp_byarch, pool);
}

void
hy_add_filter_nevra_object(HyQuery query, HyNevra nevra, gboolean icase)
{
    if (!nevra->getName().empty() && nevra->getName() != "*") {
        if (icase)
            hy_query_filter(query, HY_PKG_NAME, HY_GLOB|HY_ICASE, nevra->getName().c_str());
        else
            hy_query_filter(query, HY_PKG_NAME, HY_GLOB, nevra->getName().c_str());
    }
    if (nevra->getEpoch() != -1)
        hy_query_filter_num(query, HY_PKG_EPOCH, HY_EQ, nevra->getEpoch());
    if (!nevra->getVersion().empty() && nevra->getVersion() != "*")
        hy_query_filter(query, HY_PKG_VERSION, HY_GLOB, nevra->getVersion().c_str());
    if (!nevra->getRelease().empty() && nevra->getRelease() != "*")
        hy_query_filter(query, HY_PKG_RELEASE, HY_GLOB, nevra->getRelease().c_str());
    if (!nevra->getArch().empty() && nevra->getArch() != "*")
        hy_query_filter(query, HY_PKG_ARCH, HY_GLOB, nevra->getArch().c_str());
}

void
hy_add_filter_extras(HyQuery query)
{
    Pool *pool = dnf_sack_get_pool(query->sack);
    gboolean matched;
    Solvable *s_installed, *s_available;

    hy_query_apply(query);

    HyQuery query_installed = hy_query_clone(query);
    hy_query_filter(query_installed, HY_PKG_REPONAME, HY_EQ, HY_SYSTEM_REPO_NAME);
    HyQuery query_available = hy_query_clone(query);
    hy_query_filter(query_available, HY_PKG_REPONAME, HY_NEQ, HY_SYSTEM_REPO_NAME);

    hy_query_apply(query_installed);
    hy_query_apply(query_available);
    MAPZERO(query->result);

    for (Id id_installed = 1; id_installed < pool->nsolvables; ++id_installed) {
        if (!MAPTST(query_installed->result, id_installed)) {
            continue;
        }
        s_installed = pool_id2solvable(pool, id_installed);
        matched = FALSE;
        for (Id id_available = 1; id_available < pool->nsolvables; ++id_available) {
            if (!MAPTST(query_available->result, id_available)) {
                continue;
            }
            s_available = pool_id2solvable(pool, id_available);
            if ((s_installed->name == s_available->name) && (s_installed->evr == s_available->evr) && (s_installed->arch == s_available->arch)) {
                matched = TRUE;
                break;
            }
        }
        if (!matched)
            MAPSET(query->result, id_installed);
    }
    hy_query_free(query_installed);
    hy_query_free(query_available);
}

void
hy_filter_recent(HyQuery query, const long unsigned int recent_limit)
{
    Pool *pool = dnf_sack_get_pool(query->sack);
    hy_query_apply(query);

    for (Id id = 1; id < pool->nsolvables; ++id) {
        if (!MAPTST(query->result, id)) {
            continue;
        }
        DnfPackage *pkg = dnf_package_new(query->sack, id);
        guint64 build_time = dnf_package_get_buildtime(pkg);
        if (build_time <= recent_limit) {
            MAPCLR(query->result, id);
        }
    }
}

static void
add_duplicates_to_map(Pool *pool, Map *res, Queue samename, int start_block, int stop_block)
{
    Solvable *s_first, *s_second;
    for (int pos = start_block; pos < stop_block; ++pos) {
        Id id_first = samename.elements[pos];
        s_first = pool->solvables + id_first;
        for (int pos2 = pos + 1; pos2 < stop_block; ++pos2) {
            Id id_second = samename.elements[pos2];
            s_second = pool->solvables + id_second;
            if ((s_first->evr == s_second->evr) && (s_first->arch != s_second->arch)) {
                continue;
            }
            MAPSET(res, id_first);
            MAPSET(res, id_second);
        }
    }
}

void
hy_filter_duplicated(HyQuery query)
{
    Queue samename;
    Pool *pool = dnf_sack_get_pool(query->sack);

    hy_query_filter(query, HY_PKG_REPONAME, HY_EQ, HY_SYSTEM_REPO_NAME);
    hy_query_apply(query);

    hy_query_to_name_ordered_queue(query, &samename);

    Solvable *considered, *highest = 0;
    int start_block = -1;
    int i;
    MAPZERO(query->result);
    for (i = 0; i < samename.count; ++i) {
        Id p = samename.elements[i];
        considered = pool->solvables + p;
        if (!highest || highest->name != considered->name) {
            /* start of a new block */
            if (start_block == -1) {
                highest = considered;
                start_block = i;
                continue;
            }
            if (start_block != i - 1) {
                add_duplicates_to_map(pool, query->result, samename, start_block, i);
            }
            highest = considered;
            start_block = i;
        }
    }
    if (start_block != -1) {
        add_duplicates_to_map(pool, query->result, samename, start_block, i);
    }
}

#if WITH_SWDB
int
hy_filter_unneeded(HyQuery query, DnfSwdb *swdb, const gboolean debug_solver)
{
    hy_query_apply(query);
    GPtrArray *nevras = g_ptr_array_new_with_free_func(g_free);
    Queue id_store;
    HyGoal goal = hy_goal_create(query->sack);
    Pool *pool = dnf_sack_get_pool(query->sack);

    HyQuery installed = hy_query_create(query->sack);
    hy_query_filter(installed, HY_PKG_REPONAME, HY_EQ, HY_SYSTEM_REPO_NAME);
    hy_query_apply(installed);
    queue_init(&id_store);

    for (Id id = 1; id < pool->nsolvables; ++id) {
        if (!MAPTST(installed->result, id))
            continue;
        Solvable* s = pool_id2solvable(pool, id);
        const char* nevra = pool_solvable2str(pool, s);
        g_ptr_array_add(nevras, (gpointer)g_strdup(nevra));
        queue_push(&id_store, id);
    }

    GArray *indexes = dnf_swdb_select_user_installed(swdb, nevras);
    g_ptr_array_free(nevras, TRUE);

    for (guint i = 0; i < indexes->len; ++i) {
        guint index = g_array_index(indexes, gint, i);
        Id pkg_id = id_store.elements[index];
        DnfPackage *pkg = dnf_package_new(query->sack, pkg_id);
        hy_goal_userinstalled(goal, pkg);
    }
    g_array_free(indexes, TRUE);
    queue_free(&id_store);
    int ret1 = hy_goal_run_flags(goal, DNF_NONE);
    if (ret1)
        return -1;

    if (debug_solver) {
        g_autoptr(GError) error = NULL;
        gboolean ret = hy_goal_write_debugdata(goal, "./debugdata-autoremove", &error);
        if (!ret) {
            return -1;
        }
    }

    Queue que;
    Solver *solv = goal->solv;

    queue_init(&que);
    solver_get_unneeded(solv, &que, 0);
    Map result;
    map_init(&result, pool->nsolvables);

    for (int i = 0; i < que.count; ++i) {
        MAPSET(&result, que.elements[i]);
    }
    queue_free(&que);
    map_and(query->result, &result);
    map_free(&result);
    return 0;
}
#endif

HySelector
hy_query_to_selector(HyQuery query)
{
    HySelector selector = hy_selector_create(query->sack);
    DnfPackageSet *pset = hy_query_run_set(query);
    hy_selector_pkg_set(selector, HY_PKG, HY_EQ, pset);
    g_object_unref(pset);
    return selector;
}
