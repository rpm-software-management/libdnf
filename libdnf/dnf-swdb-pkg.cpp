/* dnf-swdb-pkg.c
 *
 * Copyright (C) 2017 Red Hat, Inc.
 * Author: Eduard Cuba <xcubae00@stud.fit.vutbr.cz>
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

#include "dnf-swdb-pkg.h"
#include "dnf-swdb-pkg-sql.h"
#include "dnf-swdb.h"

G_DEFINE_TYPE (DnfSwdbPkg, dnf_swdb_pkg, G_TYPE_OBJECT) // history package

// SWDB package Destructor
static void
dnf_swdb_pkg_finalize (GObject *object)
{
    DnfSwdbPkg *pkg = (DnfSwdbPkg *)object;
    g_free (pkg->name);
    g_free (pkg->version);
    g_free (pkg->release);
    g_free (pkg->arch);
    g_free (pkg->checksum_data);
    g_free (pkg->checksum_type);
    g_free (pkg->state);
    g_free (pkg->_ui_repo);
    g_free (pkg->nevra);
    G_OBJECT_CLASS (dnf_swdb_pkg_parent_class)->finalize (object);
}

// SWDB Package Class initialiser
static void
dnf_swdb_pkg_class_init (DnfSwdbPkgClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize = dnf_swdb_pkg_finalize;
}

// SWDB Package object initialiser
static void
dnf_swdb_pkg_init (DnfSwdbPkg *self)
{
    self->done = 0;
    self->state = NULL;
    self->pid = 0;
    self->_ui_repo = NULL;
    self->swdb = NULL;
    self->nevra = NULL;
    self->type = DNF_SWDB_ITEM_RPM;
    self->obsoleting = 0;
}

static gchar *
_pkg_make_nevra (DnfSwdbPkg *self)
{
    if (self->epoch) {
        return g_strdup_printf ("%s-%d:%s-%s.%s",
                                self->name,
                                self->epoch,
                                self->version,
                                self->release,
                                self->arch);
    }
    return g_strdup_printf ("%s-%s-%s.%s",
                            self->name,
                            self->version,
                            self->release,
                            self->arch);
}

/**
 * dnf_swdb_pkg_new:
 *
 * Creates a new #DnfSwdbPkg.
 *
 * Returns: a #DnfSwdbPkg
 **/
DnfSwdbPkg *
dnf_swdb_pkg_new (const gchar *name,
                  gint         epoch,
                  const gchar *version,
                  const gchar *release,
                  const gchar *arch,
                  const gchar *checksum_data,
                  const gchar *checksum_type,
                  DnfSwdbItem  type)
{
    auto swdbpkg = DNF_SWDB_PKG(g_object_new(DNF_TYPE_SWDB_PKG, NULL));
    swdbpkg->name = g_strdup (name);
    swdbpkg->epoch = epoch;
    swdbpkg->version = g_strdup (version);
    swdbpkg->release = g_strdup (release);
    swdbpkg->arch = g_strdup (arch);
    swdbpkg->checksum_data = g_strdup (checksum_data);
    swdbpkg->checksum_type = g_strdup (checksum_type);
    swdbpkg->nevra = _pkg_make_nevra (swdbpkg);
    swdbpkg->type = type;
    return swdbpkg;
}

/**
 * dnf_swdb_pkg_get_reason:
 * @self: package object
 *
 * Returns: package reason ID
 **/
DnfSwdbReason
dnf_swdb_pkg_get_reason (DnfSwdbPkg *self)
{
    DnfSwdbReason reason = DNF_SWDB_REASON_UNKNOWN;
    if (!self || dnf_swdb_open (self->swdb)) {
        return reason;
    }

    if (self->pid) {
        reason = _reason_by_pid (self->swdb->db, self->pid);
    }
    return reason;
}

/**
 * dnf_swdb_pkg_ui_from_repo:
 * @self: package object
 *
 * Get UI reponame for package @self
 * If package was installed with some other releasever, then
 * print repository name in format @repo/releasever
 *
 * Returns: repository UI name
 **/
gchar *
dnf_swdb_pkg_ui_from_repo (DnfSwdbPkg *self)
{
    if (self->_ui_repo)
        return g_strdup (self->_ui_repo);
    if (!self->swdb || !self->pid || dnf_swdb_open (self->swdb)) {
        return g_strdup ("unknown");
    }
    sqlite3_stmt *res;
    const gchar *sql = S_REPO_FROM_PID;
    gint pdid = 0;
    _db_prepare (self->swdb->db, sql, &res);
    _db_bind_int (res, "@pid", self->pid);
    g_autofree gchar *r_name = NULL;
    if (sqlite3_step (res) == SQLITE_ROW) {
        r_name = g_strdup ((const gchar *)sqlite3_column_text (res, 0));
        pdid = sqlite3_column_int (res, 1);
    }
    sqlite3_finalize (res);

    // now we find out if package wasnt installed from some other releasever
    if (pdid && self->swdb->releasever) {
        g_autofree gchar *cur_releasever = NULL;
        sql = S_RELEASEVER_FROM_PDID;
        _db_prepare (self->swdb->db, sql, &res);
        _db_bind_int (res, "@pdid", pdid);
        if (sqlite3_step (res) == SQLITE_ROW) {
            cur_releasever = g_strdup ((const gchar *)sqlite3_column_text (res, 0));
        }
        sqlite3_finalize (res);
        if (cur_releasever && g_strcmp0 (cur_releasever, self->swdb->releasever)) {
            self->_ui_repo = g_strjoin (NULL, "@", r_name, "/", cur_releasever, NULL);
            return g_strdup (self->_ui_repo);
        }
    }
    if (r_name) {
        self->_ui_repo = g_strjoin(NULL, "@", r_name, NULL);
        return g_strdup (self->_ui_repo);
    }
    return g_strdup ("unknown");
}

