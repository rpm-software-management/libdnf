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

#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/limits.h>
#include <pwd.h>
#include <regex.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <wordexp.h>

// libsolv
#include <solv/chksum.h>
#include <solv/evr.h>
#include <solv/solver.h>
#include <solv/solverdebug.h>
#include <solv/util.h>

// glib
#include <glib.h>

// hawkey
#include "errno_internal.h"
#include "iutil.h"
#include "package_internal.h"
#include "packageset_internal.h"
#include "query.h"
#include "reldep.h"
#include "sack_internal.h"

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
    if (!str_endswith(path, "XXXXXX"))
	return ret;

    wordexp_t word_vector;
    char *p = solv_strdup(path);
    const int len = strlen(p);
    struct stat s;

    ret = 2;
    p[len-6] = '*';
    p[len-5] = '\0';
    if (wordexp(p, &word_vector, 0)) {
	solv_free(p);
	return ret;
    }
    for (int i = 0; i < word_vector.we_wordc; ++i) {
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
    solv_free(p);
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
    void *h = solv_chksum_create(CHKSUM_TYPE);
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
    void *h = solv_chksum_create(CHKSUM_TYPE);
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

void
checksum_dump(const unsigned char *cs)
{
    for (int i = 0; i < CHKSUM_BYTES; i+=4) {
	printf("%02x%02x%02x%02x", cs[i], cs[i+1], cs[i+2], cs[i+3]);
	if (i + 4 >= CHKSUM_BYTES)
	    printf("\n");
	else
	    printf(" : ");
    }
}

int
checksum_type2length(int type)
{
    switch(type) {
    case HY_CHKSUM_MD5:
	return 16;
    case HY_CHKSUM_SHA1:
	return 20;
    case HY_CHKSUM_SHA256:
	return 32;
    case HY_CHKSUM_SHA512:
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
	return HY_CHKSUM_MD5;
    case REPOKEY_TYPE_SHA1:
	return 	HY_CHKSUM_SHA1;
    case REPOKEY_TYPE_SHA256:
	return HY_CHKSUM_SHA256;
    case REPOKEY_TYPE_SHA512:
	return HY_CHKSUM_SHA512;
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
    if (len <= 1) {
	hy_errno = HY_E_OP;
	return NULL;
    }

    if (path[0] == '/')
	return solv_strdup(path);

    char cwd[PATH_MAX];
    if (!getcwd(cwd, PATH_MAX)) {
	hy_errno = HY_E_FAILED;
	return NULL;
    }

    return solv_dupjoin(cwd, "/", path);
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

    char *p = solv_strdup(path);

    if (p[len-1] == '/')
	p[len-1] = '\0';

    if (access(p, X_OK)) {
	*(strrchr(p, '/')) = '\0';
	ret = mkcachedir(p);
	if (str_endswith(path, "XXXXXX")) {
	    char *retptr = mkdtemp(path);
	    if (retptr == NULL)
		ret |= 1;
	} else
	    ret |= mkdir(path, CACHEDIR_PERMISSIONS);
    } else {
	ret = 0;
    }

    solv_free(p);
    return ret;
}

int
mv(HySack sack, const char *old, const char *new)
{
    if (rename(old, new)) {
	HY_LOG_ERROR(format_err_str("Failed renaming %s to %s: %s", old, new,
				    strerror(errno)));
	return HY_E_IO;
    }
    if (chmod(new, 0666 & ~get_umask())) {
	HY_LOG_ERROR(format_err_str("Failed setting perms on %s: %s", new,
				    strerror(errno)));
	return HY_E_IO;
    }
    return 0;
}

char *
this_username(void)
{
    const struct passwd *pw = getpwuid(getuid());
    return solv_strdup(pw->pw_name);
}

unsigned
count_nullt_array(const char **a)
{
    const char **strp = a;
    while (*strp) strp++;
    return strp - a;
}

const char *
ll_name(int level)
{
    switch (level) {
    case SOLV_FATAL:
	return "LFATAL";
    case SOLV_ERROR:
	return "LSERROR";
    case SOLV_WARN:
	return "LSWARN";
    case SOLV_DEBUG_RESULT:
	return "LSRESULT";
    case HY_LL_ERROR:
	return "ERROR";
    case HY_LL_INFO:
	return "INFO";
    default:
	return "(level?)";
    }
}

char *
read_whole_file(const char *path)
{
  char *contents = NULL;
  if (!g_file_get_contents (path, &contents, NULL, NULL))
    return NULL;
  return contents;
}

int
str_endswith(const char *haystack, const char *needle)
{
    const int lenh = strlen(haystack);
    const int lenn = strlen(needle);

    if (lenn > lenh)
	return 0;
    return strncmp(haystack + lenh - lenn, needle, lenn) == 0 ? 1 : 0;
}

int
str_startswith(const char *haystack, const char *needle)
{
    const int lenh = strlen(haystack);
    const int lenn = strlen(needle);

    if (lenn > lenh)
	return 0;
    return !strncmp(haystack, needle, lenn);
}

char *
pool_tmpdup(Pool *pool, const char *s)
{
    char *dup = pool_alloctmpspace(pool, strlen(s) + 1);
    return strcpy(dup, s);
}

/* can go once there is solv_strndup() */
char *
hy_strndup(const char *s, size_t n)
{
  if (!s)
    return 0;

  char *r = strndup(s, n);
  if (!r)
    solv_oom(0, n);
  return r;
}

Id
running_kernel(HySack sack)
{
    Pool *pool = sack_pool(sack);
    struct utsname un;

    uname(&un);
    char *fn = pool_tmpjoin(pool, "/boot/vmlinuz-", un.release, NULL);
    if (access(fn, F_OK)) {
	HY_LOG_ERROR("running_kernel(): no matching file: %s.", fn);
	return -1;
    }

    Id kernel_id = -1;
    HyQuery q = hy_query_create(sack);
    sack_make_provides_ready(sack);
    hy_query_filter(q, HY_PKG_FILE, HY_EQ, fn);
    hy_query_filter(q, HY_PKG_REPONAME, HY_EQ, HY_SYSTEM_REPO_NAME);
    HyPackageSet pset = hy_query_run_set(q);
    if (hy_packageset_count(pset) > 0)
	kernel_id = packageset_get_pkgid(pset, 0, -1);
    hy_packageset_free(pset);
    hy_query_free(q);

    if (kernel_id >= 0)
	HY_LOG_INFO("running_kernel(): %s.", id2nevra(pool, kernel_id));
    else
	HY_LOG_INFO("running_kernel(): running kernel not matched to a package.");
    return kernel_id;
}

int
cmptype2relflags(int type)
{
    int flags = 0;
    if (type & HY_EQ)
	flags |= REL_EQ;
    if (type & HY_LT)
	flags |= REL_LT;
    if (type & HY_GT)
	flags |= REL_GT;
    assert(flags);
    return flags;
}

Repo *
repo_by_name(HySack sack, const char *name)
{
    Pool *pool = sack_pool(sack);
    Repo *repo;
    int repoid;

    FOR_REPOS(repoid, repo) {
	if (!strcmp(repo->name, name))
	    return repo;
    }
    return NULL;
}

HyRepo
hrepo_by_name(HySack sack, const char *name)
{
    Repo *repo = repo_by_name(sack, name);

    if (repo)
	return repo->appdata;
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

void
queue2plist(HySack sack, Queue *q, HyPackageList plist)
{
    for (int i = 0; i < q->count; ++i)
	hy_packagelist_push(plist, package_create(sack, q->elements[i]));
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

int
dump_jobqueue(Pool *pool, Queue *job)
{
    for (int i = 0; i < job->count; i+=2)
	    printf("\t%s\n", pool_job2str(pool, job->elements[i],
					  job->elements[i+1], 0));
    return job->count;
}

int
dump_nullt_array(const char **a)
{
    const char **strp = a;
    while (*strp)
	printf("%s\n", *strp++);
    return strp - a;
}

int
dump_solvables_queue(Pool *pool, Queue *q)
{
    for (int i = 0; i < q->count; ++i)
	printf("%s\n", pool_solvid2str(pool, q->elements[i]));
    return q->count;
}

int
dump_map(Pool *pool, Map *m)
{
    unsigned c = 0;
    printf("(size: %d) ", m->size);
    for (Id id = 0; id < m->size << 3; ++id)
	if (MAPTST(m, id)) {
	    c++;
	    printf("%d:", id);
	}
    printf("\n");
    return c;
}

const char *
id2nevra(Pool *pool, Id id)
{
    Solvable *s = pool_id2solvable(pool, id);
    return pool_solvable2str(pool, s);
}

int
copy_str_from_subexpr(char** target, const char* source,
    regmatch_t* matches, int i)
{
    int subexpr_len = matches[i].rm_eo - matches[i].rm_so;
    if (subexpr_len == 0)
	return -1;
    *target = solv_malloc(sizeof(char*) * (subexpr_len + 1));
    strncpy(*target, &(source[matches[i].rm_so]), subexpr_len);
    (*target)[subexpr_len] = '\0';
    return 0;
}

static int
get_cmp_flags(int *cmp_type, const char* source,
    regmatch_t* matches, int i)
{
    int subexpr_len = matches[i].rm_eo - matches[i].rm_so;
    const char *match_start = &(source[matches[i].rm_so]);
    if (subexpr_len == 2) {
	if (strncmp(match_start, "!=", 2) == 0)
	    *cmp_type |= HY_NEQ;
	else if (strncmp(match_start, "<=", 2) == 0) {
	    *cmp_type |= HY_LT;
	    *cmp_type |= HY_EQ;
	}
	else if (strncmp(match_start, ">=", 2) == 0) {
	    *cmp_type |= HY_GT;
	    *cmp_type |= HY_EQ;
	}
	else
	    return -1;
    } else if (subexpr_len == 1) {
	if (*match_start == '<')
	    *cmp_type |= HY_LT;
	else if (*match_start == '>')
	    *cmp_type |= HY_GT;
	else if (*match_start == '=')
	    *cmp_type |= HY_EQ;
	else
	    return -1;
    } else
	return -1;
    return 0;
}

/**
 * Copies parsed name and evr from reldep_str. If reldep_str is valid
 * returns 0, otherwise returns -1. When parsing is successful, name
 * and evr strings need to be freed after usage.
 */
int
parse_reldep_str(const char *reldep_str, char **name, char **evr,
    int *cmp_type)
{
    regex_t reg;
    const char *regex =
        "^(\\S*)\\s*(<=|>=|!=|<|>|=)?\\s*(.*)$";
    regmatch_t matches[6];
    *cmp_type = 0;
    int ret = 0;

    regcomp(&reg, regex, REG_EXTENDED);

    if(regexec(&reg, reldep_str, 6, matches, 0) == 0) {
	if (copy_str_from_subexpr(name, reldep_str, matches, 1) == -1)
	    ret = -1;
	// without comparator and evr
	else if ((matches[2].rm_eo - matches[2].rm_so) == 0 &&
	    (matches[3].rm_eo - matches[3].rm_so) == 0)
	    ret = 0;
	else if (get_cmp_flags(cmp_type, reldep_str, matches, 2) == -1 ||
	    copy_str_from_subexpr(evr, reldep_str, matches, 3) == -1) {
	    solv_free(*name);
	    ret = -1;
	}
    } else
	ret = -1;

    regfree(&reg);
    return ret;
}

HyReldep
reldep_from_str(HySack sack, const char *reldep_str)
{
    char *name, *evr = NULL;
    int cmp_type = 0;
    if (parse_reldep_str(reldep_str, &name, &evr, &cmp_type) == -1)
	return NULL;
    HyReldep reldep = hy_reldep_create(sack, name, cmp_type, evr);
    solv_free(name);
    solv_free(evr);
    return reldep;
}

HyReldepList
reldeplist_from_str(HySack sack, const char *reldep_str)
{
    int cmp_type;
    char *name_glob = NULL;
    char *evr = NULL;
    Dataiterator di;
    Pool *pool = sack_pool(sack);

    HyReldepList reldeplist = hy_reldeplist_create(sack);
    parse_reldep_str(reldep_str, &name_glob, &evr, &cmp_type);

    dataiterator_init(&di, pool, 0, 0, 0, name_glob, SEARCH_STRING | SEARCH_GLOB);
    while (dataiterator_step(&di)) {
        HyReldep reldep = hy_reldep_create(sack, di.kv.str, cmp_type, evr);
        if (reldep)
            hy_reldeplist_add(reldeplist, reldep);
        hy_reldep_free(reldep);
    }

    dataiterator_free(&di);
    solv_free(name_glob);
    solv_free(evr);
    return reldeplist;
}
