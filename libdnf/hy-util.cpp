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

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>
#include <sys/stat.h>

// hawkey
#include "dnf-types.h"
#include "hy-iutil-private.hpp"
#include "nevra.hpp"
#include "hy-package.h"
#include "hy-subject.h"
#include "hy-util-private.hpp"

#include <memory>

enum _dnf_sack_cpu_flags {
    ARM_NEON = 1 << 0,
    ARM_VFP3  = 1 << 1,
    ARM_VFP  = 1 << 2
};

static int
parse_cpu_flags(int *flags, const char *section)
{
    char *cpuinfo = read_whole_file("/proc/cpuinfo");
    if (cpuinfo == NULL)
        return DNF_ERROR_FAILED;

    char *features = strstr(cpuinfo, section);
    if (features != NULL) {
        char *saveptr;
        features = strtok_r(features, "\n", &saveptr);
        char *tok = strtok_r(features, " ", &saveptr);
        while (tok) {
            if (!strcmp(tok, "neon"))
                *flags |= ARM_NEON;
            else if (!strcmp(tok, "vfpv3"))
                *flags |= ARM_VFP3;
            else if (!strcmp(tok, "vfp"))
                *flags |= ARM_VFP;
            tok = strtok_r(NULL, " ", &saveptr);
        }
    }

    g_free(cpuinfo);
    return 0;
}

const char *
hy_chksum_name(int chksum_type)
{
    switch (chksum_type) {
    case G_CHECKSUM_MD5:
        return "md5";
    case G_CHECKSUM_SHA1:
        return "sha1";
    case G_CHECKSUM_SHA256:
        return "sha256";
    case G_CHECKSUM_SHA512:
        return "sha512";
    default:
        return NULL;
    }
}

int
hy_chksum_type(const char *chksum_name)
{
    if (!strcasecmp(chksum_name, "md5"))
        return G_CHECKSUM_MD5;
    if (!strcasecmp(chksum_name, "sha1"))
        return G_CHECKSUM_SHA1;
    if (!strcasecmp(chksum_name, "sha256"))
        return G_CHECKSUM_SHA256;
    if (!strcasecmp(chksum_name, "sha512"))
        return G_CHECKSUM_SHA512;
    return 0;
}

char *
hy_chksum_str(const unsigned char *chksum, int type)
{
    int length = checksum_type2length(type);
    if (length==-1)
        return NULL;
    char *s = static_cast<char *>(g_malloc(2 * length + 1));
    solv_bin2hex(chksum, length, s);

    return s;
}

#define MAX_ARCH_LENGTH 64

int
hy_detect_arch(char **arch)
{
    struct utsname un;

    if (uname(&un) < 0)
        return DNF_ERROR_FAILED;

    if (!strcmp(un.machine, "armv6l")) {
        int flags = 0;
        int ret = parse_cpu_flags(&flags, "Features");
        if (ret)
            return ret;
        if (flags & ARM_VFP)
            strcpy(un.machine, "armv6hl");
    }
    if (!strcmp(un.machine, "armv7l")) {
        int flags = 0;
        int ret = parse_cpu_flags(&flags, "Features");
        if (ret)
            return ret;
        if (flags & ARM_VFP3) {
            if (flags & ARM_NEON)
                strcpy(un.machine, "armv7hnl");
            else
                strcpy(un.machine, "armv7hl");
        }
    }
#ifdef __MIPSEL__
    if (!strcmp(un.machine, "mips"))
        strcpy(un.machine, "mipsel");
    else if (!strcmp(un.machine, "mips64"))
        strcpy(un.machine, "mips64el");
#endif
    *arch = g_strdup(un.machine);
    return 0;
}

#undef MAX_ARCH_LENGTH

gboolean
hy_is_glob_pattern(const char *pattern)
{
    return strpbrk(pattern, "*[?") != NULL;
}

int
hy_split_nevra(const char *nevra, char **name, int *epoch,
               char **version, char **release, char **arch)
{
    if (strlen(nevra) <= 0)
        return DNF_ERROR_INTERNAL_ERROR;
    libdnf::Nevra nevraObj;
    if (nevraObj.parse(nevra, HY_FORM_NEVRA)) {
        *arch = g_strdup(nevraObj.getArch().c_str());
        *name = g_strdup(nevraObj.getName().c_str());
        *release = g_strdup(nevraObj.getRelease().c_str());
        *version = g_strdup(nevraObj.getVersion().c_str());
        *epoch = nevraObj.getEpoch();
        if (*epoch == -1)
            *epoch = 0;
        return 0;
    }
    return DNF_ERROR_INTERNAL_ERROR;
}

GPtrArray *
hy_packagelist_create(void)
{
    return (GPtrArray *)g_ptr_array_new_with_free_func ((GDestroyNotify)g_object_unref);
}

int
hy_packagelist_has(GPtrArray *plist, DnfPackage *pkg)
{
    GPtrArray *a = (GPtrArray*)plist;
    for (guint i = 0; i < a->len; ++i)
        if (dnf_package_get_identical(pkg, static_cast<DnfPackage *>(a->pdata[i])))
            return 1;
    return 0;
}

