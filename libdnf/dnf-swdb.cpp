/* dnf-swdb.c
 *
 * Copyright (C) 2017 Red Hat, Inc.
 * Author: Eduard Cuba <ecuba@redhat.com>
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

#include "dnf-swdb.h"
#include "dnf-swdb-sql.h"
#include "dnf-swdb-trans.h"
#include "hy-nevra.hpp"
#include "hy-subject-private.hpp"
#include "hy-subject.h"
#include "hy-types.h"

G_DEFINE_TYPE (DnfSwdb, dnf_swdb, G_TYPE_OBJECT)
G_DEFINE_TYPE (DnfSwdbTransData, dnf_swdb_transdata, G_TYPE_OBJECT) // trans data
G_DEFINE_TYPE (DnfSwdbPkgData, dnf_swdb_pkgdata, G_TYPE_OBJECT)

// SWDB object Destructor
static void
dnf_swdb_finalize (GObject *object)
{
    DnfSwdb *swdb = (DnfSwdb *)object;
    dnf_swdb_close (swdb);
    g_free (swdb->path);
    g_free (swdb->releasever);
    G_OBJECT_CLASS (dnf_swdb_parent_class)->finalize (object);
}

// SWDB Class initialiser
static void
dnf_swdb_class_init (DnfSwdbClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize = dnf_swdb_finalize;
}

// SWDB Object initialiser
static void
dnf_swdb_init (DnfSwdb *self)
{
    self->path = NULL;
    self->releasever = NULL;
}

/**
 * dnf_swdb_new:
 *
 * Creates a new #DnfSwdb.
 *
 * Returns: a #DnfSwdb
 **/
DnfSwdb *
dnf_swdb_new (const gchar *db_path, const gchar *releasever)
{
    auto swdb = DNF_SWDB(g_object_new(DNF_TYPE_SWDB, NULL));
    if (releasever) {
        swdb->releasever = g_strdup (releasever);
    }
    if (db_path) {
        g_free (swdb->path);
        swdb->path = g_strdup (db_path);
    }
    return swdb;
}

// SWDB Package Data Destructor
static void
dnf_swdb_pkgdata_finalize (GObject *object)
{
    DnfSwdbPkgData *pkgdata = (DnfSwdbPkgData *)object;
    g_free (pkgdata->from_repo);
    g_free (pkgdata->from_repo_revision);
    g_free (pkgdata->installed_by);
    g_free (pkgdata->changed_by);
    G_OBJECT_CLASS (dnf_swdb_pkgdata_parent_class)->finalize (object);
}

// SWDB Package Data Class initialiser
static void
dnf_swdb_pkgdata_class_init (DnfSwdbPkgDataClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize = dnf_swdb_pkgdata_finalize;
}

// SWDB Package Data Object initialiser
static void
dnf_swdb_pkgdata_init (DnfSwdbPkgData *self)
{
    self->from_repo = NULL;
    self->from_repo_revision = NULL;
    self->from_repo_timestamp = 0;
    self->installed_by = NULL;
    self->changed_by = NULL;
}

/**
 * dnf_swdb_pkgdata_new:
 *
 * Creates a new #DnfSwdbPkgData.
 *
 * Returns: a #DnfSwdbPkgData
 **/
DnfSwdbPkgData *
dnf_swdb_pkgdata_new (const gchar *from_repo_revision,
                      gint64 from_repo_timestamp,
                      const gchar *installed_by,
                      const gchar *changed_by,
                      const gchar *from_repo)
{
    auto pkgdata = DNF_SWDB_PKGDATA(g_object_new (DNF_TYPE_SWDB_PKGDATA, NULL));
    pkgdata->from_repo_revision = g_strdup (from_repo_revision);
    pkgdata->from_repo_timestamp = from_repo_timestamp;
    pkgdata->installed_by = g_strdup (installed_by);
    pkgdata->changed_by = g_strdup (changed_by);
    pkgdata->from_repo = g_strdup (from_repo);
    return pkgdata;
}

// SWDB Transaction Data Destructor
static void
dnf_swdb_transdata_finalize (GObject *object)
{
    DnfSwdbTransData *tdata = (DnfSwdbTransData *)object;
    g_free (tdata->state);
    G_OBJECT_CLASS (dnf_swdb_transdata_parent_class)->finalize (object);
}

// SWDB Transaction Data Class initialiser
static void
dnf_swdb_transdata_class_init (DnfSwdbTransDataClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    object_class->finalize = dnf_swdb_transdata_finalize;
}

// TRANS DATA Object initialiser
static void
dnf_swdb_transdata_init (DnfSwdbTransData *self)
{
}

/**
 * dnf_swdb_transdata_new:
 *
 * Creates a new #DnfSwdbTransData.
 *
 * Returns: a #DnfSwdbTransData
 **/
DnfSwdbTransData *
dnf_swdb_transdata_new (gint tdid,
                        gint tid,
                        gint pdid,
                        gint tgid,
                        gint done,
                        gint obsoleting,
                        DnfSwdbReason reason,
                        gchar *state)
{
    auto data = DNF_SWDB_TRANSDATA(g_object_new (DNF_TYPE_SWDB_TRANSDATA, NULL));
    data->tdid = tdid;
    data->tid = tid;
    data->pdid = pdid;
    data->tgid = tgid;
    data->done = done;
    data->obsoleting = obsoleting;
    data->reason = reason;
    data->state = g_strdup (state);
    return data;
}

/**
 * dnf_swdb_search:
 * @patterns: (element-type utf8) (transfer container): list of patterns
 *
 * Find transactions containing packages matched by list of patterns
 * Accepted pattern formats:
 *   name
 *   name.arch
 *   name-version-release.arch
 *   name-version
 *   epoch:name-version-release.arch
 *   name-epoch-version-release.arch
 *
 * Returns: (element-type gint32) (transfer container): list of constants
 */
GArray *
dnf_swdb_search (DnfSwdb *self, GPtrArray *patterns)
{
    if (dnf_swdb_open (self))
        return NULL;

    GArray *tids = g_array_new (0, 0, sizeof (gint));
    for (guint i = 0; i < patterns->len; ++i) {
        auto pattern = static_cast<const gchar *>(g_ptr_array_index(patterns, i));

        // try simple search based on package name
        GArray *simple = _simple_search (self->db, pattern);
        if (simple->len) {
            tids = g_array_append_vals (tids, simple->data, simple->len);
            g_array_free (simple, TRUE);
            continue;
        }
        g_array_free (simple, TRUE);
        // need for extended search - slower
        GArray *extended = _extended_search (self->db, pattern);
        if (extended) {
            tids = g_array_append_vals (tids, extended->data, extended->len);
        }
        g_array_free (extended, TRUE);
    }
    return tids;
}

