/*
 * Copyright (C) 2012-2013 Red Hat, Inc.
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

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <pwd.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <wordexp.h>

// libsolv
extern "C" {
#include <solv/chksum.h>
#include <solv/evr.h>
#include <solv/solver.h>
#include <solv/solverdebug.h>
#include <solv/util.h>
#include <solv/pool_parserpmrichdep.h>
}

// glib
#include <glib.h>

// hawkey
#include "dnf-advisory-private.hpp"
#include "dnf-types.h"
#include "hy-iutil-private.hpp"
#include "hy-package-private.hpp"
#include "hy-packageset-private.hpp"
#include "hy-query.h"
#include "hy-util-private.hpp"
#include "dnf-sack-private.hpp"
#include "sack/packageset.hpp"

#include "utils/bgettext/bgettext-lib.h"
#include "sack/packageset.hpp"

#define BUF_BLOCK 4096
#define CHKSUM_TYPE REPOKEY_TYPE_SHA256
#define CHKSUM_IDENT "H000"
#define CACHEDIR_PERMISSIONS 0700

static mode_t
get_umask(void)
{
    mode_t mask = umask(0);
    umask(mask);
    return mask;
}

static int
glob_for_cachedir(char *path)
{
    int ret = 1;
    if (!g_str_has_suffix(path, "XXXXXX"))
        return ret;

    wordexp_t word_vector;
    char *p = g_strdup(path);
    const int len = strlen(p);
    struct stat s;

    ret = 2;
    p[len-6] = '*';
    p[len-5] = '\0';
    if (wordexp(p, &word_vector, 0)) {
        g_free(p);
        return ret;
    }
    for (guint i = 0; i < word_vector.we_wordc; ++i) {
        char *entry = word_vector.we_wordv[i];
        if (stat(entry, &s))
            continue;
        if (S_ISDIR(s.st_mode) &&
            s.st_uid == getuid()) {
            assert(strlen(path) == strlen(entry));
            strcpy(path, entry);
            ret = 0;
            break;
        }
    }
    wordfree(&word_vector);
    g_free(p);
    return ret;
}

int
checksum_cmp(const unsigned char *cs1, const unsigned char *cs2)
{
    return memcmp(cs1, cs2, CHKSUM_BYTES);
}

/* calls rewind(fp) before returning */
int
checksum_fp(unsigned char *out, FILE *fp)
{
    /* based on calc_checksum_fp in libsolv's solv.c */
    char buf[4096];
    auto h = solv_chksum_create(CHKSUM_TYPE);
    int l;

    rewind(fp);
    solv_chksum_add(h, CHKSUM_IDENT, strlen(CHKSUM_IDENT));
    while ((l = fread(buf, 1, sizeof(buf), fp)) > 0)
        solv_chksum_add(h, buf, l);
    rewind(fp);
    solv_chksum_free(h, out);
    return 0;
}

/* calls rewind(fp) before returning */
int
checksum_read(unsigned char *csout, FILE *fp)
{
    if (fseek(fp, -32, SEEK_END) ||
        fread(csout, CHKSUM_BYTES, 1, fp) != 1)
        return 1;
    rewind(fp);
    return 0;
}

/* does not move the fp position */
int
checksum_stat(unsigned char *out, FILE *fp)
{
    assert(fp);

    struct stat stat;
    if (fstat(fileno(fp), &stat))
        return 1;

    /* based on calc_checksum_stat in libsolv's solv.c */
    auto h = solv_chksum_create(CHKSUM_TYPE);
    solv_chksum_add(h, CHKSUM_IDENT, strlen(CHKSUM_IDENT));
    solv_chksum_add(h, &stat.st_dev, sizeof(stat.st_dev));
    solv_chksum_add(h, &stat.st_ino, sizeof(stat.st_ino));
    solv_chksum_add(h, &stat.st_size, sizeof(stat.st_size));
    solv_chksum_add(h, &stat.st_mtime, sizeof(stat.st_mtime));
    solv_chksum_free(h, out);
    return 0;
}

