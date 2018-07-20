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

#ifndef __DNF_SACK_H
#define __DNF_SACK_H

#include <glib-object.h>

#include "hy-types.h"
#include "dnf-sack.h"
#include "dnf-repo.h"
#include "dnf-state.h"
#include "hy-packageset.h"

G_BEGIN_DECLS

#define DNF_TYPE_SACK (dnf_sack_get_type ())
G_DECLARE_DERIVABLE_TYPE (DnfSack, dnf_sack, DNF, SACK, GObject)

struct _DnfSackClass
{
        GObjectClass            parent_class;
        /*< private >*/
        void (*_dnf_reserved1)  (void);
        void (*_dnf_reserved2)  (void);
        void (*_dnf_reserved3)  (void);
        void (*_dnf_reserved4)  (void);
        void (*_dnf_reserved5)  (void);
        void (*_dnf_reserved6)  (void);
        void (*_dnf_reserved7)  (void);
        void (*_dnf_reserved8)  (void);
};

/**
 * DnfSackSetupFlags:
 * @DNF_SACK_SETUP_FLAG_NONE:                   No flags set
 * @DNF_SACK_SETUP_FLAG_MAKE_CACHE_DIR:         Create the cache dir if required
 *
 * Flags to use when setting up the sack.
 **/
typedef enum {
    DNF_SACK_SETUP_FLAG_NONE                = 0,
    DNF_SACK_SETUP_FLAG_MAKE_CACHE_DIR      = 1 << 0,
    /*< private >*/
    DNF_SACK_SETUP_FLAG_LAST
} DnfSackSetupFlags;

/**
 * DnfSackAddFlags:
 * @DNF_SACK_LOAD_FLAG_NONE:                    No flags set
 * @DNF_SACK_LOAD_FLAG_BUILD_CACHE:             Build the solv cache if required
 * @DNF_SACK_LOAD_FLAG_USE_FILELISTS:           Use the filelists metadata
 * @DNF_SACK_LOAD_FLAG_USE_PRESTO:              Use presto deltas metadata
 * @DNF_SACK_LOAD_FLAG_USE_UPDATEINFO:          Use updateinfo metadata
 *
 * Flags to use when loading from the sack.
 **/
typedef enum {
    DNF_SACK_LOAD_FLAG_NONE                 = 0,
    DNF_SACK_LOAD_FLAG_BUILD_CACHE          = 1 << 0,
    DNF_SACK_LOAD_FLAG_USE_FILELISTS        = 1 << 1,
    DNF_SACK_LOAD_FLAG_USE_PRESTO           = 1 << 2,
    DNF_SACK_LOAD_FLAG_USE_UPDATEINFO       = 1 << 3,
    /*< private >*/
    DNF_SACK_LOAD_FLAG_LAST
} DnfSackLoadFlags;

DnfSack     *dnf_sack_new                   (void);

void         dnf_sack_set_cachedir          (DnfSack        *sack,
                                             const gchar    *value);
gboolean     dnf_sack_set_arch              (DnfSack        *sack,
                                             const gchar    *value,
                                             GError        **error);
void         dnf_sack_set_all_arch          (DnfSack        *sack,
                                             gboolean        all_arch);
gboolean     dnf_sack_get_all_arch          (DnfSack        *sack);
void         dnf_sack_set_rootdir           (DnfSack        *sack,
                                             const gchar    *value);
gboolean     dnf_sack_setup                 (DnfSack        *sack,
                                             int             flags,
                                             GError        **error);
int          dnf_sack_evr_cmp               (DnfSack        *sack,
                                             const char     *evr1,
                                             const char     *evr2);
const char  *dnf_sack_get_cache_dir         (DnfSack        *sack);
DnfPackage  *dnf_sack_get_running_kernel    (DnfSack        *sack);
char        *dnf_sack_give_cache_fn         (DnfSack        *sack,
                                             const char     *reponame,
                                             const char     *ext);
const char **dnf_sack_list_arches           (DnfSack        *sack);
void         dnf_sack_set_installonly       (DnfSack        *sack,
                                             const char    **installonly);
void         dnf_sack_set_installonly_limit (DnfSack        *sack,
                                             guint           limit);
guint        dnf_sack_get_installonly_limit (DnfSack        *sack);
DnfPackage  *dnf_sack_add_cmdline_package   (DnfSack        *sack,
                                             const char     *fn);