/**
 * _fill_nevra_res:
 * Parse nevra and fill SQL expression
 **/
static HyNevra
_fill_nevra_res (sqlite3_stmt *res, const gchar *nevra)
{
    auto hnevra = new libdnf::Nevra;
    if (hy_nevra_possibility(nevra, HY_FORM_NEVRA, hnevra)) {
        return NULL;
    }

    gint epoch = hnevra->getEpoch();
    if (epoch == -1) {
        epoch = 0;
    }
    _db_bind_str (res, "@n", hnevra->getName().c_str());
    _db_bind_int (res, "@e", epoch);
    _db_bind_str (res, "@v", hnevra->getVersion().c_str());
    _db_bind_str (res, "@r", hnevra->getRelease().c_str());
    _db_bind_str (res, "@a", hnevra->getArch().c_str());

    return hnevra;
}

/**
 * _pid_by_nevra:
 * @db: sqlite database handle
 * @nevra: string in format name-[epoch:]version-release.arch
 *
 * Get Package ID from SWDB by package nevra
 * Indexed nevra provides fast binding between other package objects
 *
 * Returns: id of package matched by @nevra
 **/
static gint
_pid_by_nevra (sqlite3 *db, const gchar *nevra)
{
    const gchar *sql = S_PID_BY_NEVRA;
    sqlite3_stmt *res;
    _db_prepare (db, sql, &res);

    HyNevra hnevra = _fill_nevra_res (res, nevra);
    if (hnevra == NULL) {
        // failed to parse nevra
        sqlite3_finalize (res);
        return 0;
    }
    gint rc = _db_find_int (res);
    delete hnevra;
    return rc;
}

/**
 * dnf_swdb_checksums:
 * @nevras: (element-type utf8) (transfer container): list of patterns
 *
 * Patch packages by name-[epoch:]version-release.arch and return list of their
 * checksums.
 * Output is in format:
 *   [nevra-1, checksum_type-1, checksum_data-1, nevra-2, checksum_type-2, checksum_data-2, ...]
 *
 * Used for performace optimization of rpmdb_version calculation in DNF
 *
 * Returns: (element-type utf8) (transfer container): list of checksums
 **/
GPtrArray *
dnf_swdb_checksums (DnfSwdb *self, GPtrArray *nevras)
{
    if (dnf_swdb_open (self))
        return NULL;
    gchar *buff1;
    gchar *buff2;
    gchar *nevra;

    GPtrArray *checksums = g_ptr_array_new ();
    const gchar *sql = S_CHECKSUM_BY_NEVRA;
    for (guint i = 0; i < nevras->len; i++) {
        // get nevra
        nevra = (gchar *)g_ptr_array_index (nevras, i);

        // save nevra to output list
        g_ptr_array_add (checksums, (gpointer)g_strdup (nevra)); // nevra

        // prepare the sql statement
        sqlite3_stmt *res;
        _db_prepare (self->db, sql, &res);

        gboolean inserted = FALSE;

        HyNevra hnevra = _fill_nevra_res (res, nevra);

        // parse and lookup nevra
        if (hnevra && sqlite3_step (res) == SQLITE_ROW) {
            buff1 = (gchar *)sqlite3_column_text (res, 0);
            buff2 = (gchar *)sqlite3_column_text (res, 1);
            if (buff1 && buff2) {
                g_ptr_array_add (checksums, (gpointer)g_strdup (buff2)); // type
                g_ptr_array_add (checksums, (gpointer)g_strdup (buff1)); // data
                inserted = TRUE;
            }
        }
        sqlite3_finalize (res);
        delete hnevra;

        if (!inserted) {
            // insert plain checksum data to keep the order correct
            g_ptr_array_add (checksums, NULL);
            g_ptr_array_add (checksums, NULL);
        }
    }
    return checksums;
}

/**
 * dnf_swdb_select_user_installed:
 * @nevras: (element-type utf8)(transfer container): list of patterns
 *
 * Match user installed packages by their name-[epoch:]version-release.arch
 * Return id's of user installed pacakges
 *
 * Example:
 *   @nevras = ["abc", "def", "ghi"]
 *   # "abc" and "ghi" are user installed packages
 *   Output:[0,2]
 *
 * Returns: (element-type gint32)(transfer container): list of matched indexes
 **/
GArray *
dnf_swdb_select_user_installed (DnfSwdb *self, GPtrArray *nevras)
{
    if (dnf_swdb_open (self))
        return NULL;

    GArray *usr_ids = g_array_new (0, 0, sizeof (gint));

    gint pid = 0;
    gint pdid = 0;
    DnfSwdbReason reason_id = DNF_SWDB_REASON_UNKNOWN;
    const gchar *sql = S_REASON_ID_BY_PDID;

    for (guint i = 0; i < nevras->len; ++i) {
        pid = _pid_by_nevra (self->db, (gchar *)g_ptr_array_index (nevras, i));
        if (!pid) {
            // better not uninstall package when not sure
            g_array_append_val (usr_ids, i);
            continue;
        }
        pdid = _pdid_from_pid (self->db, pid);
        if (!pdid) {
            g_array_append_val (usr_ids, i);
            continue;
        }
        sqlite3_stmt *res;
        _db_prepare (self->db, sql, &res);
        _db_bind_int (res, "@pdid", pdid);
        gboolean push = TRUE;
        while (sqlite3_step (res) == SQLITE_ROW) {
            reason_id = static_cast<DnfSwdbReason>(sqlite3_column_int(res, 0));
            if (reason_id == DNF_SWDB_REASON_DEP || reason_id == DNF_SWDB_REASON_WEAK) {
                push = FALSE;
                continue;
            }
            push = TRUE;
            break;
        }
        // push even in case when no record is found
        if (push) {
            g_array_append_val (usr_ids, i);
        }
        sqlite3_finalize (res);
    }
    return usr_ids;
}

/**
 * dnf_swdb_user_installed:
 * @self: SWDB object
 * @nevra: string in format name-[epoch:]version-release.arch
 * Returns: %TRUE if package is user installed
 **/
