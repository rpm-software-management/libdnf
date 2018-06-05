/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2014-2015 Richard Hughes <richard@hughsie.com>
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

#ifndef __DNF_TRANSACTION_H
#define __DNF_TRANSACTION_H

#include <glib-object.h>
#include <gio/gio.h>

#include "hy-goal.h"
#include "dnf-types.h"
#include "dnf-context.h"
#include "dnf-state.h"

G_BEGIN_DECLS

#define DNF_TYPE_TRANSACTION (dnf_transaction_get_type ())
G_DECLARE_DERIVABLE_TYPE (DnfTransaction, dnf_transaction, DNF, TRANSACTION, GObject)

struct _DnfTransactionClass
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
 * DnfTransactionFlag:
 * @DNF_TRANSACTION_FLAG_NONE:                  No flags
 * @DNF_TRANSACTION_FLAG_ONLY_TRUSTED:          Only install trusted packages
 * @DNF_TRANSACTION_FLAG_ALLOW_REINSTALL:       Allow package reinstallation
 * @DNF_TRANSACTION_FLAG_ALLOW_DOWNGRADE:       Allow package downrades
 * @DNF_TRANSACTION_FLAG_NODOCS:                Don't install documentation
 * @DNF_TRANSACTION_FLAG_TEST:                  Only do a transaction test
 *
 * The transaction flags.
 **/
typedef enum {
        DNF_TRANSACTION_FLAG_NONE               = 0,
        DNF_TRANSACTION_FLAG_ONLY_TRUSTED       = 1 << 0,
        DNF_TRANSACTION_FLAG_ALLOW_REINSTALL    = 1 << 1,
        DNF_TRANSACTION_FLAG_ALLOW_DOWNGRADE    = 1 << 2,
        DNF_TRANSACTION_FLAG_NODOCS             = 1 << 3,
        DNF_TRANSACTION_FLAG_TEST               = 1 << 4,
        /*< private >*/
        DNF_TRANSACTION_FLAG_LAST
} DnfTransactionFlag;

DnfTransaction  *dnf_transaction_new                    (DnfContext     *context);

/* getters */
guint64          dnf_transaction_get_flags              (DnfTransaction *transaction);
GPtrArray       *dnf_transaction_get_remote_pkgs        (DnfTransaction *transaction);
DnfDb           *dnf_transaction_get_db                 (DnfTransaction *transaction);

/* setters */
void             dnf_transaction_set_repos            (DnfTransaction *transaction,
                                                         GPtrArray      *repos);
void             dnf_transaction_set_uid                (DnfTransaction *transaction,
                                                         guint           uid);
void             dnf_transaction_set_flags              (DnfTransaction *transaction,
                                                         guint64         flags);

/* object methods */
gboolean         dnf_transaction_depsolve               (DnfTransaction *transaction,
                                                         HyGoal          goal,
                                                         DnfState       *state,
                                                         GError         **error);
gboolean         dnf_transaction_download               (DnfTransaction *transaction,
                                                         DnfState       *state,
                                                         GError         **error);

gboolean         dnf_transaction_import_keys            (DnfTransaction *transaction,
                                                         GError        **error);

gboolean         dnf_transaction_gpgcheck_package       (DnfTransaction *transaction,
                                                         DnfPackage     *pkg,
                                                         GError        **error);
gboolean         dnf_transaction_check_untrusted        (DnfTransaction *transaction,
                                                         HyGoal          goal,
                                                         GError        **error);

gboolean         dnf_transaction_commit                 (DnfTransaction *transaction,
                                                         HyGoal          goal,
                                                         DnfState       *state,
                                                         GError         **error);
gboolean         dnf_transaction_ensure_repo          (DnfTransaction *transaction,
                                                         DnfPackage *      pkg,
                                                         GError         **error);
gboolean         dnf_transaction_ensure_repo_list     (DnfTransaction *transaction,
                                                         GPtrArray *  pkglist,
                                                         GError         **error);

G_END_DECLS

#endif /* __DNF_TRANSACTION_H */