/**
 * dnf_swdb_pkg_compare:
 * @pkg1: first package
 * @pkg2: second package
 *
 * Performs "@pkg2 - @pkg1"
 *
 * Returns: < 0 when is @pkg1 newer, > 0 when older, else 0
 **/
gint64
dnf_swdb_pkg_compare (DnfSwdbPkg *pkg1, DnfSwdbPkg *pkg2)
{
    // compare epochs
    gint64 res = pkg2->epoch - pkg1->epoch;
    if (res) {
        return res;
    }

    // compare versions

    // split version string into substrings
    g_auto (GStrv) version1 = g_strsplit (pkg1->version, ".", 0);
    if (!version1) {
        return 0;
    }

    g_auto (GStrv) version2 = g_strsplit (pkg2->version, ".", 0);
    if (!version2) {
        return 0;
    }

    // compare subversions
    const gchar *subv1 = *version1;
    const gchar *subv2 = *version2;

    gint i = 0;
    res = 0;
    while (subv1 && subv2) {
        // convert subversions into int
        gint64 v1 = g_ascii_strtoll (subv1, NULL, 0);
        gint64 v2 = g_ascii_strtoll (subv2, NULL, 0);

        // compare subversions
        res = v2 - v1;
        if (res) {
            break;
        }

        // move to next subversion
        i++;
        subv1 = *(version1 + i);
        subv2 = *(version2 + i);
    }

    return res;
}

/**
 * dnf_swdb_pkg___lt__:
 * @first: first package
 * @second: second package
 *
 * Returns: %true when @first < @second - @second is newer
 **/
gboolean
dnf_swdb_pkg___lt__ (DnfSwdbPkg *first, DnfSwdbPkg *second)
{
    return dnf_swdb_pkg_compare (first, second) > 0;
}

/**
 * dnf_swdb_pkg___gt__:
 * @first: first package
 * @second: second package
 *
 * Returns: %true when @first > @second - @first is newer
 **/
gboolean
dnf_swdb_pkg___gt__ (DnfSwdbPkg *first, DnfSwdbPkg *second)
{
    return dnf_swdb_pkg_compare (first, second) < 0;
}

/**
 * dnf_swdb_pkg___str__:
 * @pkg: package object
 *
 * Returns: text represenation of the package - nevra
 **/
gchar *
dnf_swdb_pkg___str__ (DnfSwdbPkg *pkg)
{
    return g_strdup (pkg->nevra);
}

/**
 * dnf_swdb_pkg_match:
 * @pkg: package object
 * @pattern: string/regex to match with package
 *
 * Accepted forms:
 * - name
 * - name.arch
 * - name-version-release.arch
 * - name-epoch:version-release.arch
 * - name-version
 * - name-version-release
 * - epoch:name-version-release.arch (depreceated)
 *
 * Returns: %true if package can be matched with pattern in any accepted form (case insensitive)
 **/
gboolean
dnf_swdb_pkg_match (DnfSwdbPkg *pkg, const gchar *pattern)
{
    const gchar *n = pkg->name;
    const gchar *v = pkg->version;
    const gchar *r = pkg->release;
    const gchar *a = pkg->arch;
    const gint e = pkg->epoch;

    // create regular expression for pattern
    GError *err = NULL;
    g_autoptr (GRegex) regex = g_regex_new(pattern, G_REGEX_CASELESS,
                                           static_cast<GRegexMatchFlags>(0), &err);

    if (err) {
        g_warning ("Error parsing regular expression: %s", err->message);
        return FALSE;
    }

    // match against name
    if (g_regex_match (regex, n, static_cast<GRegexMatchFlags>(0), NULL)) {
        return TRUE;
    }

    //match against name.arch
    g_autofree gchar *name_arch = g_strjoin (".", n, a, NULL);
    if (g_regex_match (regex, name_arch, static_cast<GRegexMatchFlags>(0), NULL)) {
        return TRUE;
    }

    //match against name-version-release.arch
    g_autofree gchar *nvra = g_strdup_printf ("%s-%s-%s.%s", n, v, r, a);
    if (g_regex_match (regex, nvra, static_cast<GRegexMatchFlags>(0), NULL)) {
        return TRUE;
    }

    //match against name-epoch:version-release.arch
    g_autofree gchar *nevra = g_strdup_printf ("%s-%d:%s-%s.%s", n, e, v, r, a);
    if (g_regex_match (regex, nvra, static_cast<GRegexMatchFlags>(0), NULL)) {
        return TRUE;
    }

    //match against name-version
    g_autofree gchar *name_version = g_strjoin ("-", n, v, NULL);
    if (g_regex_match (regex, name_version, static_cast<GRegexMatchFlags>(0), NULL)) {
        return TRUE;
    }

    //match against name-version-release
    g_autofree gchar *nvr = g_strjoin ("-", n, v, r, NULL);
    if (g_regex_match (regex, nvr, static_cast<GRegexMatchFlags>(0), NULL)) {
        return TRUE;
    }

    //match against epoch:name-version-release.arch
    g_autofree gchar *envra = g_strdup_printf ("%d:%s-%s-%s.%s", e, n, v, r, a);
    if (g_regex_match (regex, envra, static_cast<GRegexMatchFlags>(0), NULL)) {
        return TRUE;
    }

    return FALSE;
}