gboolean
dnf_swdb_user_installed (DnfSwdb *self, const gchar *nevra)
{
    if (dnf_swdb_open (self))
        return TRUE;

    gint pid = _pid_by_nevra (self->db, nevra);
    if (!pid) {
        return FALSE;
    }
    gboolean rc = TRUE;
    gchar *repo = _repo_by_pid (self->db, pid);
    DnfSwdbReason reason = _reason_by_pid (self->db, pid);
    if (reason != DNF_SWDB_REASON_USER || !g_strcmp0 ("anakonda", repo)) {
        rc = FALSE;
    }
    g_free (repo);
    return rc;
}

/***************************** REPO PERSISTOR ********************************/

/**
 * _bind_repo_by_name:
 * @db: sqlite database handle
 * @name: repository name
 * Returns: repository ID
 **/
static gint
_bind_repo_by_name (sqlite3 *db, const gchar *name)
{
    if (!name) {
        return 0;
    }
    sqlite3_stmt *res;
    const gchar *sql = FIND_REPO_BY_NAME;
    _db_prepare (db, sql, &res);
    _db_bind_str (res, "@name", name);
    gint rc = _db_find_int (res);
    if (rc) {
        return rc;
    }
    sql = INSERT_REPO;
    _db_prepare (db, sql, &res);
    _db_bind_str (res, "@name", name);
    _db_step (res);
    return sqlite3_last_insert_rowid (db);
}

/**
 * _repo_by_rid:
 * @db: sqlite database handle
 * @rid: repository ID
 * Returns: repository name
 **/
gchar *
_repo_by_rid (sqlite3 *db, gint rid)
{
    sqlite3_stmt *res;
    const gchar *sql = S_REPO_BY_RID;
    _db_prepare (db, sql, &res);
    _db_bind_int (res, "@rid", rid);
    return _db_find_str (res);
}

/**
 * _repo_by_pid:
 * @db: sqlite database handle
 * @pid: package id
 * Returns: repository name
 **/
gchar *
_repo_by_pid (sqlite3 *db, gint pid)
{
    sqlite3_stmt *res;
    const gchar *sql = S_REPO_FROM_PID2;
    _db_prepare (db, sql, &res);
    _db_bind_int (res, "@pid", pid);
    return _db_find_str (res);
}

/**
 * dnf_swdb_repo:
 * @self: SWDB object
 * @nevra: string in format name-[epoch:]version-release.arch
 * Returns: repository name
 **/
gchar *
dnf_swdb_repo (DnfSwdb *self, const gchar *nevra)
{
    if (dnf_swdb_open (self))
        return NULL;
    gint pid = _pid_by_nevra (self->db, nevra);
    if (!pid) {
        return g_strdup ("unknown");
    }
    return _repo_by_pid (self->db, pid);
}

/**
 * dnf_swdb_set_repo:
 * @self: SWDB object
 * @nevra: string in format name-[epoch:]version-release.arch
 * @repo: repository name
 *
 * Set source repository for package matched by @nevra
 *
 * Returns: 0 if source repository was set successfully
 **/
gint
dnf_swdb_set_repo (DnfSwdb *self, const gchar *nevra, const gchar *repo)
{
    if (dnf_swdb_open (self))
        return 1;
    gint pid = _pid_by_nevra (self->db, nevra);
    if (!pid) {
        return 1;
    }
    gint rid = _bind_repo_by_name (self->db, repo);
    sqlite3_stmt *res;
    const gchar *sql = U_REPO_BY_PID;
    _db_prepare (self->db, sql, &res);
    _db_bind_int (res, "@rid", rid);
    _db_bind_int (res, "@pid", pid);
    _db_step (res);
    return 0;
}

/**************************** PACKAGE PERSISTOR ******************************/

/**
 * _package_insert:
 * @self: SWDB object
 * @package: package object
 *
 * Insert package object into swdb
 *
 * Returns: inserted package ID
 **/
static gint
_package_insert (DnfSwdb *self, DnfSwdbPkg *package)
{
    sqlite3_stmt *res;
    const gchar *sql = INSERT_PKG;
    _db_prepare (self->db, sql, &res);
    _db_bind_str (res, "@name", package->name);
    _db_bind_int (res, "@epoch", package->epoch);
    _db_bind_str (res, "@version", package->version);
    _db_bind_str (res, "@release", package->release);
    _db_bind_str (res, "@arch", package->arch);
    _db_bind_str (res, "@cdata", package->checksum_data);
    _db_bind_str (res, "@ctype", package->checksum_type);
    _db_bind_int (res, "@type", package->type);
    _db_step (res);
    package->pid = sqlite3_last_insert_rowid (self->db);
    return package->pid;
}

/**
 * dnf_swdb_add_package:
 * @self: SWDB object
 * @pkg: package object
 *
 * Insert package into database
 *
 * Returns: inserted package ID or -1 if database error occurred
 **/
gint
dnf_swdb_add_package (DnfSwdb *self, DnfSwdbPkg *pkg)
{
    if (dnf_swdb_open (self))
        return -1;
    gint rc = _package_insert (self, pkg);
    return rc;
}

/**
 * dnf_swdb_update_package_data:
 * @self: SWDB object
 * @pid: package ID
 * @tid: transation ID
 * @pkgdata: package data object
 *
 * Insert or update pkgdata for package with ID @pid in transation @tid.
 * This method MUST be called after transaction data initialization by `dnf_swdb_trans_data_beg`
 *
 * Returns: 0 if successful
 **/
gint
dnf_swdb_update_package_data (DnfSwdb *self, gint pid, gint tid, DnfSwdbPkgData *pkgdata)
{
    if (dnf_swdb_open (self))
        return 1;

    gint rid = _bind_repo_by_name (self->db, pkgdata->from_repo);

    // look for old pid's package data - all values of autogenerated rows are null
    sqlite3_stmt *res;
    const gchar *sql = S_PREV_PKG_DATA;
    _db_prepare (self->db, sql, &res);
    _db_bind_int (res, "@pid", pid);
    _db_bind_int (res, "@tid", tid);
    gint pdid = _db_find_int (res);

    if (!pdid) {
        // field is not available
        g_warning ("Package %d is not available in transation %d", pid, tid);
        return 1;
    }

    sql = UPDATE_PKG_DATA;
    _db_prepare (self->db, sql, &res);
    _db_bind_int (res, "@pid", pid);
    _db_bind_int (res, "@rid", rid);
    _db_bind_str (res, "@repo_r", pkgdata->from_repo_revision);
    _db_bind_int (res, "@repo_t", pkgdata->from_repo_timestamp);
    _db_bind_str (res, "@installed_by", pkgdata->installed_by);
    _db_bind_str (res, "@changed_by", pkgdata->changed_by);
    _db_step (res);
    return 0;
}