/* moves fp to the end of file */
int checksum_write(const unsigned char *cs, FILE *fp)
{
    if (fseek(fp, 0, SEEK_END) ||
        fwrite(cs, CHKSUM_BYTES, 1, fp) != 1)
        return 1;
    return 0;
}

int
checksum_type2length(int type)
{
    switch(type) {
    case G_CHECKSUM_MD5:
        return 16;
    case G_CHECKSUM_SHA1:
        return 20;
    case G_CHECKSUM_SHA256:
        return 32;
    case G_CHECKSUM_SHA384:
        return 48;
    case G_CHECKSUM_SHA512:
        return 64;
    default:
        return -1;
    }
}

int
checksumt_l2h(int type)
{
    switch (type) {
    case REPOKEY_TYPE_MD5:
        return G_CHECKSUM_MD5;
    case REPOKEY_TYPE_SHA1:
        return G_CHECKSUM_SHA1;
    case REPOKEY_TYPE_SHA256:
        return G_CHECKSUM_SHA256;
    case REPOKEY_TYPE_SHA384:
        return G_CHECKSUM_SHA384;
    case REPOKEY_TYPE_SHA512:
        return G_CHECKSUM_SHA512;
    default:
        assert(0);
        return 0;
    }
}

const char *
pool_checksum_str(Pool *pool, const unsigned char *chksum)
{
    int length = checksum_type2length(checksumt_l2h(CHKSUM_TYPE));
    return pool_bin2hex(pool, chksum, length);
}

char *
abspath(const char *path)
{
    const int len = strlen(path);
    if (len <= 1)
        return NULL;

    if (path[0] == '/')
        return g_strdup(path);

    char cwd[PATH_MAX];
    if (!getcwd(cwd, PATH_MAX)) {
        return NULL;
    }

    return solv_dupjoin(cwd, "/", path);
}

Map *
free_map_fully(Map *m)
{
    if (m == NULL)
        return NULL;
    map_free(m);
    g_free(m);
    return NULL;
}

int
is_package(const Pool *pool, const Solvable *s)
{
    return !g_str_has_prefix(pool_id2str(pool, s->name), SOLVABLE_NAME_ADVISORY_PREFIX);
}

int
is_readable_rpm(const char *fn)
{
    int len = strlen(fn);

    if (access(fn, R_OK))
        return 0;
    if (len <= 4 || strcmp(fn + len - 4, ".rpm"))
        return 0;

    return 1;
}

/**
 * Recursively create directory.
 *
 * If it is in the format accepted by mkdtemp() the function globs for a
 * matching name and if not found it uses mkdtemp() to create the path. 'path'
 * is modified in those two cases.
 */
int
mkcachedir(char *path)
{
    int ret = 1;

    if (!glob_for_cachedir(path))
        return 0;

    const int len = strlen(path);
    if (len < 1 || path[0] != '/')
        return 1; // only absolute pathnames are accepted

    char *p = g_strdup(path);

    if (p[len-1] == '/')
        p[len-1] = '\0';

    if (access(p, X_OK)) {
        *(strrchr(p, '/')) = '\0';
        ret = mkcachedir(p);
        if (g_str_has_suffix(path, "XXXXXX")) {
            char *retptr = mkdtemp(path);
            if (retptr == NULL)
                ret |= 1;
        } else
            ret |= mkdir(path, CACHEDIR_PERMISSIONS);
    } else {
        ret = 0;
    }

    g_free(p);
    return ret;
}

gboolean
mv(const char* old_path, const char* new_path, GError** error)
{
    if (rename(old_path, new_path)) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_FILE_INVALID,
                    _("Failed renaming %1$s to %2$s: %3$s"),
                    old_path, new_path, strerror(errno));
        return FALSE;
    }
    if (chmod(new_path, 0666 & ~get_umask())) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_FILE_INVALID,
                    _("Failed setting perms on %1$s: %2$s"),
                    new_path, strerror(errno));
        return FALSE;
    }
    return TRUE;
}