// [WIP]
int
mtime(const char *filename)
{
    struct stat st;
    stat(filename, &st);
    return st.st_mtime;
}

// [WIP]
unsigned long
age(const char *filename)
{
    return time(NULL) - mtime(filename);
}

// [WIP]
const char *
cksum(const char *filename, GChecksumType ctype)
{
    FILE *fp;
    if (!(fp = fopen(filename, "rb")))
        return "";
    guchar buffer[4096];
    gssize len = 0;
    GChecksum *csum = g_checksum_new(ctype);
    while ((len = fread(buffer, 1, sizeof(buffer), fp)) > 0)
        g_checksum_update(csum, buffer, len);
    gchar *result = g_strdup(g_checksum_get_string(csum));
    g_checksum_free(csum);
    return result;
}

// [WIP]
void
rmtree(const char *dir)
{
    // FIXME: Implement our own version?
    dnf_remove_recursive(dir, NULL);
}

// [WIP]
void
copy_yum_repo(LrYumRepo *dst, LrYumRepo *src)
{
    // strings
    dst->repomd = g_strdup(src->repomd);
    dst->url = g_strdup(src->url);
    dst->destdir = g_strdup(src->destdir);
    dst->signature = g_strdup(src->signature);
    dst->mirrorlist = g_strdup(src->mirrorlist);
    dst->metalink = g_strdup(src->metalink);

    // paths list
    GSList *elem_src = src->paths;
    GSList *elem_dst = g_slist_copy(src->paths);
    dst->paths = elem_dst;
    while (elem_src && elem_dst) {
        elem_dst->data = lr_malloc(sizeof(LrYumRepoPath));
        LrYumRepoPath *path_src = (LrYumRepoPath *)elem_src->data;
        LrYumRepoPath *path_dst = (LrYumRepoPath *)elem_dst->data;
        path_dst->type = g_strdup(path_src->type);
        path_dst->path = g_strdup(path_src->path);
        elem_src = g_slist_next(elem_src);
        elem_dst = g_slist_next(elem_dst);
    }
}

// [WIP]
void
copy_yum_repomd(LrYumRepoMd *dst, LrYumRepoMd *src)
{
    GSList *elem_src;
    GSList *elem_dst;

    // strings
    dst->revision = g_strdup(src->revision);
    dst->repoid = g_strdup(src->repoid);
    dst->repoid_type = g_strdup(src->repoid_type);
    dst->chunk = NULL;

    // string lists
    dst->repo_tags = g_slist_copy_deep(src->repo_tags,
                                       (GCopyFunc) g_strdup, NULL);
    dst->content_tags = g_slist_copy_deep(src->content_tags,
                                          (GCopyFunc) g_strdup, NULL);

    // distro_tags list
    elem_src = src->distro_tags;
    elem_dst = g_slist_copy(src->distro_tags);
    dst->distro_tags = elem_dst;
    while (elem_src && elem_dst) {
        elem_dst->data = lr_malloc(sizeof(LrYumDistroTag));
        LrYumDistroTag *tag_src = (LrYumDistroTag *)elem_src->data;
        LrYumDistroTag *tag_dst = (LrYumDistroTag *)elem_dst->data;

        // copy the strings
        tag_dst->cpeid = g_strdup(tag_src->cpeid);
        tag_dst->tag = g_strdup(tag_src->tag);

        elem_src = g_slist_next(elem_src);
        elem_dst = g_slist_next(elem_dst);
    }

    // records list
    elem_src = src->records;
    elem_dst = g_slist_copy(src->records);
    dst->records = elem_dst;
    while (elem_src && elem_dst) {
        elem_dst->data = lr_malloc(sizeof(LrYumRepoMdRecord));
        LrYumRepoMdRecord *rec_src = (LrYumRepoMdRecord *)elem_src->data;
        LrYumRepoMdRecord *rec_dst = (LrYumRepoMdRecord *)elem_dst->data;

        // copy the strings
        rec_dst->type = g_strdup(rec_src->type);
        rec_dst->location_href = g_strdup(rec_src->location_href);
        rec_dst->location_base = g_strdup(rec_src->location_base);
        rec_dst->checksum = g_strdup(rec_src->checksum);
        rec_dst->checksum_type = g_strdup(rec_src->checksum_type);
        rec_dst->checksum_open = g_strdup(rec_src->checksum_open);
        rec_dst->checksum_open_type = g_strdup(rec_src->checksum_open_type);
        rec_dst->chunk = NULL;
        // copy the ints
        rec_dst->timestamp = rec_src->timestamp;
        rec_dst->size = rec_src->size;
        rec_dst->size_open = rec_src->size_open;
        rec_dst->db_version = rec_src->db_version;

        elem_src = g_slist_next(elem_src);
        elem_dst = g_slist_next(elem_dst);
    }
}