/**
 * _new_pdid_for_pid:
 * @db: sqlite database handle
 * @pid: package ID
 *
 * Create new record in PACKAGE_DATA table for @pid
 *
 * Returns: new pdid
 **/
static gint
_new_pdid_for_pid (sqlite3 *db, gint pid)
{
    sqlite3_stmt *res;
    const gchar *sql = INSERT_PDID;
    _db_prepare (db, sql, &res);
    _db_bind_int (res, "@pid", pid);
    _db_step (res);
    return sqlite3_last_insert_rowid (db);
}

/**
 * _pdid_from_pid:
 * @db: sqlite database handle
 * @pid: package ID
 *
 * Get package data ID for package with ID @pid.
 * Create new package data record if unavailable.
 *
 * Returns: package data ID
 **/
gint
_pdid_from_pid (sqlite3 *db, gint pid)
{
    sqlite3_stmt *res;
    const gchar *sql = FIND_PDID_FROM_PID;
    _db_prepare (db, sql, &res);
    _db_bind_int (res, "@pid", pid);
    gint rc = _db_find_int (res);
    if (rc)
        return rc;
    return _new_pdid_for_pid (db, pid);
}

/**
 * _pids_by_tid:
 * @db: sqlite database handle
 * @tid: transaction ID
 *
 * Get all package IDs connected with transation with ID @tid
 *
 * Returns: list od package IDs
 **/
static GArray *
_pids_by_tid (sqlite3 *db, gint tid)
{
    GArray *pids = g_array_new (0, 0, sizeof (gint));
    sqlite3_stmt *res;
    const gchar *sql = PID_BY_TID;
    _db_prepare (db, sql, &res);
    _db_bind_int (res, "@tid", tid);
    gint pid;
    while ((pid = _db_find_int_multi (res))) {
        g_array_append_val (pids, pid);
    }
    return pids;
}

/**
 * _reason_by_pid:
 * @db: sqlite database handle
 * @pid: package ID
 *
 * Returns: reason id for package with @pid
 **/
DnfSwdbReason
_reason_by_pid (sqlite3 *db, gint pid)
{
    sqlite3_stmt *res;
    const gchar *sql = S_REASON_BY_PID;
    _db_prepare (db, sql, &res);
    _db_bind_int (res, "@pid", pid);
    return static_cast<DnfSwdbReason>(_db_find_int(res));
}

/**
 * dnf_swdb_reason:
 * @self: SWDB object
 * @nevra: string in format name-[epoch:]version-release.arch
 *
 * Returns: reason ID for package matched by @nevra
 **/
DnfSwdbReason
dnf_swdb_reason (DnfSwdb *self, const gchar *nevra)
{
    DnfSwdbReason reason = DNF_SWDB_REASON_UNKNOWN;
    if (dnf_swdb_open (self))
        return reason;
    gint pid = _pid_by_nevra (self->db, nevra);
    if (pid) {
        reason = _reason_by_pid (self->db, pid);
    }
    return reason;
}

/**
 * _resolve_package_state:
 * @self: SWDB object
 * @pkg: package object
 * @tid: transation ID
 *
 * Set package state to match transaction given by @tid
 **/
static void
_resolve_package_state (DnfSwdb *self, DnfSwdbPkg *pkg, gint tid)
{
    sqlite3_stmt *res;
    const gchar *sql = S_PACKAGE_STATE;
    _db_prepare (self->db, sql, &res);
    _db_bind_int (res, "@pid", pkg->pid);
    _db_bind_int (res, "@tid", tid);
    if (sqlite3_step (res) == SQLITE_ROW) {
        pkg->done = sqlite3_column_int (res, 0);
        gint state_code = sqlite3_column_int (res, 1);
        pkg->obsoleting = sqlite3_column_int (res, 2);
        sqlite3_finalize (res);
        pkg->state = _get_description (self->db, state_code, S_STATE_TYPE_BY_ID);
    } else {
        sqlite3_finalize (res);
    }
}

/**
 * _get_package_data_by_pid:
 * @db: sqlite database handle
 * @pid: package ID
 *
 * Returns: latest package data object for package with ID @pid
 **/
static DnfSwdbPkgData *
_get_package_data_by_pid (sqlite3 *db, gint pid)
{
    sqlite3_stmt *res;
    const gchar *sql = S_PACKAGE_DATA_BY_PID;
    _db_prepare (db, sql, &res);
    _db_bind_int (res, "@pid", pid);
    if (sqlite3_step (res) == SQLITE_ROW) {
        DnfSwdbPkgData *pkgdata =
          dnf_swdb_pkgdata_new ((gchar *)sqlite3_column_text (res, 3), // from_repo_revision
                                sqlite3_column_int (res, 4),           // from_repo_timestamp
                                (gchar *)sqlite3_column_text (res, 5), // installed_by
                                (gchar *)sqlite3_column_text (res, 6), // changed_by
                                NULL);                                 // from_repo
        pkgdata->pdid = sqlite3_column_int (res, 0);                   // pdid
        pkgdata->pid = pid;
        gint rid = sqlite3_column_int (res, 2);
        sqlite3_finalize (res);
        pkgdata->from_repo = _repo_by_rid (db, rid); // from repo
        return pkgdata;
    }
    sqlite3_finalize (res);
    return NULL;
}

/**
 * _get_package_by_pid:
 * @db: sqlite database handle
 * @pid: package ID
 *
 * Returns: package object for @pid
 **/
