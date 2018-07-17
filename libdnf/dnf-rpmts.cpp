/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2013-2015 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * Most of this code was taken from Zif, libzif/zif-transaction.c
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or(at your option) any later version.
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

/**
 * SECTION:dnf-goal
 * @short_description: Helper methods for dealing with rpm transactions.
 * @include: libdnf.h
 * @stability: Unstable
 *
 * These methods make it easier to deal with rpm transactions.
 */


#include <glib.h>
#include <rpm/rpmlib.h>
#include <rpm/rpmlog.h>
#include <rpm/rpmdb.h>

#include "dnf-rpmts.h"
#include "dnf-types.h"
#include "dnf-utils.h"

#include "utils/bgettext/bgettext-lib.h"

/**
 * dnf_rpmts_add_install_filename:
 * @ts: a #rpmts instance.
 * @filename: the package.
 * @allow_untrusted: is we can add untrusted packages.
 * @is_update: if the package is an update.
 * @error: a #GError or %NULL..
 *
 * Add to the transaction a package to be installed.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_rpmts_add_install_filename(rpmts ts,
                               const gchar *filename,
                               gboolean allow_untrusted,
                               gboolean is_update,
                               GError **error)
{
    gboolean ret = TRUE;
    gint res;
    Header hdr;
    FD_t fd;

    /* open this */
    fd = Fopen(filename, "r.ufdio");
    res = rpmReadPackageFile(ts, fd, filename, &hdr);

    /* be less strict when we're allowing untrusted transactions */
    if (allow_untrusted) {
        switch(res) {
        case RPMRC_NOKEY:
        case RPMRC_NOTFOUND:
        case RPMRC_NOTTRUSTED:
        case RPMRC_OK:
            break;
        case RPMRC_FAIL:
            ret = FALSE;
            g_set_error(error,
                        DNF_ERROR,
                        DNF_ERROR_INTERNAL_ERROR,
                        _("signature does not verify for %s"),
                        filename);
            goto out;
        default:
            ret = FALSE;
            g_set_error(error,
                        DNF_ERROR,
                        DNF_ERROR_INTERNAL_ERROR,
                        _("failed to open(generic error): %s"),
                        filename);
            goto out;
        }
    } else {
        switch(res) {
        case RPMRC_OK:
            break;
        case RPMRC_NOTTRUSTED:
            ret = FALSE;
            g_set_error(error,
                        DNF_ERROR,
                        DNF_ERROR_INTERNAL_ERROR,
                        _("failed to verify key for %s"),
                        filename);
            goto out;
        case RPMRC_NOKEY:
            ret = FALSE;
            g_set_error(error,
                        DNF_ERROR,
                        DNF_ERROR_INTERNAL_ERROR,
                        _("public key unavailable for %s"),
                        filename);
            goto out;
        case RPMRC_NOTFOUND:
            ret = FALSE;
            g_set_error(error,
                        DNF_ERROR,
                        DNF_ERROR_INTERNAL_ERROR,
                        _("signature not found for %s"),
                        filename);
            goto out;
        case RPMRC_FAIL:
            ret = FALSE;
            g_set_error(error,
                        DNF_ERROR,
                        DNF_ERROR_INTERNAL_ERROR,
                        _("signature does not verify for %s"),
                        filename);
            goto out;
        default:
            ret = FALSE;
            g_set_error(error,
                        DNF_ERROR,
                        DNF_ERROR_INTERNAL_ERROR,
                        _("failed to open(generic error): %s"),
                        filename);
            goto out;
        }
    }

    /* add to the transaction */
    res = rpmtsAddInstallElement(ts, hdr, (fnpyKey) filename, is_update, NULL);
    if (res != 0) {
        ret = FALSE;
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    _("failed to add install element: %1$s [%2$i]"),
                    filename, res);
        goto out;
    }
out:
    Fclose(fd);
    headerFree(hdr);
    return ret;
}

