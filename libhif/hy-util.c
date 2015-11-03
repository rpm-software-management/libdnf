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
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/utsname.h>

// hawkey
#include "hif-types.h"
#include "hy-iutil.h"
#include "hy-package.h"
#include "hy-util.h"

enum _hif_sack_cpu_flags {
    ARM_NEON = 1 << 0,
    ARM_VFP3  = 1 << 1,
    ARM_VFP  = 1 << 2
};

static int
parse_cpu_flags(int *flags, const char *section)
{
    char *cpuinfo = read_whole_file("/proc/cpuinfo");
    if (cpuinfo == NULL)
        return HIF_ERROR_FAILED;

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
    char *s = g_malloc(2 * length + 1);
    solv_bin2hex(chksum, length, s);

    return s;
}

#define MAX_ARCH_LENGTH 64

int
hy_detect_arch(char **arch)
{
    struct utsname un;

    if (uname(&un))
        return HIF_ERROR_FAILED;

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
        if (flags & (ARM_NEON | ARM_VFP3))
            strcpy(un.machine, "armv7hnl");
        else if (flags & ARM_VFP3)
            strcpy(un.machine, "armv7hl");
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

int
hy_split_nevra(const char *nevra, char **name, long int *epoch,
               char **version, char **release, char **arch)
{
    const int len = strlen(nevra);
    if (len <= 0)
        return HIF_ERROR_INTERNAL_ERROR;

    const char *m1 = NULL, *m2 = NULL, *m3 = NULL;
    const char *c;
    for (c = nevra + len - 1; c > nevra; --c)
        if (*c == '.') {
            m3 = c;
            break;
        }
    if (c == nevra)
        return HIF_ERROR_INTERNAL_ERROR;

    for (; c > nevra; --c)
        if (*c == '-') {
            if (m2 == NULL)
                m2 = c;
            else if (m1 == NULL) {
                m1 = c;
                break;
            }
        }
    if (c == nevra)
        return HIF_ERROR_INTERNAL_ERROR;

    *arch = g_strdup(m3+1);
    *name = g_strndup(nevra, (m1 - nevra));
    *release = g_strndup(m2 + 1, (m3 - m2 - 1));

    char *endptr;
    long int converted;
    errno = 0;
    converted = strtol(m1 + 1, &endptr, 10);
    if (!errno && *endptr == ':') {
        *epoch = converted;
        *version = g_strndup(endptr + 1, (m2 - endptr - 1));
    } else {
        *epoch = 0;
        *version = g_strndup(m1 + 1, (m2 - m1 - 1));
    }

    return 0;
}

GPtrArray *
hy_packagelist_create(void)
{
    return (GPtrArray *)g_ptr_array_new_with_free_func ((GDestroyNotify)g_object_unref);
}

int
hy_packagelist_has(GPtrArray *plist, HifPackage *pkg)
{
    GPtrArray *a = (GPtrArray*)plist;
    for (guint i = 0; i < a->len; ++i)
        if (hif_package_get_identical(pkg, a->pdata[i]))
            return 1;
    return 0;
}
