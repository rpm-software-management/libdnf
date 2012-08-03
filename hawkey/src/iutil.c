#define _GNU_SOURCE
#include <assert.h>
#include <pwd.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <wordexp.h>

// libsolv
#include "solv/chksum.h"
#include "solv/evr.h"
#include "solv/solver.h"
#include "solv/solverdebug.h"

// hawkey
#include "iutil.h"
#include "package_internal.h"
#include "sack_internal.h"

#define CHKSUM_TYPE REPOKEY_TYPE_SHA256
#define CHKSUM_IDENT "H000"
#define CACHEDIR_PERMISSIONS 0700

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
    default:
	assert(0);
	return -1;
    }
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

    char *p = solv_strdup(path);
    int len = strlen(p);

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

int
str_endswith(const char *haystack, const char *needle)
{
    const int lenh = strlen(haystack);
    const int lenn = strlen(needle);

    if (lenn > lenh)
	return 0;
    return strncmp(haystack + lenh - lenn, needle, lenn) == 0 ? 1 : 0;
}

void
repo_internalize_trigger(Repo *repo)
{
    if (strcmp(repo->name, HY_CMDLINE_REPO_NAME))
	return; /* this is only done for the cmdline repo, the ordinary ones get
		   internalized immediately */
    repo_internalize(repo);
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
    Solvable *s;
    int i;

    for (i = 0; i < q->count; ++i) {
	s = pool_id2solvable(sack_pool(sack), q->elements[i]);
	hy_packagelist_push(plist, package_from_solvable(s));
    }
}

/**
 * Return id of a package that can be upgraded with pkg.
 *
 * The returned package Id fulfills the following criteria:
 * :: it is installed
 * :: has the same name and arch as pkg
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
	    updated->name != s->name ||
	    updated->arch != s->arch)
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