DnfSwdbPkg *
_get_package_by_pid (sqlite3 *db, gint pid)
{
    sqlite3_stmt *res;
    const gchar *sql = S_PACKAGE_BY_PID;
    _db_prepare (db, sql, &res);
    _db_bind_int (res, "@pid", pid);
    if (sqlite3_step (res) == SQLITE_ROW) {
        DnfSwdbPkg *pkg = dnf_swdb_pkg_new ((gchar *)sqlite3_column_text (res, 1), // name
                                            sqlite3_column_int (res, 2),           // epoch
                                            (gchar *)sqlite3_column_text (res, 3), // version
                                            (gchar *)sqlite3_column_text (res, 4), // release
                                            (gchar *)sqlite3_column_text (res, 5), // arch
                                            (gchar *)sqlite3_column_text (res, 6), // checksum_data
                                            (gchar *)sqlite3_column_text (res, 7), // checksum_type
                                            static_cast<DnfSwdbItem>(
                                                sqlite3_column_int(res, 8))); // type
        pkg->pid = pid;
        sqlite3_finalize (res);
        return pkg;
    }
    sqlite3_finalize (res);
    return NULL;
}

/**
 * dnf_swdb_get_packages_by_tid:
 * @self: SWDB object
 * @tid: transation ID
 *
 * Get list of package object featured in transaction with ID @tid
 *
 * Returns: (element-type DnfSwdbPkg)(array)(transfer container): list of #DnfSwdbPkg
 */
GPtrArray *
dnf_swdb_get_packages_by_tid (DnfSwdb *self, gint tid)
{
    if (dnf_swdb_open (self))
        return NULL;

    GPtrArray *node = g_ptr_array_new ();
    GArray *pids = _pids_by_tid (self->db, tid);
    gint pid;
    DnfSwdbPkg *pkg;
    for (guint i = 0; i < pids->len; i++) {
        pid = g_array_index (pids, gint, i);
        pkg = _get_package_by_pid (self->db, pid);
        if (pkg) {
            pkg->swdb = self;
            _resolve_package_state (self, pkg, tid);
            g_ptr_array_add (node, (gpointer)pkg);
        }
    }
    g_array_free (pids, TRUE);
    return node;
}

/**
 * dnf_swdb_pid_by_nevra:
 * @self: SWDB object
 * @nevra: string in format name-[epoch:]version-release.arch
 *
 * Returns: ID of package matched by @nevra or 0
 **/
gint
dnf_swdb_pid_by_nevra (DnfSwdb *self, const gchar *nevra)
{
    if (dnf_swdb_open (self))
        return 0;
    gint pid = _pid_by_nevra (self->db, nevra);
    return pid;
}

/**
 * dnf_swdb_package:
 * @self: SWDB object
 * @nevra: string in format name-[epoch:]version-release.arch
 *
 * Returns: (transfer full): #DnfSwdbPkg matched by @nevra
 **/
DnfSwdbPkg *
dnf_swdb_package (DnfSwdb *self, const gchar *nevra)
{
    if (dnf_swdb_open (self))
        return NULL;
    gint pid = _pid_by_nevra (self->db, nevra);
    if (!pid) {
        return NULL;
    }
    DnfSwdbPkg *pkg = _get_package_by_pid (self->db, pid);
    pkg->swdb = self;
    return pkg;
}

/**
 * dnf_swdb_package_data:
 * @self: SWDB object
 * @nevra: string in format name-[epoch:]version-release.arch
 *
 * Returns: (transfer full): #DnfSwdbPkgData for package matched by @nevra
 **/
DnfSwdbPkgData *
dnf_swdb_package_data (DnfSwdb *self, const gchar *nevra)
{
    if (dnf_swdb_open (self))
        return NULL;
    gint pid = _pid_by_nevra (self->db, nevra);
    if (!pid) {
        return NULL;
    }
    DnfSwdbPkgData *pkgdata = _get_package_data_by_pid (self->db, pid);
    return pkgdata;
}

/**
 * _mark_pkg_as:
 * @db: sqlite database handle
 * @pdid: package data ID
 * @mark: reason ID
 *
 * Set reason in package data with ID @pdid to @mark
 **/
static void
_mark_pkg_as (sqlite3 *db, gint pdid, DnfSwdbReason mark)
{
    sqlite3_stmt *res;
    const gchar *sql = U_REASON_BY_PDID;
    _db_prepare (db, sql, &res);
    _db_bind_int (res, "@reason", mark);
    _db_bind_int (res, "@pdid", pdid);
    _db_step (res);
}

/**
 * dnf_swdb_set_reason:
 * @self: SWDB object
 * @nevra: string in format name-[epoch:]version-release.arch
 * @reason: reason id
 *
 * Set reason of package matched by @nevra to @reason
 * Returns: 0 if successfull, else 1
 **/
gint
dnf_swdb_set_reason (DnfSwdb *self, const gchar *nevra, DnfSwdbReason reason)
{
    if (dnf_swdb_open (self))
        return 1;
    gint pid = _pid_by_nevra (self->db, nevra);
    if (!pid) {
        return 1;
    }
    gint pdid = _pdid_from_pid (self->db, pid);
    _mark_pkg_as (self->db, pdid, reason);
    return 0;
}

/*************************** TRANS DATA PERSISTOR ****************************/

/**
 * _resolve_group_origin:
 * @db: sqlite database handler
 * @tid: transaction ID
 * @pid: package ID
 *
 * Get trans group data of package installed with group
 *
 * Returns: trans group data ID for package with ID @pid
 **/
static gint
_resolve_group_origin (sqlite3 *db, gint tid, gint pid)
{
    sqlite3_stmt *res;
    const gchar *sql = RESOLVE_GROUP_TRANS;
    _db_prepare (db, sql, &res);
    _db_bind_int (res, "@pid", pid);
    _db_bind_int (res, "@tid", tid);
    return _db_find_int (res);
}

/**
 * dnf_swdb_trans_data_beg:
 * @self: SWDB object
 * @tid: transaction ID
 * @pid: package ID
 * @reason: reason ID
 * @state: state string
 *
 * Initialize transaction data for package with ID @pid in transaction @tid
 *
 * If package was installed with group, create binding between transaction data
 *   and transaction group data
 *
 * Returns: 0 if successfull
 **/
gint
dnf_swdb_trans_data_beg (
  DnfSwdb *self, gint tid, gint pid, DnfSwdbReason reason, const gchar *state, gint obsoleting)
{
    if (dnf_swdb_open (self))
        return 1;

    // generate new empty package data entry
    gint pdid = _new_pdid_for_pid (self->db, pid);

    // resolve group orogin
    gint tgid = 0;
    if (reason == DNF_SWDB_REASON_GROUP) {
        tgid = _resolve_group_origin (self->db, tid, pid);
    }

    // resolve state
    gint state_code = dnf_swdb_get_state_type (self, state);

    // insert new entry
    sqlite3_stmt *res;
    const gchar *sql = INSERT_TRANS_DATA_BEG;
    _db_prepare (self->db, sql, &res);
    _db_bind_int (res, "@tid", tid);
    _db_bind_int (res, "@pdid", pdid);
    _db_bind_int (res, "@tgid", tgid);
    _db_bind_int (res, "@obsoleting", obsoleting);
    _db_bind_int (res, "@reason", reason);
    _db_bind_int (res, "@state", state_code);
    _db_step (res);

    return 0;
}