char *
this_username(void)
{
    const struct passwd *pw = getpwuid(getuid());
    return g_strdup(pw->pw_name);
}

char *
read_whole_file(const char *path)
{
  char *contents = NULL;
  if (!g_file_get_contents (path, &contents, NULL, NULL))
    return NULL;
  return contents;
}

static char *
pool_tmpdup(Pool *pool, const char *s)
{
    char *dup = pool_alloctmpspace(pool, strlen(s) + 1);
    return strcpy(dup, s);
}

Id
running_kernel(DnfSack *sack)
{
    Pool *pool = dnf_sack_get_pool(sack);
    struct utsname un;

    if (uname(&un) < 0) {
        g_debug("uname(): %s", g_strerror(errno));
        return -1;
    }
    char *fn = pool_tmpjoin(pool, "/boot/vmlinuz-", un.release, NULL);
    if (access(fn, F_OK))
        g_debug("running_kernel(): no matching file: %s.", fn);

    Id kernel_id = -1;
    HyQuery q = hy_query_create_flags(sack, HY_IGNORE_EXCLUDES);
    dnf_sack_make_provides_ready(sack);
    hy_query_filter(q, HY_PKG_FILE, HY_EQ, fn);
    hy_query_filter(q, HY_PKG_REPONAME, HY_EQ, HY_SYSTEM_REPO_NAME);
    DnfPackageSet *pset = hy_query_run_set(q);
    kernel_id = pset->next(kernel_id);
    delete pset;
    hy_query_free(q);

    if (kernel_id >= 0)
        g_debug("running_kernel(): %s.", id2nevra(pool, kernel_id));
    else
        g_debug("running_kernel(): running kernel not matched to a package.");
    return kernel_id;
}

Repo *
repo_by_name(DnfSack *sack, const char *name)
{
    Pool *pool = dnf_sack_get_pool(sack);
    Repo *repo;
    int repoid;

    FOR_REPOS(repoid, repo) {
        if (!strcmp(repo->name, name))
            return repo;
    }
    return NULL;
}

HyRepo
hrepo_by_name(DnfSack *sack, const char *name)
{
    Repo *repo = repo_by_name(sack, name);

    if (repo)
        return static_cast<HyRepo>(repo->appdata);
    return NULL;
}

Id
str2archid(Pool *pool, const char *arch)
{
    // originally from libsolv/examples/solv.c:str2archid()
    Id id;
    if (!*arch)
        return 0;
    id = pool_str2id(pool, arch, 0);
    if (id == ARCH_SRC || id == ARCH_NOSRC || id == ARCH_NOARCH)
        return id;
    if (pool->id2arch && (id > pool->lastarch || !pool->id2arch[id]))
        return 0;
    return id;
}

/**
 * Return id of a package that can be upgraded with pkg.
 *
 * The returned package Id fulfills the following criteria:
 * :: it is installed
 * :: has the same name as pkg
 * :: arch of the installed pkg is upgradable to the new pkg. In RPM world that
 *    roughly means: if both pacakges are colored (contains ELF binaries and was
 *    built with internal dependency generator), they are not upgradable to each
 *    other (i.e. i386 package can not be upgraded to x86_64, neither the other
 *    way round). If one of them is noarch and the other one colored then the
 *    pkg is upgradable (i.e. one can upgrade .noarch to .x86_64 and then again
 *    to a new version that is .noarch)
 * :: is of lower version than pkg.
 * :: if there are multiple packages of that name return the highest version
 *    (implying we won't claim we can upgrade an old package with an already
 *    installed version, e.g kernel).
 *
 * Or 0 if none such package is installed.
 */
