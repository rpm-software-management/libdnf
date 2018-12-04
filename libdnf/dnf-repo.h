/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2013-2015 Richard Hughes <richard@hughsie.com>
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

#ifndef __DNF_REPO_H
#define __DNF_REPO_H

#include <glib-object.h>

#include "hy-repo.h"
#include "hy-package.h"

#include "dnf-context.h"
#include "dnf-state.h"

#include <librepo/librepo.h>

G_BEGIN_DECLS

#define DNF_TYPE_REPO (dnf_repo_get_type ())
G_DECLARE_DERIVABLE_TYPE (DnfRepo, dnf_repo, DNF, REPO, GObject)

struct _DnfRepoClass
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
 * DnfRepoUpdateFlags:
 * @DNF_REPO_UPDATE_FLAG_NONE:                No flags set
 * @DNF_REPO_UPDATE_FLAG_FORCE:               Force the repo to be updated
 * @DNF_REPO_UPDATE_FLAG_IMPORT_PUBKEY:       Import the repo public key to librpm if possible
 * @DNF_REPO_UPDATE_FLAG_SIMULATE:            Do not actually update the repo
 *
 * The update flags.
 **/
typedef enum {
        DNF_REPO_UPDATE_FLAG_NONE             = 0,
        DNF_REPO_UPDATE_FLAG_FORCE            = 1,
        DNF_REPO_UPDATE_FLAG_IMPORT_PUBKEY    = 2,
        DNF_REPO_UPDATE_FLAG_SIMULATE         = 4,
        /*< private >*/
        DNF_REPO_UPDATE_FLAG_LAST
} DnfRepoUpdateFlags;

/**
 * DnfRepoKind:
 * @DNF_REPO_KIND_REMOTE:                     A remote repo
 * @DNF_REPO_KIND_MEDIA:                      A media repo, e.g. a DVD
 * @DNF_REPO_KIND_LOCAL:                      A local repo, e.g. file://
 *
 * The repo kind.
 **/
typedef enum {
        DNF_REPO_KIND_REMOTE,
        DNF_REPO_KIND_MEDIA,
        DNF_REPO_KIND_LOCAL,
        /*< private >*/
        DNF_REPO_KIND_LAST
} DnfRepoKind;

/**
 * DnfRepoEnabled:
 * @DNF_REPO_ENABLED_NONE:                    Source is disabled
 * @DNF_REPO_ENABLED_PACKAGES:                Source is fully enabled
 * @DNF_REPO_ENABLED_METADATA:                Only repo metadata is enabled
 *
 * How enabled is the repo.
 **/
typedef enum {
        DNF_REPO_ENABLED_NONE                 = 0,
        DNF_REPO_ENABLED_PACKAGES             = 1,
        DNF_REPO_ENABLED_METADATA             = 2,
        /*< private >*/
        DNF_REPO_ENABLED_LAST
} DnfRepoEnabled;

DnfRepo         *dnf_repo_new                   (DnfContext           *context);

/* getters */
const gchar     *dnf_repo_get_id                (DnfRepo              *repo);
const gchar     *dnf_repo_get_location          (DnfRepo              *repo);
const gchar     *dnf_repo_get_filename          (DnfRepo              *repo);
const gchar     *dnf_repo_get_packages          (DnfRepo              *repo);
gchar **         dnf_repo_get_public_keys       (DnfRepo              *repo);
DnfRepoEnabled   dnf_repo_get_enabled           (DnfRepo              *repo);
gboolean         dnf_repo_get_required          (DnfRepo              *repo);
guint            dnf_repo_get_cost              (DnfRepo              *repo);
gboolean         dnf_repo_get_module_hotfixes   (DnfRepo              *repo);
guint            dnf_repo_get_metadata_expire   (DnfRepo              *repo);
DnfRepoKind      dnf_repo_get_kind              (DnfRepo              *repo);
gchar          **dnf_repo_get_exclude_packages  (DnfRepo              *repo);
gboolean         dnf_repo_get_gpgcheck          (DnfRepo              *repo);
gboolean         dnf_repo_get_gpgcheck_md       (DnfRepo              *repo);
gchar           *dnf_repo_get_description       (DnfRepo              *repo);
guint64          dnf_repo_get_timestamp_generated(DnfRepo              *repo);
guint            dnf_repo_get_n_solvables       (DnfRepo              *repo);
const gchar     *dnf_repo_get_filename_md       (DnfRepo              *repo,
                                                 const gchar          *md_kind);