/**
 * dnf_swdb_trans_data_pid_end:
 * @self: SWDB object
 * @pid: package ID
 * @tid: transaction ID
 * @state: state string
 *
 * Finalize transaction for package with ID @pid
 *
 * Returns: 0 if successfull
 **/
gint
dnf_swdb_trans_data_pid_end (DnfSwdb *self, gint pid, gint tid, const gchar *state)
{
    if (dnf_swdb_open (self))
        return 1;

    gint pdid = _pdid_from_pid (self->db, pid);
    gint state_code = dnf_swdb_get_state_type (self, state);

    sqlite3_stmt *res;
    const gchar *sql = UPDATE_TRANS_DATA_PID_END;
    _db_prepare (self->db, sql, &res);
    _db_bind_int (res, "@done", 1);
    _db_bind_int (res, "@tid", tid);
    _db_bind_int (res, "@pdid", pdid);
    _db_bind_int (res, "@state", state_code);
    _db_step (res);
    return 0;
}

/****************************** TRANS PERSISTOR ******************************/

/**
 * dnf_swdb_trans_beg:
 * @self: SWDB object
 * @timestamp: transaction timestamp
 * @rpmdb_version: rpmdb_version before transaction
 * @cmdline: command which initiated transaction
 * @loginuid: ID of user which initiated transaction
 * @releasever: release version of system
 *
 * Initialize transaction
 *
 * Returns: transaction ID or -1 if error occurred
 **/
gint
dnf_swdb_trans_beg (DnfSwdb *self,
                    gint64 timestamp,
                    const gchar *rpmdb_version,
                    const gchar *cmdline,
                    const gchar *loginuid,
                    const gchar *releasever)
{
    if (dnf_swdb_open (self))
        return -1;

    sqlite3_stmt *res;
    const gchar *sql = INSERT_TRANS_BEG;
    _db_prepare (self->db, sql, &res);
    _db_bind_int (res, "@beg", timestamp);
    _db_bind_str (res, "@rpmdbv", rpmdb_version);
    _db_bind_str (res, "@cmdline", cmdline);
    _db_bind_str (res, "@loginuid", loginuid);
    _db_bind_str (res, "@releasever", releasever);
    _db_step (res);
    gint rc = sqlite3_last_insert_rowid (self->db);
    return rc;
}

/**
 * dnf_swdb_trans_end:
 * @self: SWDB object
 * @tid: transaction ID
 * @end_timestamp: end timestamp
 * @end_rpmdb_version: rpmdb version after transaction
 * @return_code: return code of transaction
 *
 * Finalize transaction
 *
 * Returns: 0 if successfull
 **/
gint
dnf_swdb_trans_end (
  DnfSwdb *self, gint tid, gint64 end_timestamp, const gchar *end_rpmdb_version, gint return_code)
{
    if (dnf_swdb_open (self))
        return 1;
    sqlite3_stmt *res;
    const gchar *sql = INSERT_TRANS_END;
    _db_prepare (self->db, sql, &res);
    _db_bind_int (res, "@tid", tid);
    _db_bind_int (res, "@end", end_timestamp);
    _db_bind_str (res, "@rpmdbv", end_rpmdb_version);
    _db_bind_int (res, "@rc", return_code);
    _db_step (res);
    return 0;
}

/**
 * dnf_swdb_trans_with:
 * @self: SWDB object
 * @tid: transaction ID
 * @pid: package ID
 *
 * Log transaction perforemed with package
 **/
void
dnf_swdb_trans_with (DnfSwdb *self, int tid, int pid)
{
    if (dnf_swdb_open (self)) {
        return;
    }
    sqlite3_stmt *res;
    const gchar *sql = I_TRANS_WITH;
    _db_prepare (self->db, sql, &res);
    _db_bind_int (res, "@tid", tid);
    _db_bind_int (res, "@pid", pid);
    _db_step (res);
}

/**
 * dnf_swdb_trans_with_libdnf:
 * @self: SWDB object
 * @tid: transaction ID
 *
 * Log transaction perforemed with libdnf
 **/
void
dnf_swdb_trans_with_libdnf (DnfSwdb *self, int tid)
{
    if (dnf_swdb_open (self)) {
        return;
    }
    sqlite3_stmt *res;
    const gchar *sql = S_LATEST_PACKAGE;
    _db_prepare (self->db, sql, &res);
    _db_bind_str (res, "@name", "libdnf");
    gint pid = _db_find_int (res);
    if (pid) {
        dnf_swdb_trans_with (self, tid, pid);
    }
    _db_prepare (self->db, sql, &res);
    _db_bind_str (res, "@name", "rpm");
    pid = _db_find_int (res);
    if (pid) {
        dnf_swdb_trans_with (self, tid, pid);
    }
}

/**
 * dnf_swdb_trans_cmdline:
 * @self: SWDB object
 * @tid: transaction ID
 *
 * Returns: command line which initiated transaction @tid
 **/
gchar *
dnf_swdb_trans_cmdline (DnfSwdb *self, gint tid)
{
    if (dnf_swdb_open (self))
        return NULL;
    sqlite3_stmt *res;
    const gchar *sql = GET_TRANS_CMDLINE;
    _db_prepare (self->db, sql, &res);
    _db_bind_int (res, "@tid", tid);
    gchar *cmdline = _db_find_str (res);
    return cmdline;
}

/**
 * _resolve_altered:
 * @trans: list of #DnfSwdbTrans
 *
 * Resolve if rpmdb altered between transactions
 **/
static void
_resolve_altered (GPtrArray *trans)
{
    for (guint i = 0; i < (trans->len - 1); i++) {
        DnfSwdbTrans *las = (DnfSwdbTrans *)g_ptr_array_index (trans, i);
        DnfSwdbTrans *obj = (DnfSwdbTrans *)g_ptr_array_index (trans, i + 1);

        if (!las->end_rpmdb_version || !obj->beg_rpmdb_version)
            continue;

        if (g_strcmp0 (obj->end_rpmdb_version, las->beg_rpmdb_version)) {
            obj->altered_gt_rpmdb = 1;
            las->altered_lt_rpmdb = 1;
        } else {
            las->altered_lt_rpmdb = 0;
            obj->altered_gt_rpmdb = 0;
        }
    }
}