Id
what_upgrades(Pool *pool, Id pkg)
{
    Id l = 0, l_evr = 0;
    Id p, pp;
    Solvable *updated, *s = pool_id2solvable(pool, pkg);

    assert(pool->installed);
    assert(pool->whatprovides);
    FOR_PROVIDES(p, pp, s->name) {
        updated = pool_id2solvable(pool, p);
        if (updated->repo != pool->installed ||
            updated->name != s->name)
            continue;
        if (updated->arch != s->arch &&
            updated->arch != ARCH_NOARCH &&
            s->arch != ARCH_NOARCH)
            continue;
        if (pool_evrcmp(pool, updated->evr, s->evr, EVRCMP_COMPARE) >= 0)
            // >= version installed, this pkg can not be used for upgrade
            return 0;
        if (l == 0 ||
            pool_evrcmp(pool, updated->evr, l_evr, EVRCMP_COMPARE) > 0) {
            l = p;
            l_evr = updated->evr;
        }
    }
    return l;
}

/**
 * Return id of a package that can be upgraded with pkg.
 *
 * The returned package Id fulfills the following criteria:
 * :: it is installed
 * :: has the same name and arch as pkg
 * :: is of higher version than pkg.
 * :: if there are multiple such packages return the lowest version (so we won't
 *    claim we can downgrade a package when a lower version is already
 *    installed)
 *
 * Or 0 if none such package is installed.
 */
Id
what_downgrades(Pool *pool, Id pkg)
{
    Id l = 0, l_evr = 0;
    Id p, pp;
    Solvable *updated, *s = pool_id2solvable(pool, pkg);

    assert(pool->installed);
    assert(pool->whatprovides);
    FOR_PROVIDES(p, pp, s->name) {
        updated = pool_id2solvable(pool, p);
        if (updated->repo != pool->installed ||
            updated->name != s->name ||
            updated->arch != s->arch)
            continue;
        if (pool_evrcmp(pool, updated->evr, s->evr, EVRCMP_COMPARE) <= 0)
            // <= version installed, this pkg can not be used for downgrade
            return 0;
        if (l == 0 ||
            pool_evrcmp(pool, updated->evr, l_evr, EVRCMP_COMPARE) < 0) {
            l = p;
            l_evr = updated->evr;
        }
    }
    return l;
}

unsigned long
pool_get_epoch(Pool *pool, const char *evr)
{
    char *e, *v, *r, *endptr;
    unsigned long epoch = 0;

    pool_split_evr(pool, evr, &e, &v, &r);
    if (e) {
        long int converted = strtol(e, &endptr, 10);
        assert(converted > 0);
        assert(*endptr == '\0');
        epoch = converted;
    }

    return epoch;
}

/**
 * Split evr into its components.
 *
 * Believes blindly in 'evr' being well formed. This could be implemented
 * without 'pool' of course but either the caller would have to provide buffers
 * to store the split pieces, or this would call strdup (which is more expensive
 * than the pool temp space).
 */
void
pool_split_evr(Pool *pool, const char *evr_c, char **epoch, char **version,
                   char **release)
{
    char *evr = pool_tmpdup(pool, evr_c);
    char *e, *v, *r;

    for (e = evr + 1; *e != ':' && *e != '-' && *e != '\0'; ++e)
        ;

    if (*e == '-') {
        *e = '\0';
        v = evr;
        r = e + 1;
        e = NULL;
    } else if (*e == '\0') {
        v = evr;
        e = NULL;
        r = NULL;
    } else { /* *e == ':' */
        *e = '\0';
        v = e + 1;
        e = evr;
        for (r = v + 1; *r != '-'; ++r)
            assert(*r);
        *r = '\0';
        r++;
    }
    *epoch = e;
    *version = v;
    *release = r;
}

const char *
id2nevra(Pool *pool, Id id)
{
    Solvable *s = pool_id2solvable(pool, id);
    return pool_solvable2str(pool, s);
}


GPtrArray *
packageSet2GPtrArray(libdnf::PackageSet * pset)
{
    if (!pset) {
        return NULL;
    }
    GPtrArray *plist = hy_packagelist_create();

    DnfSack * sack = pset->getSack();

    Id id = -1;
    while ((id = pset->next(id)) != -1) {
        g_ptr_array_add(plist, dnf_package_new(sack, id));
    }
    return plist;
}
