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
#include <ctype.h>
#include <sys/utsname.h>
#include <sys/stat.h>

#ifdef __APPLE__
typedef int auxv_t;
#ifndef AT_HWCAP2
#define AT_HWCAP2 26
#endif
#ifndef AT_HWCAP
#define AT_HWCAP 16
#endif
static unsigned long getauxval(unsigned long type)
{
  unsigned long ret = 0;
  return ret;
}
#else
#include <sys/auxv.h>
#endif

// hawkey
#include "dnf-types.h"
#include "hy-iutil-private.hpp"
#include "nevra.hpp"
#include "hy-package.h"
#include "hy-subject.h"
#include "hy-util-private.hpp"

#include <memory>

/* ARM specific HWCAP defines may be missing on non-ARM devices */
#ifndef HWCAP_ARM_VFP
#define HWCAP_ARM_VFP	(1<<6)
#endif
#ifndef HWCAP_ARM_NEON
#define HWCAP_ARM_NEON	(1<<12)
#endif

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
    case G_CHECKSUM_SHA384:
        return "sha384";
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
    if (!strcasecmp(chksum_name, "sha384"))
        return G_CHECKSUM_SHA384;
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

    if (!strncmp(un.machine, "armv", 4)) {
        /* un.machine is armvXE, where X is version number and E is
         * endianness (b or l); we need to add modifiers such as
         * h (hardfloat), n (neon). Neon is a requirement of armv8 so
         * as far as rpm is concerned armv8l is the equivilent of armv7hnl
         * (or 7hnb) so we don't explicitly add 'n' for 8+ as it's expected. */
        char endian = un.machine[strlen(un.machine)-1];
        char *modifier = un.machine + 5;
        while(isdigit(*modifier)) /* keep armv7, armv8, armv9, armv10, armv100, ... */
            modifier++;
        if (getauxval(AT_HWCAP) & HWCAP_ARM_VFP)
            *modifier++ = 'h';
        if ((atoi(un.machine+4) == 7) && (getauxval(AT_HWCAP) & HWCAP_ARM_NEON))
            *modifier++ = 'n';
        *modifier++ = endian;
        *modifier = 0;
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
        if (arch)
            *arch = g_strdup(nevraObj.getArch().c_str());
        if (name)
            *name = g_strdup(nevraObj.getName().c_str());
        if (release)
            *release = g_strdup(nevraObj.getRelease().c_str());
        if (version)
            *version = g_strdup(nevraObj.getVersion().c_str());
        if (epoch) {
            *epoch = nevraObj.getEpoch();
            if (*epoch == -1)
                *epoch = 0;
        }
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

int
mtime(const char *filename)
{
    struct stat st;
    stat(filename, &st);
    return st.st_mtime;
}