/**
 * _test_output:
 * @self: SWDB object
 * @tid: transaction ID
 * @o_type: output type string
 *
 * Returns: %TRUE if transaction has output of type @o_type
 **/
static gboolean
_test_output (DnfSwdb *self, gint tid, const gchar *o_type)
{
    gint type = dnf_swdb_get_output_type (self, o_type);
    sqlite3_stmt *res;
    const gchar *sql = T_OUTPUT;
    _db_prepare (self->db, sql, &res);
    _db_bind_int (res, "@tid", tid);
    _db_bind_int (res, "@type", type);
    if (sqlite3_step (res) == SQLITE_ROW) {
        sqlite3_finalize (res);
        return 1;
    }
    sqlite3_finalize (res);
    return 0;
}

/**
 * _resolve_outputs:
 * @self: SWDB object
 * @trans: list of #DnfSwdbTrans
 *
 * Tag if transaction has some output on stdout or stderr
 **/
static void
_resolve_outputs (DnfSwdb *self, GPtrArray *trans)
{
    for (guint i = 0; i < trans->len; ++i) {
        DnfSwdbTrans *obj = (DnfSwdbTrans *)g_ptr_array_index (trans, i);
        obj->is_output = _test_output (self, obj->tid, "stdout");
        obj->is_error = _test_output (self, obj->tid, "stderr");
    }
}

/**
 * dnf_swdb_trans_old:
 * @tids: (element-type gint32)(transfer container): list of transaction IDs
 * @limit: return max. @limit latest transactions, 0 for no limit
 * @complete_only: list only complete transactions
 *
 * Get list of specified transactions
 *
 * Transactions may be specified in @tids
 *
 * If @tids is %NULL or empty last @limit transactions are listed
 *
 * Returns: (element-type DnfSwdbTrans)(array)(transfer container): list of #DnfSwdbTrans
 */
GPtrArray *
dnf_swdb_trans_old (DnfSwdb *self, GArray *tids, gint limit, gboolean complete_only)
{
    if (dnf_swdb_open (self))
        return NULL;
    GPtrArray *node = g_ptr_array_new ();
    sqlite3_stmt *res;
    if (tids && tids->len)
        limit = 0;
    const gchar *sql;
    if (limit && complete_only)
        sql = S_TRANS_COMP_W_LIMIT;
    else if (limit)
        sql = S_TRANS_W_LIMIT;
    else if (complete_only)
        sql = S_TRANS_COMP;
    else
        sql = S_TRANS;
    _db_prepare (self->db, sql, &res);
    if (limit)
        _db_bind_int (res, "@limit", limit);
    gint tid;
    gboolean match = 0;
    while (sqlite3_step (res) == SQLITE_ROW) {
        tid = sqlite3_column_int (res, 0);
        if (tids && tids->len) {
            match = 0;
            for (guint i = 0; i < tids->len; ++i) {
                if (tid == g_array_index (tids, gint, i)) {
                    match = 1;
                    break;
                }
            }
            if (!match)
                continue;
        }
        DnfSwdbTrans *trans =
          dnf_swdb_trans_new (tid,                                   // tid
                              sqlite3_column_int (res, 1),           // beg_t
                              sqlite3_column_int (res, 2),           // end_t
                              (gchar *)sqlite3_column_text (res, 3), // beg_rpmdb_v
                              (gchar *)sqlite3_column_text (res, 4), // end_rpmdb_v
                              (gchar *)sqlite3_column_text (res, 5), // cmdline
                              (gchar *)sqlite3_column_text (res, 6), // loginuid
                              (gchar *)sqlite3_column_text (res, 7), // releasever
                              sqlite3_column_int (res, 8)            // return_code
          );
        trans->swdb = self;
        g_ptr_array_add (node, (gpointer)trans);
    }
    sqlite3_finalize (res);
    if (node->len) {
        _resolve_altered (node);
        _resolve_outputs (self, node);
    }
    return node;
}

/**
 * dnf_swdb_last:
 * @self: SWDB object
 *
 * Get last transaction
 *
 * Returns: (transfer full): last #DnfSwdbTrans
 **/
DnfSwdbTrans *
dnf_swdb_last (DnfSwdb *self, gboolean complete_only)
{
    GPtrArray *node = dnf_swdb_trans_old (self, NULL, 1, complete_only);
    if (!node) {
        return NULL;
    }
    if (!node->len) {
        g_ptr_array_free (node, TRUE);
        return NULL;
    }
    auto trans = static_cast<DnfSwdbTrans *>(g_ptr_array_index(node, 0));
    g_ptr_array_free (node, TRUE);
    return trans;
}

static gboolean
_is_state_installed (const gchar *state)
{
    return !g_strcmp0 (state, "Install") || !g_strcmp0 (state, "Reinstall") ||
           !g_strcmp0 (state, "Update") || !g_strcmp0 (state, "Downgrade");
}

static DnfSwdbReason
_get_erased_reason (sqlite3 *db, gint tid, gint pid)
{
    sqlite3_stmt *res;
    const gchar *sql = S_ERASED_REASON;
    _db_prepare (db, sql, &res);
    _db_bind_int (res, "@tid", tid);
    _db_bind_int (res, "@pid", pid);
    auto reason = static_cast<DnfSwdbReason>(_db_find_int (res));
    return reason != DNF_SWDB_REASON_UNKNOWN ? reason : DNF_SWDB_REASON_USER;
}

/**
 * dnf_swdb_get_erased_reason:
 * @self: SWDB object
 * @nevra: nevra or nvra of package being installed
 * @first_trans: id of first transaction being undone
 * @rollback: %true if transaction is performing a rollback
 *
 * Get package reason before @first_trans. If package altered in the meantime (and we are
 * not performing a rollback) then keep reason from latest installation.
 *
 * Returns: reason ID before transaction being undone
 **/
