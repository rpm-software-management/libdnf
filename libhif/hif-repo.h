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

#ifndef __HIF_REPO_H
#define __HIF_REPO_H

#include <glib-object.h>

#include "hy-repo.h"
#include "hy-package.h"

#include "hif-context.h"
#include "hif-state.h"

#include <librepo/librepo.h>

G_BEGIN_DECLS

#define HIF_TYPE_REPO (hif_repo_get_type ())
G_DECLARE_DERIVABLE_TYPE (HifRepo, hif_repo, HIF, REPO, GObject)

struct _HifRepoClass
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
 * HifRepoUpdateFlags:
 * @HIF_REPO_UPDATE_FLAG_NONE:                No flags set
 * @HIF_REPO_UPDATE_FLAG_FORCE:               Force the repo to be updated
 * @HIF_REPO_UPDATE_FLAG_IMPORT_PUBKEY:       Import the repo public key to librpm if possible
 * @HIF_REPO_UPDATE_FLAG_SIMULATE:            Do not actually update the repo
 *
 * The update flags.
 **/
typedef enum {
        HIF_REPO_UPDATE_FLAG_NONE             = 0,
        HIF_REPO_UPDATE_FLAG_FORCE            = 1,
        HIF_REPO_UPDATE_FLAG_IMPORT_PUBKEY    = 2,
        HIF_REPO_UPDATE_FLAG_SIMULATE         = 4,
        /*< private >*/
        HIF_REPO_UPDATE_FLAG_LAST
} HifRepoUpdateFlags;

/**
 * HifRepoKind:
 * @HIF_REPO_KIND_REMOTE:                     A remote repo
 * @HIF_REPO_KIND_MEDIA:                      A media repo, e.g. a DVD
 * @HIF_REPO_KIND_LOCAL:                      A local repo, e.g. file://
 *
 * The repo kind.
 **/
typedef enum {
        HIF_REPO_KIND_REMOTE,
        HIF_REPO_KIND_MEDIA,
        HIF_REPO_KIND_LOCAL,
        /*< private >*/
        HIF_REPO_KIND_LAST
} HifRepoKind;

/**
 * HifRepoEnabled:
 * @HIF_REPO_ENABLED_NONE:                    Source is disabled
 * @HIF_REPO_ENABLED_PACKAGES:                Source is fully enabled
 * @HIF_REPO_ENABLED_METADATA:                Only repo metadata is enabled
 *
 * How enabled is the repo.
 **/
typedef enum {
        HIF_REPO_ENABLED_NONE                 = 0,
        HIF_REPO_ENABLED_PACKAGES             = 1,
        HIF_REPO_ENABLED_METADATA             = 2,
        /*< private >*/
        HIF_REPO_ENABLED_LAST
} HifRepoEnabled;

HifRepo         *hif_repo_new                   (HifContext           *context);

/* getters */
const gchar     *hif_repo_get_id                (HifRepo              *repo);
const gchar     *hif_repo_get_location          (HifRepo              *repo);
const gchar     *hif_repo_get_filename          (HifRepo              *repo);
const gchar     *hif_repo_get_packages          (HifRepo              *repo);
HifRepoEnabled   hif_repo_get_enabled           (HifRepo              *repo);
gboolean         hif_repo_get_required          (HifRepo              *repo);
guint            hif_repo_get_cost              (HifRepo              *repo);
HifRepoKind      hif_repo_get_kind              (HifRepo              *repo);
gchar          **hif_repo_get_exclude_packages  (HifRepo              *repo);
gboolean         hif_repo_get_gpgcheck          (HifRepo              *repo);
gboolean         hif_repo_get_gpgcheck_md       (HifRepo              *repo);
gchar           *hif_repo_get_description       (HifRepo              *repo);
const gchar     *hif_repo_get_filename_md       (HifRepo              *repo,
                                                 const gchar          *md_kind);
#ifndef __GI_SCANNER__
HyRepo           hif_repo_get_repo              (HifRepo              *repo);
#endif
gboolean         hif_repo_is_devel              (HifRepo              *repo);
gboolean         hif_repo_is_local              (HifRepo              *repo);
gboolean         hif_repo_is_repo             (HifRepo              *repo);

/* setters */
void             hif_repo_set_id                (HifRepo              *repo,
                                                 const gchar          *id);
void             hif_repo_set_location          (HifRepo              *repo,
                                                 const gchar          *location);
void             hif_repo_set_location_tmp      (HifRepo              *repo,
                                                 const gchar          *location_tmp);
void             hif_repo_set_filename          (HifRepo              *repo,
                                                 const gchar          *filename);
void             hif_repo_set_packages          (HifRepo              *repo,
                                                 const gchar          *packages);
void             hif_repo_set_packages_tmp      (HifRepo              *repo,
                                                 const gchar          *packages_tmp);
void             hif_repo_set_enabled           (HifRepo              *repo,
                                                 HifRepoEnabled        enabled);
void             hif_repo_set_required          (HifRepo              *repo,
                                                 gboolean              required);
void             hif_repo_set_cost              (HifRepo              *repo,
                                                 guint                 cost);
void             hif_repo_set_kind              (HifRepo              *repo,
                                                 HifRepoKind           kind);
void             hif_repo_set_gpgcheck          (HifRepo              *repo,
                                                 gboolean              gpgcheck_pkgs);
void             hif_repo_set_gpgcheck_md       (HifRepo              *repo,
                                                 gboolean              gpgcheck_md);
void             hif_repo_set_keyfile           (HifRepo              *repo,
                                                 GKeyFile             *keyfile);
gboolean         hif_repo_setup                 (HifRepo              *repo,
                                                 GError              **error);

/* object methods */
gboolean         hif_repo_check                 (HifRepo              *repo,
                                                 guint                 permissible_cache_age,
                                                 HifState             *state,
                                                 GError              **error);
gboolean         hif_repo_update                (HifRepo              *repo,
                                                 HifRepoUpdateFlags    flags,
                                                 HifState             *state,
                                                 GError              **error);
gboolean         hif_repo_clean                 (HifRepo              *repo,
                                                 GError              **error);
gboolean         hif_repo_set_data              (HifRepo              *repo,
                                                 const gchar          *parameter,
                                                 const gchar          *value,
                                                 GError              **error);
gboolean         hif_repo_commit                (HifRepo              *repo,
                                                 GError              **error);

LrHandle *       hif_repo_get_lr_handle         (HifRepo              *repo);

LrResult *       hif_repo_get_lr_result         (HifRepo              *repo);

#ifndef __GI_SCANNER__
gchar           *hif_repo_download_package      (HifRepo              *repo,
                                                 HifPackage *            pkg,
                                                 const gchar          *directory,
                                                 HifState             *state,
                                                 GError              **error);
#endif

G_END_DECLS

#endif /* __HIF_REPO_H */