DnfPackage  *dnf_sack_add_cmdline_package_nochecksum   (DnfSack        *sack,
                                             const char     *fn);
int          dnf_sack_count                 (DnfSack        *sack);
void         dnf_sack_add_excludes          (DnfSack        *sack,
                                             DnfPackageSet  *pset);
void         dnf_sack_add_module_excludes   (DnfSack        *sack,
                                             DnfPackageSet *pset);
void         dnf_sack_add_includes          (DnfSack        *sack,
                                             DnfPackageSet  *pset);
void         dnf_sack_remove_excludes       (DnfSack        *sack,
                                             DnfPackageSet  *pset);
void         dnf_sack_remove_module_excludes(DnfSack        *sack,
                                             DnfPackageSet  *pset);
void         dnf_sack_remove_includes       (DnfSack        *sack,
                                             DnfPackageSet  *pset);
void         dnf_sack_set_excludes          (DnfSack        *sack,
                                             DnfPackageSet  *pset);
void         dnf_sack_set_module_excludes   (DnfSack *sack,
                                             DnfPackageSet *pset);
void         dnf_sack_set_module_includes   (DnfSack *sack,
                                             DnfPackageSet *pset);
void         dnf_sack_set_includes          (DnfSack        *sack,
                                             DnfPackageSet  *pset);
void         dnf_sack_reset_excludes        (DnfSack        *sack);
void         dnf_sack_reset_module_excludes (DnfSack        *sack);
void         dnf_sack_reset_includes        (DnfSack        *sack);
DnfPackageSet *dnf_sack_get_includes        (DnfSack        *sack);
DnfPackageSet *dnf_sack_get_excludes        (DnfSack        *sack);
DnfPackageSet *dnf_sack_get_module_excludes (DnfSack        *sack);
DnfPackageSet *dnf_sack_get_module_includes (DnfSack        *sack);
gboolean     dnf_sack_set_use_includes      (DnfSack        *sack,
                                             const char     *reponame,
                                             gboolean        enabled);
gboolean     dnf_sack_get_use_includes      (DnfSack        *sack,
                                             const char     *reponame,
                                             gboolean       *enabled);
int          dnf_sack_repo_enabled          (DnfSack        *sack,
                                             const char     *reponame,
                                             int             enabled);
gboolean     dnf_sack_load_system_repo      (DnfSack        *sack,
                                             HyRepo          a_hrepo,
                                             int             flags,
                                             GError        **error);
gboolean     dnf_sack_load_repo             (DnfSack        *sack,
                                             HyRepo          hrepo,
                                             int             flags,
                                             GError        **error);
Pool        *dnf_sack_get_pool              (DnfSack    *sack);

void dnf_sack_filter_modules(DnfSack *sack, GPtrArray *repos, const char *install_root,
                             const char * platformModule);


/**********************************************************************/

/**
 * DnfSackAddFlags:
 * @DNF_SACK_ADD_FLAG_NONE:                     Add the primary
 * @DNF_SACK_ADD_FLAG_FILELISTS:                Add the filelists
 * @DNF_SACK_ADD_FLAG_UPDATEINFO:               Add the updateinfo
 * @DNF_SACK_ADD_FLAG_REMOTE:                   Use remote repos
 * @DNF_SACK_ADD_FLAG_UNAVAILABLE:              Add repos that are unavailable
 *
 * Flags to control repo loading into the sack.
 **/
typedef enum {
        DNF_SACK_ADD_FLAG_NONE                  = 0,
        DNF_SACK_ADD_FLAG_FILELISTS             = 1,
        DNF_SACK_ADD_FLAG_UPDATEINFO            = 2,
        DNF_SACK_ADD_FLAG_REMOTE                = 4,
        DNF_SACK_ADD_FLAG_UNAVAILABLE           = 8,
        /*< private >*/
        DNF_SACK_ADD_FLAG_LAST
} DnfSackAddFlags;

gboolean         dnf_sack_add_repo            (DnfSack        *sack,
                                                 DnfRepo      *repo,
                                                 guint           permissible_cache_age,
                                                 DnfSackAddFlags flags,
                                                 DnfState       *state,
                                                 GError         **error);
gboolean         dnf_sack_add_repos           (DnfSack        *sack,
                                                 GPtrArray      *repos,
                                                 guint           permissible_cache_age,
                                                 DnfSackAddFlags flags,
                                                 DnfState       *state,
                                                 GError         **error);

G_END_DECLS

#endif /* __DNF_SACK_H */