DnfSwdbReason
dnf_swdb_get_erased_reason (DnfSwdb *self, gchar *nevra, gint first_trans, gboolean rollback)
{
    // prepare database
    if (dnf_swdb_open (self)) {
        // failed to open DB - warning is printed and package is considered user installed
        return DNF_SWDB_REASON_USER;
    }

    // get package object
    g_autoptr (DnfSwdbPkg) pkg = dnf_swdb_package (self, nevra);

    if (!pkg) {
        // package not found - consider it user installed
        return DNF_SWDB_REASON_USER;
    }

    // get package transactions
    GPtrArray *patterns = g_ptr_array_new ();
    g_ptr_array_add (patterns, (gpointer)pkg->name);

    GArray *tids = dnf_swdb_search (self, patterns);

    g_ptr_array_free (patterns, TRUE);

    if (tids->len == 0) {
        // there are no transactions available for this package, consider it user installed
        g_array_free (tids, TRUE);
        return DNF_SWDB_REASON_USER;
    }

    if (!rollback) {
        // check if package was modified since transaction being undone

        // find the latest transaction
        gint max = 0;
        for (guint i = 0; i < tids->len; ++i) {
            gint act = g_array_index (tids, gint, i);
            max = (act > max) ? act : max;
        }

        // resolve package state in the latest transaction
        _resolve_package_state (self, pkg, max);
        if (_is_state_installed (pkg->state)) {
            // package altered since last transaction and is installed in the system
            // keep its reason
            g_array_free (tids, TRUE);
            return _reason_by_pid (self->db, pkg->pid);
        }
    }

    // find last transaction before transaction being undone
    gint last = 0;
    for (guint i = 0; i < tids->len; ++i) {
        gint act = g_array_index (tids, gint, i);
        if (act < first_trans && act > last) {
            // set may not be ordered
            last = act;
        }
    }

    g_array_free (tids, TRUE);

    if (last == 0) {
        // no transaction found - keep it user installed
        return DNF_SWDB_REASON_USER;
    }

    // get pkg reason from that transaction
    return _get_erased_reason (self->db, last, pkg->pid);
}

/****************************** OUTPUT PERSISTOR *****************************/

/**
 * dnf_swdb_log_error:
 * @self: SWDB object
 * @tid: transaction ID
 * @msg: error text
 *
 * Log transaction error output
 *
 * Returns: 0 if successfull
 **/
gint
dnf_swdb_log_error (DnfSwdb *self, gint tid, const gchar *msg)
{
    if (dnf_swdb_open (self))
        return 1;

    _output_insert (self->db, tid, msg, dnf_swdb_get_output_type (self, "stderr"));
    return 0;
}

/**
 * dnf_swdb_log_output:
 * @self: SWDB object
 * @tid: transaction ID
 * @msg: output text
 *
 * Log transaction standard output
 *
 * Returns: 0 if successfull
 **/
gint
dnf_swdb_log_output (DnfSwdb *self, gint tid, const gchar *msg)
{
    if (dnf_swdb_open (self))
        return 1;

    _output_insert (self->db, tid, msg, dnf_swdb_get_output_type (self, "stdout"));
    return 0;
}

/**
 * dnf_swdb_load_error:
 * @self: SWDB object
 * @tid: transaction ID
 *
 * Load error output of transaction @tid
 *
 * Returns: (element-type utf8) (transfer container): list of outputs
 */
GPtrArray *
dnf_swdb_load_error (DnfSwdb *self, gint tid)
{
    if (dnf_swdb_open (self))
        return NULL;
    return _load_output (self->db, tid, dnf_swdb_get_output_type (self, "stderr"));
}

/**
 * dnf_swdb_load_output:
 * @self: SWDB object
 * @tid: transaction ID
 *
 * Load standard output of transaction @tid
 *
 * Returns: (element-type utf8) (transfer container): list of outputs
 */
GPtrArray *
dnf_swdb_load_output (DnfSwdb *self, gint tid)
{
    if (dnf_swdb_open (self))
        return NULL;
    return _load_output (self->db, tid, dnf_swdb_get_output_type (self, "stdout"));
}

/****************************** TYPE BINDERS *********************************/

/**
 * _get_reason_id:
 * @db: database handle
 * @desc: reason description
 * @sql: sql command selecting id with "desc" parameter placeholder
 *
 * Find reason id by its @desc in table used in @sql statement
 *
 * Returns: @desc's id from @sql
 **/
static gint
_get_description_id (sqlite3 *db, const gchar *desc, const gchar *sql)
{
    sqlite3_stmt *res;

    _db_prepare (db, sql, &res);
    _db_bind_str (res, "@desc", desc);
    return _db_find_int (res);
}

/**
 * _add_description:
 * @db: database handle
 * @desc: new description
 * @sql: sql command for inserting description into table with @desc placeholder
 *
 * Insert @desc into table specified in sql and return inserted row id
 *
 * Returns: inserted @desc id in table
 **/
static gint
_add_description (sqlite3 *db, const gchar *desc, const gchar *sql)
{
    sqlite3_stmt *res;
    _db_prepare (db, sql, &res);
    _db_bind_str (res, "@desc", desc);
    _db_step (res);
    return sqlite3_last_insert_rowid (db);
}

/**
 * _get_description:
 * @db: database handle
 * @id: description
 * @sql: sql statement selecting description with placeholder for @id
 *
 * Returns: description for @id
 **/
gchar *
_get_description (sqlite3 *db, gint id, const gchar *sql)
{
    sqlite3_stmt *res;
    _db_prepare (db, sql, &res);
    _db_bind_int (res, "@id", id);
    return _db_find_str (res);
}

/**
 * dnf_swdb_get_output_type:
 * @self: swdb object
 * @type: output type description
 *
 * Returns: output @type description id
 **/
gint
dnf_swdb_get_output_type (DnfSwdb *self, const gchar *type)
{
    if (dnf_swdb_open (self))
        return -1;
    gint id = _get_description_id (self->db, type, S_OUTPUT_TYPE_ID);
    if (!id) {
        id = _add_description (self->db, type, I_DESC_OUTPUT);
    }
    return id;
}

/**
 * dnf_swdb_get_state_type:
 * @self: swdb object
 * @type: state type description
 *
 * Returns: state @type description id
 **/
gint
dnf_swdb_get_state_type (DnfSwdb *self, const gchar *type)
{
    if (dnf_swdb_open (self))
        return -1;
    gint id = _get_description_id (self->db, type, S_STATE_TYPE_ID);
    if (!id) {
        id = _add_description (self->db, type, I_DESC_STATE);
    }
    return id;
}
