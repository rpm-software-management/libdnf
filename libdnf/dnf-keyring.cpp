/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2013-2015 Richard Hughes <richard@hughsie.com>
 *
 * Most of this code was taken from Zif, libzif/zif-transaction.c
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
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
 * License along with this library; if not, see
 * <https://www.gnu.org/licenses/>.
 */
/**
 * SECTION:dnf-keyring
 * @short_description: Helper methods for dealing with rpm keyrings.
 * @include: libdnf.h
 * @stability: Unstable
 *
 * These methods make it easier to deal with rpm keyrings.
 */


#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <rpm/rpmlib.h>
#include <rpm/rpmts.h>
#include <rpm/rpmlog.h>
#include <rpm/rpmcli.h>

#include "catch-error.hpp"
#include "dnf-types.h"
#include "dnf-keyring.h"
#include "dnf-utils.h"

/* Return a key ID as a hexadecimal string.
 * @key: a public key
 * Returns: A pointer to be freed, NULL on error. */
static char *formatkeyid(rpmPubkey key) {
    char *string = NULL;
#ifdef RPM_HAS_KEYIDASHEX
    string = strdup(rpmPubkeyKeyIDAsHex(key));
#else
    /* A fallback implementation for rpmPubkeyKeyIDAsHex() which is available
     * since RPM 6. */
    static const char table[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
    pgpDigParams parameters = NULL; /* weak pointer */
    const uint8_t *keyid = NULL;    /* weak pointer */

    if (!key)
        return NULL;
    string = (char*)malloc(PGP_KEYID_LEN*2+1);
    if (!string)
        return NULL;
    parameters = rpmPubkeyPgpDigParams(key);
    if (!parameters) {
        free(string);
        return NULL;
    }
    keyid = pgpDigParamsSignID(parameters);
    for (int i = 0; i < PGP_KEYID_LEN; i++) {
        string[i*2] = table[keyid[i] >> 4];
        string[i*2 + 1] = table[keyid[i] & 0x0f];
    }
    string[PGP_KEYID_LEN*2] = '\0';
#endif
    return string;
}

/**
 * dnf_keyring_add_public_key_from_memory:
 * @keyring: a #rpmKeyring instance.
 * @filename: a public key filename.
 * @pkt: a memory block with dearmored single OpenPGP public key packet
 * @len: a length of the memory block
 * @error: a #GError or %NULL.
 *
 * Adds a specific public key to the keyring.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 **/
static gboolean
dnf_keyring_add_public_key_from_memory(rpmKeyring keyring,
                                       const gchar *filename,
                                       const uint8_t *pkt,
                                       size_t len,
                                       GError **error) try
{
    gboolean ret = TRUE;
    int rc;
    rpmPubkey pubkey = NULL;
    rpmPubkey *subkeys = NULL;
    int nsubkeys = 0;
    char *keyid = NULL;

    if (pkt == NULL || len == 0) {
        ret = FALSE;
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    "empty memory block passed to dnf_keyring_add_public_key_from_memory()");
        goto out;
    }

    /* Parse the public key */
    pubkey = rpmPubkeyNew(pkt, len);
    if (pubkey == NULL) {
        ret = FALSE;
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_GPG_SIGNATURE_INVALID,
                    "failed to parse public key for %s",
                    filename);
        goto out;
    }
    keyid = formatkeyid(pubkey);

    /* add to in-memory keyring */
    rc = rpmKeyringAddKey(keyring, pubkey);
    if (rc == 1) {
        ret = TRUE;
        if (keyid == NULL)
            g_debug("a key from %s is already added", filename);
        else
            g_debug("0x%s key from %s is already added", keyid, filename);
        goto out;
    } else if (rc < 0) {
        ret = FALSE;
        if (keyid == NULL)
            g_set_error(error,
                        DNF_ERROR,
                        DNF_ERROR_GPG_SIGNATURE_INVALID,
                        "failed to add a public key from %s to rpmdb",
                        filename);
        else
            g_set_error(error,
                        DNF_ERROR,
                        DNF_ERROR_GPG_SIGNATURE_INVALID,
                        "failed to add 0x%s public key from %s to rpmdb",
                        keyid,
                        filename);
        goto out;
    }
    if (keyid == NULL)
        g_debug("added missing public key from %s to rpmdb", filename);
    else
        g_debug("added missing 0x%s public key from %s to rpmdb", keyid, filename);