#ifndef __GI_SCANNER__
HyRepo           dnf_repo_get_repo              (DnfRepo              *repo);
#endif
gboolean         dnf_repo_is_devel              (DnfRepo              *repo);
gboolean         dnf_repo_is_local              (DnfRepo              *repo);
gboolean         dnf_repo_is_source             (DnfRepo              *repo);

/* setters */
void             dnf_repo_set_id                (DnfRepo              *repo,
                                                 const gchar          *id);
void             dnf_repo_set_location          (DnfRepo              *repo,
                                                 const gchar          *location);
void             dnf_repo_set_location_tmp      (DnfRepo              *repo,
                                                 const gchar          *location_tmp);
void             dnf_repo_set_filename          (DnfRepo              *repo,
                                                 const gchar          *filename);
void             dnf_repo_set_packages          (DnfRepo              *repo,
                                                 const gchar          *packages);
void             dnf_repo_set_packages_tmp      (DnfRepo              *repo,
                                                 const gchar          *packages_tmp);
void             dnf_repo_set_enabled           (DnfRepo              *repo,
                                                 DnfRepoEnabled        enabled);
void             dnf_repo_set_required          (DnfRepo              *repo,
                                                 gboolean              required);
void             dnf_repo_set_cost              (DnfRepo              *repo,
                                                 guint                 cost);
void             dnf_repo_set_module_hotfixes   (DnfRepo              *repo,
                                                 gboolean              module_hotfixes);
void             dnf_repo_set_kind              (DnfRepo              *repo,
                                                 DnfRepoKind           kind);
void             dnf_repo_set_gpgcheck          (DnfRepo              *repo,
                                                 gboolean              gpgcheck_pkgs);
void             dnf_repo_set_gpgcheck_md       (DnfRepo              *repo,
                                                 gboolean              gpgcheck_md);
void             dnf_repo_set_keyfile           (DnfRepo              *repo,
                                                 GKeyFile             *keyfile);
void             dnf_repo_set_metadata_expire   (DnfRepo              *repo,
                                                 guint                 metadata_expire);
gboolean         dnf_repo_setup                 (DnfRepo              *repo,
                                                 GError              **error);

/* object methods */
gboolean         dnf_repo_check                 (DnfRepo              *repo,
                                                 guint                 permissible_cache_age,
                                                 DnfState             *state,
                                                 GError              **error);
gboolean         dnf_repo_update                (DnfRepo              *repo,
                                                 DnfRepoUpdateFlags    flags,
                                                 DnfState             *state,
                                                 GError              **error);
gboolean         dnf_repo_clean                 (DnfRepo              *repo,
                                                 GError              **error);
gboolean         dnf_repo_set_data              (DnfRepo              *repo,
                                                 const gchar          *parameter,
                                                 const gchar          *value,
                                                 GError              **error);
gboolean         dnf_repo_commit                (DnfRepo              *repo,
                                                 GError              **error);

#ifndef __GI_SCANNER__
LrHandle *       dnf_repo_get_lr_handle         (DnfRepo              *repo);

LrResult *       dnf_repo_get_lr_result         (DnfRepo              *repo);
#endif

#ifndef __GI_SCANNER__
gchar           *dnf_repo_download_package      (DnfRepo              *repo,
                                                 DnfPackage *            pkg,
                                                 const gchar          *directory,
                                                 DnfState             *state,
                                                 GError              **error);
gboolean         dnf_repo_download_packages     (DnfRepo              *repo,
                                                 GPtrArray            *pkgs,
                                                 const gchar          *directory,
                                                 DnfState             *state,
                                                 GError              **error);

HyRepo dnf_repo_get_hy_repo(DnfRepo *repo);
#endif

void   dnf_repo_add_metadata_type_to_download   (DnfRepo              *repo,
                                                 const gchar          *metadataType);
gboolean         dnf_repo_get_metadata_content  (DnfRepo              *repo,
                                                 const gchar          *metadataType,
                                                 gpointer             *content,
                                                 gsize                *length,
                                                 GError              **error);

G_END_DECLS

#endif /* __DNF_REPO_H */
