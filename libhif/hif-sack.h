/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2012-2015 Red Hat, Inc.
 * Copyright (C) 2015 Richard Hughes <richard@hughsie.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#if !defined (__LIBHIF_H) && !defined (HIF_COMPILATION)
#error "Only <libhif.h> can be included directly."
#endif

#ifndef __HIF_SACK_H
#define __HIF_SACK_H

#include <glib-object.h>

#include "hy-types.h"
#include "hif-sack.h"
#include "hif-source.h"
#include "hif-state.h"

G_BEGIN_DECLS

#define HIF_TYPE_SACK (hif_sack_get_type ())
G_DECLARE_DERIVABLE_TYPE (HifSack, hif_sack, HIF, SACK, GObject)

struct _HifSackClass
{
        GObjectClass            parent_class;
        /*< private >*/
        void (*_hif_reserved1)  (void);
        void (*_hif_reserved2)  (void);
        void (*_hif_reserved3)  (void);
        void (*_hif_reserved4)  (void);
        void (*_hif_reserved5)  (void);
        void (*_hif_reserved6)  (void);
        void (*_hif_reserved7)  (void);
        void (*_hif_reserved8)  (void);
};

/**
 * HifSackSetupFlags:
 * @HIF_SACK_SETUP_FLAG_NONE:                   No flags set
 * @HIF_SACK_SETUP_FLAG_MAKE_CACHE_DIR:         Create the cache dir if required
 *
 * Flags to use when setting up the sack.
 **/
typedef enum {
    HIF_SACK_SETUP_FLAG_NONE                = 0,
    HIF_SACK_SETUP_FLAG_MAKE_CACHE_DIR      = 1 << 0,
    /*< private >*/
    HIF_SACK_SETUP_FLAG_LAST
} HifSackSetupFlags;

/**
 * HifSackAddFlags:
 * @HIF_SACK_LOAD_FLAG_NONE:                    No flags set
 * @HIF_SACK_LOAD_FLAG_BUILD_CACHE:             Build the solv cache if required
 * @HIF_SACK_LOAD_FLAG_USE_FILELISTS:           Use the filelists metadata
 * @HIF_SACK_LOAD_FLAG_USE_PRESTO:              Use presto deltas metadata
 * @HIF_SACK_LOAD_FLAG_USE_UPDATEINFO:          Use updateinfo metadata
 *
 * Flags to use when loading from the sack.
 **/
typedef enum {
    HIF_SACK_LOAD_FLAG_NONE                 = 0,
    HIF_SACK_LOAD_FLAG_BUILD_CACHE          = 1 << 0,
    HIF_SACK_LOAD_FLAG_USE_FILELISTS        = 1 << 1,
    HIF_SACK_LOAD_FLAG_USE_PRESTO           = 1 << 2,
    HIF_SACK_LOAD_FLAG_USE_UPDATEINFO       = 1 << 3,
    /*< private >*/
    HIF_SACK_LOAD_FLAG_LAST
} HifSackLoadFlags;

HifSack     *hif_sack_new                   (void);

void         hif_sack_set_cachedir          (HifSack        *sack,
                                             const gchar    *value);
gboolean     hif_sack_set_arch              (HifSack        *sack,
                                             const gchar    *value,
                                             GError        **error);
void         hif_sack_set_rootdir           (HifSack        *sack,
                                             const gchar    *value);
gboolean     hif_sack_setup                 (HifSack        *sack,
                                             int             flags,
                                             GError        **error);
int          hif_sack_evr_cmp               (HifSack        *sack,
                                             const char     *evr1,
                                             const char     *evr2);
const char  *hif_sack_get_cache_dir         (HifSack        *sack);
HyPackage    hif_sack_get_running_kernel    (HifSack        *sack);
char        *hif_sack_give_cache_fn         (HifSack        *sack,
                                             const char     *reponame,
                                             const char     *ext);
const char **hif_sack_list_arches           (HifSack        *sack);
void         hif_sack_set_installonly       (HifSack        *sack,
                                             const char    **installonly);
void         hif_sack_set_installonly_limit (HifSack        *sack,
                                             guint           limit);
guint        hif_sack_get_installonly_limit (HifSack        *sack);
HyPackage    hif_sack_add_cmdline_package   (HifSack        *sack,
                                             const char     *fn);
int          hif_sack_count                 (HifSack        *sack);
void         hif_sack_add_excludes          (HifSack        *sack,
                                             HyPackageSet    pset);
void         hif_sack_add_includes          (HifSack        *sack,
                                             HyPackageSet    pset);
void         hif_sack_set_excludes          (HifSack        *sack,
                                             HyPackageSet    pset);
void         hif_sack_set_includes          (HifSack        *sack,
                                             HyPackageSet    pset);
int          hif_sack_repo_enabled          (HifSack        *sack,
                                             const char     *reponame,
                                             int             enabled);
gboolean     hif_sack_load_system_repo      (HifSack        *sack,
                                             HyRepo          a_hrepo,
                                             int             flags,
                                             GError        **error);
gboolean     hif_sack_load_repo             (HifSack        *sack,
                                             HyRepo          hrepo,
                                             int             flags,
                                             GError        **error);

/**********************************************************************/

/**
 * HifSackAddFlags:
 * @HIF_SACK_ADD_FLAG_NONE:                     Add the primary
 * @HIF_SACK_ADD_FLAG_FILELISTS:                Add the filelists
 * @HIF_SACK_ADD_FLAG_UPDATEINFO:               Add the updateinfo
 * @HIF_SACK_ADD_FLAG_REMOTE:                   Use remote sources
 * @HIF_SACK_ADD_FLAG_UNAVAILABLE:              Add sources that are unavailable
 *
 * The error code.
 **/
typedef enum {
        HIF_SACK_ADD_FLAG_NONE                  = 0,
        HIF_SACK_ADD_FLAG_FILELISTS             = 1,
        HIF_SACK_ADD_FLAG_UPDATEINFO            = 2,
        HIF_SACK_ADD_FLAG_REMOTE                = 4,
        HIF_SACK_ADD_FLAG_UNAVAILABLE           = 8,
        /*< private >*/
        HIF_SACK_ADD_FLAG_LAST
} HifSackAddFlags;

gboolean         hif_sack_add_source            (HifSack        *sack,
                                                 HifSource      *src,
                                                 guint           permissible_cache_age,
                                                 HifSackAddFlags flags,
                                                 HifState       *state,
                                                 GError         **error);
gboolean         hif_sack_add_sources           (HifSack        *sack,
                                                 GPtrArray      *sources,
                                                 guint           permissible_cache_age,
                                                 HifSackAddFlags flags,
                                                 HifState       *state,
                                                 GError         **error);

G_END_DECLS

#endif /* __HIF_SACK_H */