#ifndef RPM_AUTOADDS_SUBKEYS
    /* RPM before 5.99.90 required adding subkeys explicitly.
     * RPM >= 5.99.90 processes subkeys automatically with a primary key and
     * fails on processing standalone subkeys in rpmKeyringAddKey(). */
    subkeys = rpmGetSubkeys(pubkey, &nsubkeys);
    for (int i = 0; i < nsubkeys; i++) {
        rpmPubkey subkey = subkeys[i];
        if (rpmKeyringAddKey(keyring, subkey) < 0) {
            char *subkeyid = formatkeyid(subkey);
            ret = FALSE;
            if (keyid == NULL)
                if (subkey == NULL)
                    g_set_error(error,
                                DNF_ERROR,
                                DNF_ERROR_GPG_SIGNATURE_INVALID,
                                "failed to add a subkey from %s to rpmdb",
                                filename);
                else
                    g_set_error(error,
                                DNF_ERROR,
                                DNF_ERROR_GPG_SIGNATURE_INVALID,
                                "failed to add 0x%s subkey from %s to rpmdb",
                                subkeyid,
                                keyid,
                                filename);
            else
                if (subkeyid == NULL)
                    g_set_error(error,
                                DNF_ERROR,
                                DNF_ERROR_GPG_SIGNATURE_INVALID,
                                "failed to add a subkey for 0x%s primary key from %s to rpmdb",
                                subkeyid,
                                keyid,
                                filename);
                else
                    g_set_error(error,
                                DNF_ERROR,
                                DNF_ERROR_GPG_SIGNATURE_INVALID,
                                "failed to add 0x%s subkey for 0x%s primary key from %s to rpmdb",
                                subkeyid,
                                keyid,
                                filename);
            if (subkeyid != NULL)
                free(subkeyid);
            goto out;
        }
    }
#endif
out:
    if (keyid != NULL)
        free(keyid);
    if (pubkey != NULL)
        rpmPubkeyFree(pubkey);
    if (subkeys != NULL) {
        for (int i = 0; i < nsubkeys; i++) {
          rpmPubkeyFree(subkeys[i]);
        }
        free(subkeys);
    }
    return ret;
} CATCH_TO_GERROR(FALSE)

/**
 * dnf_keyring_add_public_key:
 * @keyring: a #rpmKeyring instance.
 * @filename: The public key filename.
 * @error: a #GError or %NULL.
 *
 * Adds a specific public key to the keyring.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_keyring_add_public_key(rpmKeyring keyring,
                           const gchar *filename,
                           GError **error) try
{
    gboolean ret = TRUE;
    bool importable_certificates_found = FALSE;
    uint8_t *pkt = NULL;
    gsize len;
    g_autofree gchar *data = NULL;

    /* ignore symlinks and directories */
    if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR))
        goto out;
    if (g_file_test(filename, G_FILE_TEST_IS_SYMLINK))
        goto out;

    /* get data */
    ret = g_file_get_contents(filename, &data, &len, error);
    if (!ret)
        goto out;

    /* Iterate over multiple ASCII-armored blocks.
     * There is no function for it in the RPM library yet. */
    for (
            const gchar *block = data;
            NULL != (block = strstr(block, "-----BEGIN PGP PUBLIC KEY BLOCK-----"));
            free(pkt), pkt = NULL, block++) {
        pgpArmor armor;

        /* rip off the ASCII armor and parse it */
        armor = pgpParsePkts(block, &pkt, &len);
        if (armor < 0) {
            ret = FALSE;
            if (error && !*error) {
                g_set_error(error,
                            DNF_ERROR,
                            DNF_ERROR_GPG_SIGNATURE_INVALID,
                            "failed to parse PKI file %s",
                            filename);
            }
            continue;
        }

        /* make sure it's something we can add to rpm */
        if (armor != PGPARMOR_PUBKEY) {
            ret = FALSE;
            if (error && !*error) {
                g_set_error(error,
                            DNF_ERROR,
                            DNF_ERROR_GPG_SIGNATURE_INVALID,
                            "PKI file %s is not a public key",
                            filename);
            }
            continue;
        }

        {
            /* Iterate over all public keys in this dearmored block */
            uint8_t *tpkt = pkt;
            size_t cert_len;
            while (len > 0) {
                    if (pgpPubKeyCertLen(tpkt, len, &cert_len))
                        break;
                    if (cert_len > len)
                        break;

                    if (!dnf_keyring_add_public_key_from_memory(keyring, filename, tpkt, cert_len,
                                /* Remember first error message */
                                error == NULL || *error != NULL ? NULL : error))
                        ret = FALSE;

                    tpkt += cert_len;
                    len -= cert_len;
                    importable_certificates_found = TRUE;
            }
        }
    }

    if (!ret)
        /* Prevent overwriting error messages */
        goto out;

    if (!importable_certificates_found) {
        ret = FALSE;
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_GPG_SIGNATURE_INVALID,
                    "PKI file %s contains no valid public key",
                    filename);
        goto out;
    }

    if (ret) {
        /* success */
        g_debug("added missing public key %s to rpmdb", filename);
    }