/**
 * dnf_rpmts_look_for_problems:
 * @ts: a #rpmts instance.
 * @error: a #GError or %NULL..
 *
 * Look for problems in the transaction.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_rpmts_look_for_problems(rpmts ts, GError **error)
{
    gboolean ret = TRUE;
    rpmProblem prob;
    rpmpsi psi;
    rpmps probs = NULL;
    g_autoptr(GString) string = NULL;

    /* get a list of problems */
    probs = rpmtsProblems(ts);
    if (rpmpsNumProblems(probs) == 0)
        goto out;

    /* parse problems */
    string = g_string_new("");
    psi = rpmpsInitIterator(probs);
    while (rpmpsNextIterator(psi) >= 0) {
        g_autofree gchar *msg = NULL;
        prob = rpmpsGetProblem(psi);
        msg = rpmProblemString(prob);
        g_string_append(string, msg);
        g_string_append(string, "\n");
    }
    rpmpsFreeIterator(psi);

    /* set error */
    ret = FALSE;

    /* we failed, and got a reason to report */
    if (string->len > 0) {
        g_string_set_size(string, string->len - 1);
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    _("Error running transaction: %s"),
                    string->str);
        goto out;
    }

    /* we failed, and got no reason why */
    g_set_error_literal(error,
                        DNF_ERROR,
                        DNF_ERROR_INTERNAL_ERROR,
                        _("Error running transaction and no problems were reported!"));
out:
    rpmpsFree(probs);
    return ret;
}

/**
 * dnf_rpmts_log_handler_cb:
 **/
static int
dnf_rpmts_log_handler_cb(rpmlogRec rec, rpmlogCallbackData data)
{
    GString **string =(GString **) data;

    /* only log errors */
    if (rpmlogRecPriority(rec) != RPMLOG_ERR)
        return RPMLOG_DEFAULT;

    /* do not log internal BDB errors */
    if (g_strstr_len(rpmlogRecMessage(rec), -1, "BDB") != NULL)
        return 0;

    /* create string if required */
    if (*string == NULL)
        *string = g_string_new("");

    /* if text already exists, join them */
    if ((*string)->len > 0)
        g_string_append(*string, ": ");
    g_string_append(*string, rpmlogRecMessage(rec));

    /* remove the trailing /n which rpm does */
    if ((*string)->len > 0)
        g_string_truncate(*string,(*string)->len - 1);
    return 0;
}

/**
 * dnf_rpmts_find_package:
 **/
static Header
dnf_rpmts_find_package(rpmts ts, DnfPackage *pkg, GError **error)
{
    Header hdr = NULL;
    rpmdbMatchIterator iter;
    unsigned int recOffset;
    g_autoptr(GString) rpm_error = NULL;

    /* find package by db-id */
    recOffset = dnf_package_get_rpmdbid(pkg);
    rpmlogSetCallback(dnf_rpmts_log_handler_cb, &rpm_error);
    iter = rpmtsInitIterator(ts, RPMDBI_PACKAGES,
                 &recOffset, sizeof(recOffset));
    if (iter == NULL) {
        if (rpm_error != NULL) {
            g_set_error_literal(error,
                                DNF_ERROR,
                                DNF_ERROR_UNFINISHED_TRANSACTION,
                                rpm_error->str);
        } else {
            g_set_error_literal(error,
                                DNF_ERROR,
                                DNF_ERROR_UNFINISHED_TRANSACTION,
                                _("Fatal error, run database recovery"));
        }
        goto out;
    }
    hdr = rpmdbNextIterator(iter);
    if (hdr == NULL) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_FILE_NOT_FOUND,
                    _("failed to find package %s"),
                    dnf_package_get_name(pkg));
        goto out;
    }

    /* success */
    headerLink(hdr);
out:
    rpmlogSetCallback(NULL, NULL);
    if (iter != NULL)
        rpmdbFreeIterator(iter);
    return hdr;
}

/**
 * dnf_rpmts_add_remove_pkg:
 * @ts: a #rpmts instance.
 * @pkg: a #DnfPackage *instance.
 * @error: a #GError or %NULL..
 *
 * Adds to the transaction a package to be removed.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_rpmts_add_remove_pkg(rpmts ts, DnfPackage *pkg, GError **error)
{
    gboolean ret = TRUE;
    gint retval;
    Header hdr;

    hdr = dnf_rpmts_find_package(ts, pkg, error);
    if (hdr == NULL) {
        ret = FALSE;
        goto out;
    }

    /* remove it */
    retval = rpmtsAddEraseElement(ts, hdr, -1);
    if (retval != 0) {
        ret = FALSE;
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    _("could not add erase element %1$s(%2$i)"),
                    dnf_package_get_name(pkg), retval);
        goto out;
    }
out:
    if (hdr != NULL)
        headerFree(hdr);
    return ret;
}