out:
    if (pkt != NULL)
        free(pkt); /* yes, free() */
    return ret;
} CATCH_TO_GERROR(FALSE)

/**
 * dnf_keyring_add_public_keys:
 * @keyring: a #rpmKeyring instance.
 * @error: a #GError or %NULL.
 *
 * Adds all installed public keys to the RPM and shared keyring.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_keyring_add_public_keys(rpmKeyring keyring, GError **error) try
{
    const gchar *gpg_dir = "/etc/pki/rpm-gpg";
    gboolean ret = TRUE;
    g_autoptr(GDir) dir = NULL;
    GError *localError = NULL;

    /* search all the public key files */
    dir = g_dir_open(gpg_dir, 0, &localError);
    if (dir == NULL) {
        if (localError->domain != G_FILE_ERROR || localError->code != G_FILE_ERROR_NOENT) {
            g_warning("%s", localError->message);
        }
        g_error_free(localError);
        return TRUE;
    }
    do {
        const gchar *filename;
        g_autofree gchar *path_tmp = NULL;
        filename = g_dir_read_name(dir);
        if (filename == NULL)
            break;
        path_tmp = g_build_filename(gpg_dir, filename, NULL);
        ret = dnf_keyring_add_public_key(keyring, path_tmp, &localError);
        if (!ret) {
            g_warning("%s", localError->message);
            g_error_free(localError);
            localError = NULL;
        }
    } while (true);
    return TRUE;
} CATCH_TO_GERROR(FALSE)

static int
rpmcliverifysignatures_log_handler_cb(rpmlogRec rec, rpmlogCallbackData data)
{
    GString **string =(GString **) data;

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
 * dnf_keyring_check_untrusted_file:
 */
gboolean
dnf_keyring_check_untrusted_file(rpmKeyring keyring,
                                 const gchar *filename,
                                 GError **error) try
{
    FD_t fd = NULL;
    gboolean ret = FALSE;
    Header hdr = NULL;
    rpmRC rc;
    rpmts ts = NULL;

    char *path = g_strdup(filename);
    char *path_array[2] = {path, NULL};
    g_autoptr(GString) rpm_error = NULL;

    /* open the file for reading */
    fd = Fopen(filename, "r.fdio");
    if (fd == NULL) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_FILE_INVALID,
                    "failed to open %s",
                    filename);
        goto out;
    }
    if (Ferror(fd)) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_FILE_INVALID,
                    "failed to open %s: %s",
                    filename,
                    Fstrerror(fd));
        goto out;
    }

    ts = rpmtsCreate();

    if (rpmtsSetKeyring(ts, keyring) < 0) {
        g_set_error_literal(error, DNF_ERROR, DNF_ERROR_INTERNAL_ERROR, "failed to set keyring");
        goto out;
    }
    rpmtsSetVfyLevel(ts, RPMSIG_SIGNATURE_TYPE);
    rpmlogSetCallback(rpmcliverifysignatures_log_handler_cb, &rpm_error);

    // rpm doesn't provide any better API call than rpmcliVerifySignatures (which is for CLI):
    // - use path_array as input argument
    // - gather logs via callback because we don't want to print anything if check is successful
    if (rpmcliVerifySignatures(ts, (char * const*) path_array)) {
        g_set_error(error,
                DNF_ERROR,
                DNF_ERROR_GPG_SIGNATURE_INVALID,
                "%s could not be verified.\n%s",
                filename,
                (rpm_error ? rpm_error->str : "UNKNOWN ERROR"));
        goto out;
    }

    /* read in the file */
    rc = rpmReadPackageFile(ts, fd, filename, &hdr);
    if (rc != RPMRC_OK) {
        /* we only return SHA1 and MD5 failures, as we're not
         * checking signatures at this stage */
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_FILE_INVALID,
                    "%s could not be verified",
                    filename);
        goto out;
    }

    /* the package is signed by a key we trust */
    g_debug("%s has been verified as trusted", filename);
    ret = TRUE;
out:
    rpmlogSetCallback(NULL, NULL);

    if (path != NULL)
        g_free(path);
    if (ts != NULL)
        rpmtsFree(ts);
    if (hdr != NULL)
        headerFree(hdr);
    if (fd != NULL)
        Fclose(fd);
    return ret;
} CATCH_TO_GERROR(FALSE)
