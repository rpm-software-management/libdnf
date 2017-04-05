/* dnf-swdb.c
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

#include "dnf-swdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "dnf-swdb-sql.h"
#include "dnf-swdb-db.h"

G_DEFINE_TYPE(DnfSwdb, dnf_swdb, G_TYPE_OBJECT)
G_DEFINE_TYPE(DnfSwdbPkg, dnf_swdb_pkg, G_TYPE_OBJECT) //history package
G_DEFINE_TYPE(DnfSwdbTrans, dnf_swdb_trans, G_TYPE_OBJECT) //transaction
G_DEFINE_TYPE(DnfSwdbTransData, dnf_swdb_transdata, G_TYPE_OBJECT) //trans data
G_DEFINE_TYPE(DnfSwdbPkgData, dnf_swdb_pkgdata, G_TYPE_OBJECT)
G_DEFINE_TYPE(DnfSwdbRpmData, dnf_swdb_rpmdata, G_TYPE_OBJECT)

// SWDB Destructor
static void dnf_swdb_finalize(GObject *object)
{
    DnfSwdb *swdb = (DnfSwdb *) object;
    dnf_swdb_close(swdb);
    g_free(swdb->path);
    g_free(swdb->releasever);
    G_OBJECT_CLASS (dnf_swdb_parent_class)->finalize (object);
}

// SWDB Class initialiser
static void
dnf_swdb_class_init(DnfSwdbClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = dnf_swdb_finalize;
}

// SWDB Object initialiser
static void
dnf_swdb_init(DnfSwdb *self)
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
DnfSwdb* dnf_swdb_new(const gchar* db_path, const gchar* releasever)
{
    DnfSwdb *swdb = g_object_new(DNF_TYPE_SWDB, NULL);
    if (releasever)
    {
        swdb->releasever = g_strdup(releasever);
    }
    if (db_path)
    {
        g_free(swdb->path);
        swdb->path = g_strdup(db_path);
    }
    return swdb;
}


// PKG Destructor
static void dnf_swdb_pkg_finalize(GObject *object)
{
    DnfSwdbPkg *pkg = (DnfSwdbPkg *)object;
    g_free((gchar*) pkg->name);
    g_free((gchar*) pkg->epoch);
    g_free((gchar*) pkg->version);
    g_free((gchar*) pkg->release);
    g_free((gchar*) pkg->arch);
    g_free((gchar*) pkg->checksum_data);
    g_free((gchar*) pkg->checksum_type);
    g_free((gchar*) pkg->type);
    g_free(pkg->state);
    g_free(pkg->ui_from_repo);
    g_free(pkg->nvra);
    G_OBJECT_CLASS (dnf_swdb_pkg_parent_class)->finalize (object);
}

// PKG Class initialiser
static void
dnf_swdb_pkg_class_init(DnfSwdbPkgClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = dnf_swdb_pkg_finalize;
}

// PKG Object initialiser
static void
dnf_swdb_pkg_init(DnfSwdbPkg *self)
{
    self->done = 0;
    self->state = NULL;
    self->pid = 0;
    self->ui_from_repo = NULL;
    self->swdb = NULL;
    self->nvra = NULL;
}

/**
 * dnf_swdb_pkg_new:
 *
 * Creates a new #DnfSwdbPkg.
 *
 * Returns: a #DnfSwdbPkg
 **/
DnfSwdbPkg* dnf_swdb_pkg_new(const gchar* name,
                             const gchar* epoch,
                             const gchar* version,
                             const gchar* release,
                             const gchar* arch,
                             const gchar* checksum_data,
                             const gchar* checksum_type,
                             const gchar* type)
{
    DnfSwdbPkg *swdbpkg = g_object_new(DNF_TYPE_SWDB_PKG, NULL);
    swdbpkg->name = g_strdup(name);
    swdbpkg->epoch = g_strdup(epoch);
    swdbpkg->version = g_strdup(version);
    swdbpkg->release = g_strdup(release);
    swdbpkg->arch = g_strdup(arch);
    swdbpkg->checksum_data = g_strdup(checksum_data);
    swdbpkg->checksum_type = g_strdup(checksum_type);
    gchar *tmp = g_strjoin("-", swdbpkg->name, swdbpkg->version,
                           swdbpkg->release, NULL);
    swdbpkg->nvra = g_strjoin(".", tmp, swdbpkg->arch, NULL);
    g_free(tmp);
    if (type) swdbpkg->type = g_strdup(type); else swdbpkg->type = NULL;
    return swdbpkg;
}

// PKG DATA Destructor
static void dnf_swdb_pkgdata_finalize(GObject *object)
{
    DnfSwdbPkgData * pkgdata = (DnfSwdbPkgData*) object;
    g_free( pkgdata->from_repo);
    g_free( pkgdata->from_repo_revision);
    g_free( pkgdata->from_repo_timestamp);
    g_free( pkgdata->installed_by);
    g_free( pkgdata->changed_by);
    g_free( pkgdata->installonly);
    g_free( pkgdata->origin_url);
    G_OBJECT_CLASS (dnf_swdb_pkgdata_parent_class)->finalize (object);
}

// PKG DATA Class initialiser
static void
dnf_swdb_pkgdata_class_init(DnfSwdbPkgDataClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = dnf_swdb_pkgdata_finalize;
}

// PKG DATA Object initialiser
static void
dnf_swdb_pkgdata_init(DnfSwdbPkgData *self)
{
    self->from_repo = NULL;
    self->from_repo_revision = NULL;
    self->from_repo_timestamp = NULL;
    self->installed_by = NULL;
    self->changed_by = NULL;
    self->installonly = NULL;
    self->origin_url = NULL;
}

/**
 * dnf_swdb_pkgdata_new:
 *
 * Creates a new #DnfSwdbPkgData.
 *
 * Returns: a #DnfSwdbPkgData
 **/
DnfSwdbPkgData *dnf_swdb_pkgdata_new(const gchar* from_repo_revision,
                                     const gchar* from_repo_timestamp,
                                     const gchar* installed_by,
                                     const gchar* changed_by,
                                     const gchar* installonly,
                                     const gchar* origin_url,
                                     const gchar* from_repo)
{
    DnfSwdbPkgData *pkgdata = g_object_new(DNF_TYPE_SWDB_PKGDATA, NULL);
    pkgdata->from_repo_revision = g_strdup(from_repo_revision);
    pkgdata->from_repo_timestamp = g_strdup(from_repo_timestamp);
    pkgdata->installed_by = g_strdup(installed_by);
    pkgdata->changed_by = g_strdup(changed_by);
    pkgdata->installonly = g_strdup(installonly);
    pkgdata->origin_url = g_strdup(origin_url);
    pkgdata->from_repo = g_strdup(from_repo);
    return pkgdata;
}

// TRANS Destructor
static void dnf_swdb_trans_finalize(GObject *object)
{
    DnfSwdbTrans *trans = (DnfSwdbTrans *) object;
    g_free( (gchar*) trans->beg_timestamp);
    g_free( (gchar*) trans->end_timestamp);
    g_free( (gchar*) trans->beg_rpmdb_version);
    g_free( (gchar*) trans->end_rpmdb_version);
    g_free( (gchar*) trans->cmdline);
    g_free( (gchar*) trans->loginuid);
    g_free( (gchar*) trans->releasever);
    G_OBJECT_CLASS (dnf_swdb_trans_parent_class)->finalize (object);
}

// TRANS Class initialiser
static void
dnf_swdb_trans_class_init(DnfSwdbTransClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = dnf_swdb_trans_finalize;
}

// TRANS Object initialiser
static void
dnf_swdb_trans_init(DnfSwdbTrans *self)
{
    self->is_output = 0;
    self->is_error = 0;
    self->swdb = NULL;
    self->altered_lt_rpmdb = 0;
    self->altered_gt_rpmdb = 0;
}

/**
 * dnf_swdb_trans_new:
 *
 * Creates a new #DnfSwdbTrans.
 *
 * Returns: a #DnfSwdbTrans
**/
DnfSwdbTrans* dnf_swdb_trans_new(gint tid,
                                 const gchar *beg_timestamp,
                                 const gchar *end_timestamp,
                                 const gchar *beg_rpmdb_version,
                                 const gchar *end_rpmdb_version,
                                 const gchar *cmdline,
                                 const gchar *loginuid,
                                 const gchar *releasever,
                                 gint return_code)
{
    DnfSwdbTrans *trans = g_object_new(DNF_TYPE_SWDB_TRANS, NULL);
    trans->tid = tid;
    trans->beg_timestamp = g_strdup(beg_timestamp);
    trans->end_timestamp = g_strdup(end_timestamp);
    trans->beg_rpmdb_version = g_strdup(beg_rpmdb_version);
    trans->end_rpmdb_version = g_strdup(end_rpmdb_version);
    trans->cmdline = g_strdup(cmdline);
    trans->loginuid = g_strdup(loginuid);
    trans->releasever = g_strdup(releasever);
    trans->return_code = return_code;
    return trans;
}

//TRANS DATA object
// TRANS DATA Destructor
static void dnf_swdb_transdata_finalize(GObject *object)
{
    DnfSwdbTransData * tdata = (DnfSwdbTransData*) object;
    g_free( tdata->reason);
    g_free( tdata->state);
    G_OBJECT_CLASS (dnf_swdb_transdata_parent_class)->finalize (object);
}

// TRANS DATA Class initialiser
static void
dnf_swdb_transdata_class_init(DnfSwdbTransDataClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = dnf_swdb_transdata_finalize;
}

// TRANS DATA Object initialiser
static void
dnf_swdb_transdata_init(DnfSwdbTransData *self)
{
}

/**
 * dnf_swdb_transdata_new:
 *
 * Creates a new #DnfSwdbTransData.
 *
 * Returns: a #DnfSwdbTransData
**/
DnfSwdbTransData* dnf_swdb_transdata_new(gint tdid,
                                         gint tid,
                                         gint pdid,
                                         gint tgid,
                                         gint done,
                                         gint ORIGINAL_TD_ID,
                                         gchar *reason,
                                         gchar *state)
{
    DnfSwdbTransData *data = g_object_new(DNF_TYPE_SWDB_TRANSDATA, NULL);
    data->tdid = tdid;
    data->tid = tid;
    data->pdid = pdid;
    data->tgid = tgid;
    data->done = done;
    data->ORIGINAL_TD_ID = ORIGINAL_TD_ID;
    data->reason = g_strdup(reason);
    data->state = g_strdup(state);
    return data;
}

// rpmdata destructor
static void dnf_swdb_rpmdata_finalize(GObject *object)
{
    DnfSwdbRpmData *rpmdata = (DnfSwdbRpmData *) object;
    g_free((gchar *) rpmdata->buildtime);
    g_free((gchar *) rpmdata->buildhost);
    g_free((gchar *) rpmdata->license);
    g_free((gchar *) rpmdata->packager);
    g_free((gchar *) rpmdata->size);
    g_free((gchar *) rpmdata->sourcerpm);
    g_free((gchar *) rpmdata->url);
    g_free((gchar *) rpmdata->vendor);
    g_free((gchar *) rpmdata->committer);
    g_free((gchar *) rpmdata->committime);
    G_OBJECT_CLASS (dnf_swdb_rpmdata_parent_class)->finalize (object);
}

// rpmdata Class initialiser
static void dnf_swdb_rpmdata_class_init(DnfSwdbRpmDataClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = dnf_swdb_rpmdata_finalize;
}

// rpmdata Object initialiser
static void dnf_swdb_rpmdata_init(DnfSwdbRpmData *self)
{
    self->pid = 0;
    self->buildtime = NULL;
    self->buildhost = NULL;
    self->license = NULL;
    self->packager = NULL;
    self->size = NULL;
    self->sourcerpm = NULL;
    self->url = NULL;
    self->vendor = NULL;
    self->committer = NULL;
    self->committime = NULL;
}

/**
 * dnf_swdb_rpmdata_new:
 *
 * Creates a new #DnfSwdbRpmData.
 *
 * Returns: a #DnfSwdbRpmData
**/
DnfSwdbRpmData *dnf_swdb_rpmdata_new(gint pid,
                                     const gchar *buildtime,
                                     const gchar *buildhost,
                                     const gchar *license,
                                     const gchar *packager,
                                     const gchar *size,
                                     const gchar *sourcerpm,
                                     const gchar *url,
                                     const gchar *vendor,
                                     const gchar *committer,
                                     const gchar *committime)
{
    DnfSwdbRpmData *rpmdata = g_object_new(DNF_TYPE_SWDB_RPMDATA, NULL);
    rpmdata->pid = pid;
    rpmdata->buildtime = g_strdup(buildtime);
    rpmdata->buildhost = g_strdup(buildhost);
    rpmdata->license = g_strdup(license);
    rpmdata->packager = g_strdup(packager);
    rpmdata->size = g_strdup(size);
    rpmdata->sourcerpm = g_strdup(sourcerpm);
    rpmdata->url = g_strdup(url);
    rpmdata->vendor = g_strdup(vendor);
    rpmdata->committer = g_strdup(committer);
    rpmdata->committime = g_strdup(committime);
    return rpmdata;
}

/**
* dnf_swdb_search:
* @patterns: (element-type utf8) (transfer container): list of constants
* Returns: (element-type gint32) (transfer container): list of constants
*/
GArray *dnf_swdb_search(DnfSwdb *self, GPtrArray *patterns)
{
    if (dnf_swdb_open(self))
        return NULL;

    GArray *tids = g_array_new(0, 0, sizeof(gint));
    for(guint i = 0; i < patterns-> len; ++i)
    {
        gchar *pattern = g_ptr_array_index(patterns, i);

        //try simple search
        GArray *simple =  _simple_search(self->db, pattern);
        if(simple->len)
        {
            tids = g_array_append_vals(tids, simple->data, simple->len);
            g_array_free(simple, TRUE);
            continue;
        }
        g_array_free(simple, TRUE);
        //need for extended search
        GArray *extended = _extended_search(self->db, pattern);
        if(extended)
        {
            tids = g_array_append_vals(tids, extended->data, extended->len);
        }
        g_array_free(extended, TRUE);
    }
    return tids;
}

static gint _pid_by_nvra(sqlite3 *db, const gchar *nvra)
{
    gint pid = 0;
    const gchar *sql = S_PID_BY_NVRA;
    sqlite3_stmt *res;
    DB_PREP(db, sql, res);
    DB_BIND(res, "@nvra", nvra);
    pid = DB_FIND(res);
    return pid;
}

/**
* dnf_swdb_checksums_by_nvras:
* @nvras: (element-type utf8) (transfer container): list of constants
* Returns: (element-type utf8) (transfer container): list of constants
*/
GPtrArray * dnf_swdb_checksums_by_nvras(DnfSwdb *self, GPtrArray *nvras)
{
    if (dnf_swdb_open(self))
        return NULL;
    gchar *buff1;
    gchar *buff2;
    gchar *nvra;
    GPtrArray *checksums = g_ptr_array_new();
    const gchar *sql = S_CHECKSUMS_BY_NVRA;
    for(guint i = 0; i < nvras->len; i++)
    {
        nvra = (gchar *)g_ptr_array_index(nvras, i);
        sqlite3_stmt *res;
        DB_PREP(self->db, sql, res);
        DB_BIND(res, "@nvra", nvra);
        if(sqlite3_step(res) == SQLITE_ROW)
        {
            buff1 = (gchar *)sqlite3_column_text(res, 0);
            buff2 = (gchar *)sqlite3_column_text(res, 1);
            if(buff1 && buff2)
            {
                g_ptr_array_add(checksums, (gpointer)g_strdup(buff2)); //type
                g_ptr_array_add(checksums, (gpointer)g_strdup(buff1)); //data
            }
        }
        sqlite3_finalize(res);
    }
    return checksums;
}


/**
 * dnf_swdb_select_user_installed:
 * @nvras: (element-type utf8)(transfer container): list of constants
 * Returns: (element-type gint32)(transfer container): list of constants
 *  - id`s of nvras, which are user installed
 *  - e.g. when there is ["abc", "def", "ghi"] on input and "abc" and "ghi" are
 *      user installed, output will be [0,2]
**/
GArray *dnf_swdb_select_user_installed(DnfSwdb *self, GPtrArray *nvras)
{
    if (dnf_swdb_open(self))
        return NULL;

    gint depid = _get_description_id(self->db, "dep", S_REASON_TYPE_ID);
    GArray *usr_ids = g_array_new(0,0,sizeof(gint));
    if (!depid)
    {
        //there are no dependency pacakges - all packages are user installed
        for(guint i = 0; i < nvras->len; ++i)
            g_array_append_val(usr_ids, i);

        return usr_ids;
    }

    gint pid = 0;
    gint pdid = 0;
    gint reason_id = 0;
    const gchar *sql = S_REASON_ID_BY_PDID;
    for( guint i = 0; i < nvras->len; ++i)
    {
        pid = _pid_by_nvra(self->db, (gchar *)g_ptr_array_index(nvras, i));
        if(!pid)
        {
            //better not uninstall package when not sure
            g_array_append_val(usr_ids, i);
            continue;
        }
        pdid = _pdid_from_pid(self->db, pid);
        if(!pdid)
        {
            g_array_append_val(usr_ids, i);
            continue;
        }
        sqlite3_stmt *res;
        DB_PREP(self->db, sql, res);
        DB_BIND_INT(res, "@pdid", pdid);
        while( sqlite3_step(res) == SQLITE_ROW)
        {
            reason_id = sqlite3_column_int(res, 0);
            if (reason_id != depid)
            {
                g_array_append_val(usr_ids, i);
                break;
            }
        }
        sqlite3_finalize(res);
    }
    return usr_ids;
}

gboolean dnf_swdb_user_installed(DnfSwdb *self, const gchar *nvra)
{
    if (dnf_swdb_open(self))
        return TRUE;

    gint pid = _pid_by_nvra(self->db, nvra);
    if(!pid)
    {
        return FALSE;
    }
    gboolean rc = TRUE;
    gchar *repo = _repo_by_pid(self->db, pid);
    gchar *reason = _reason_by_pid(self->db, pid);
    if(g_strcmp0("user", reason) || !g_strcmp0("anakonda", repo))
        rc = FALSE;
    g_free(repo);
    g_free(reason);
    return rc;
}

/***************************** REPO PERSISTOR ********************************/

static gint _bind_repo_by_name (sqlite3 *db, const gchar *name)
{
    sqlite3_stmt *res;
    const gchar *sql = FIND_REPO_BY_NAME;
    DB_PREP(db, sql, res);
    DB_BIND(res, "@name", name);
    gint rc = DB_FIND(res);
    if (rc)
    {
        return rc;
    }
    else //not found, need to insert
    {
    sql = INSERT_REPO;
    DB_PREP(db, sql, res);
    DB_BIND(res, "@name", name);
    DB_STEP(res);
    return sqlite3_last_insert_rowid(db);
    }
}

gchar* _repo_by_rid(sqlite3 *db, gint rid)
{
    sqlite3_stmt *res;
    const gchar *sql = S_REPO_BY_RID;
    DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@rid", rid);
    return DB_FIND_STR(res);
}

gchar *_repo_by_pid  (   sqlite3 *db,
                        gint pid)
{
    sqlite3_stmt *res;
    const gchar *sql = S_REPO_FROM_PID2;
    DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@pid", pid);
    return DB_FIND_STR(res);
}

gchar *dnf_swdb_repo_by_nvra(DnfSwdb *self, const gchar *nvra)
{
    if (dnf_swdb_open(self))
        return NULL;
    gint pid = _pid_by_nvra(self->db, nvra);
    if(!pid)
    {
        return g_strdup("unknown");
    }
    gchar *r_name = _repo_by_pid(self->db, pid);
    return r_name;
}

gint dnf_swdb_set_repo(DnfSwdb *self, const gchar *nvra, const gchar *repo)
{
    if (dnf_swdb_open(self))
        return 1;
    gint pid = _pid_by_nvra(self->db, nvra);
    if (!pid)
    {
        return 1;
    }
    gint rid = _bind_repo_by_name(self->db, repo);
    sqlite3_stmt *res;
    const gchar *sql = U_REPO_BY_PID;
    DB_PREP(self->db, sql, res);
    DB_BIND_INT(res, "@rid", rid);
    DB_BIND_INT(res, "@pid", pid);
    DB_STEP(res);
    return 0;
}

/**************************** PACKAGE PERSISTOR ******************************/

static gint _package_insert(DnfSwdb *self, DnfSwdbPkg *package)
{
    gint type_id = dnf_swdb_get_package_type(self,package->type);
    sqlite3_stmt *res;
    const gchar *sql = INSERT_PKG;
    DB_PREP(self->db,sql,res);
    DB_BIND(res, "@name", package->name);
    DB_BIND(res, "@epoch", package->epoch);
    DB_BIND(res, "@version", package->version);
    DB_BIND(res, "@release", package->release);
    DB_BIND(res, "@arch", package->arch);
    DB_BIND(res, "@cdata", package->checksum_data);
    DB_BIND(res, "@ctype", package->checksum_type);
    DB_BIND_INT(res, "@type", type_id);
    DB_STEP(res);
    package->pid =  sqlite3_last_insert_rowid(self->db);
    return package->pid;
}

gint dnf_swdb_add_package(DnfSwdb *self, DnfSwdbPkg *pkg)
{
    if (dnf_swdb_open(self))
        return 1;
    gint rc = _package_insert(self, pkg);
    return rc;
    }

static gint _package_data_update(sqlite3 *db, gint pid, DnfSwdbPkgData *pkgdata)
{
    gint rid = _bind_repo_by_name(db, pkgdata->from_repo);

    //look for old pid's package data - # = origin_url stands for autogenerated
    sqlite3_stmt *res;
    const gchar *sql = S_PREV_AUTO_PD;
    DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@pid", pid);
    gint pdid = DB_FIND(res);
    if (!pdid) //autogen field not avalible, we need to insert
        sql = INSERT_PKG_DATA;
    else //there is some autogen row, we can just add values
        sql = UPDATE_PKG_DATA;
    DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@pid", pid);
    DB_BIND_INT(res, "@rid", rid);
    DB_BIND(res, "@repo_r", pkgdata->from_repo_revision);
    DB_BIND(res, "@repo_t", pkgdata->from_repo_timestamp);
    DB_BIND(res, "@installed_by", pkgdata->installed_by);
    DB_BIND(res, "@changed_by", pkgdata->changed_by);
    DB_BIND(res, "@installonly", pkgdata->installonly);
    DB_BIND(res, "@origin_url", pkgdata->origin_url);
    DB_STEP(res);
    return 0;
}

gint dnf_swdb_log_package_data(DnfSwdb *self,
                               gint pid,
                               DnfSwdbPkgData *pkgdata )
{
    if (dnf_swdb_open(self))
        return 1;
    return _package_data_update(self->db, pid, pkgdata);
}

gint _pdid_from_pid(sqlite3 *db, gint pid)
{
    sqlite3_stmt *res;
    const gchar *sql = FIND_PDID_FROM_PID;
    DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@pid", pid);
    gint rc = DB_FIND(res);
    if (rc)
        return rc;
    const gchar *sql_insert = INSERT_PDID;
    DB_PREP(db, sql_insert, res);
    DB_BIND_INT(res, "@pid", pid);
    DB_STEP(res);
    return sqlite3_last_insert_rowid(db);
}

GArray* _all_pdid_for_pid(sqlite3 *db, gint pid)
{
    GArray *pdids = g_array_new(0,0,sizeof(gint));
    sqlite3_stmt *res;
    const gchar *sql = FIND_ALL_PDID_FOR_PID;
    DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@pid", pid);
    gint pdid;
    while( (pdid = DB_FIND_MULTI(res)))
    {
        g_array_append_val(pdids, pdid);
    }
    return pdids;
}

GArray * _tids_from_pdid(sqlite3 *db, gint pdid)
{
    sqlite3_stmt *res;
    GArray *tids = g_array_new(0,0,sizeof(gint));
    const gchar *sql = FIND_TIDS_FROM_PDID;
    DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@pdid", pdid);
    gint tid = 0;
    while(sqlite3_step(res) == SQLITE_ROW)
    {
        tid = sqlite3_column_int(res, 0);
        g_array_append_val(tids, tid);
    }
    sqlite3_finalize(res);
    return tids;
}

static GArray *_pids_by_tid(sqlite3 *db, gint tid)
{
    GArray *pids = g_array_new(0, 0, sizeof(gint));
    sqlite3_stmt *res;
    const gchar* sql = PID_BY_TID;
    DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@tid", tid);
    gint pid;
    while ( (pid = DB_FIND_MULTI(res)))
    {
        g_array_append_val(pids, pid);
    }
    return pids;
}

gchar *_reason_by_pid(sqlite3 *db, gint pid)
{
    gchar *reason = NULL;
    sqlite3_stmt *res;
    const gchar *sql = S_REASON_BY_PID;
    DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@pid", pid);
    gint reason_id = DB_FIND(res);
    if (reason_id)
    {
        reason = _get_description(db, reason_id, S_REASON_TYPE_BY_ID);
    }
    return reason;
}

gchar *dnf_swdb_reason_by_nvra(DnfSwdb *self, const gchar *nvra)
{
    if (dnf_swdb_open(self))
        return NULL;
    gint pid = _pid_by_nvra(self->db, nvra);
    gchar *reason = NULL;
    if(pid)
    {
        reason = _reason_by_pid(self->db, pid);
    }
    return reason;
}

gchar *dnf_swdb_pkg_get_reason(DnfSwdbPkg *self)
{
    if (!self || dnf_swdb_open(self->swdb))
        return NULL;
    gchar *reason = NULL;
    if(self->pid)
    {
        reason = _reason_by_pid(self->swdb->db, self->pid);
    }
    return reason;
}

static void _resolve_package_state(DnfSwdb *self, DnfSwdbPkg *pkg, gint tid)
{
    sqlite3_stmt *res;
    const gchar *sql = S_PACKAGE_STATE;
    DB_PREP(self->db, sql, res);
    DB_BIND_INT(res, "@pid", pkg->pid);
    DB_BIND_INT(res, "@tid", tid);
    if (sqlite3_step(res) == SQLITE_ROW)
    {
        pkg->done = sqlite3_column_int(res, 0);
        gint state_code = sqlite3_column_int(res, 1);
        sqlite3_finalize(res);
        pkg->state = _get_description(self->db, state_code, S_STATE_TYPE_BY_ID);
    }
    else
    {
        sqlite3_finalize(res);
    }
}

static DnfSwdbPkgData *_get_package_data_by_pid (sqlite3 *db, gint pid)
{
    sqlite3_stmt *res;
    const gchar *sql = S_PACKAGE_DATA_BY_PID;
    DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@pid", pid);
    if (sqlite3_step(res) == SQLITE_ROW)
    {
        DnfSwdbPkgData *pkgdata = dnf_swdb_pkgdata_new(
            (gchar *)sqlite3_column_text(res, 3), //from_repo_revision
            (gchar *)sqlite3_column_text(res, 4), //from_repo_timestamp
            (gchar *)sqlite3_column_text(res, 5), //installed_by
            (gchar *)sqlite3_column_text(res, 6), //changed_by
            (gchar *)sqlite3_column_text(res, 7), //installonly
            (gchar *)sqlite3_column_text(res, 8), //origin_url
            NULL //from_repo
        );
        pkgdata->pdid = sqlite3_column_int(res, 0); //pdid
        pkgdata->pid = pid;
        gint rid = sqlite3_column_int(res, 2);
        sqlite3_finalize(res);
        pkgdata->from_repo = _repo_by_rid(db, rid); //from repo
        return pkgdata;
    }
    sqlite3_finalize(res);
    return NULL;
}

static DnfSwdbPkg *_get_package_by_pid(sqlite3 *db, gint pid)
{
    sqlite3_stmt *res;
    const gchar *sql = S_PACKAGE_BY_PID;
    DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@pid", pid);
    if (sqlite3_step(res) == SQLITE_ROW)
    {
        DnfSwdbPkg *pkg = dnf_swdb_pkg_new(
            (gchar *)sqlite3_column_text(res, 1), //name
            (gchar *)sqlite3_column_text(res, 2), //epoch
            (gchar *)sqlite3_column_text(res, 3), //version
            (gchar *)sqlite3_column_text(res, 4), //release
            (gchar *)sqlite3_column_text(res, 5), //arch
            (gchar *)sqlite3_column_text(res, 6), //checksum_data
            (gchar *)sqlite3_column_text(res, 7), //checksum_type
            NULL //type for now
        );
        gint type = sqlite3_column_int(res, 8); //unresolved type
        pkg->pid = pid;
        sqlite3_finalize(res);
        pkg->type = _get_description(db, type, S_PACKAGE_TYPE_BY_ID);
        return pkg;
    }
    sqlite3_finalize(res);
    return NULL;
}

/**
* dnf_swdb_get_packages_by_tid:
*
* Returns: (element-type DnfSwdbPkg)(array)(transfer container): list of #DnfSwdbPkg
*/
GPtrArray *dnf_swdb_get_packages_by_tid(DnfSwdb *self, gint tid)
{
    if (dnf_swdb_open(self))
        return NULL;

    GPtrArray *node = g_ptr_array_new();
    GArray *pids = _pids_by_tid(self->db, tid);
    gint pid;
    DnfSwdbPkg *pkg;
    for(guint i = 0; i < pids->len ;i++)
    {
        pid = g_array_index(pids, gint, i);
        pkg = _get_package_by_pid(self->db, pid);
        if(pkg)
        {
            pkg->swdb = self;
            _resolve_package_state(self, pkg, tid);
            g_ptr_array_add(node, (gpointer) pkg);
        }
    }
    g_array_free(pids, TRUE);
    return node;
}

gchar* dnf_swdb_pkg_get_ui_from_repo(DnfSwdbPkg *self)
{
    if(self->ui_from_repo)
        return g_strdup(self->ui_from_repo);
    if(!self->swdb || !self->pid || dnf_swdb_open(self->swdb))
        return g_strdup("(unknown)");
    sqlite3_stmt *res;
    const gchar *sql = S_REPO_FROM_PID;
    gint pdid = 0;
    DB_PREP(self->swdb->db, sql, res);
    DB_BIND_INT(res, "@pid", self->pid);
    gchar *r_name = NULL;
    if(sqlite3_step(res) == SQLITE_ROW)
    {
        r_name = g_strdup((const gchar*)sqlite3_column_text(res, 0));
        pdid = sqlite3_column_int(res, 1);
    }
    sqlite3_finalize(res);

    //now we find out if package wasnt installed from some other releasever
    if (pdid && self->swdb->releasever)
    {
        gchar *cur_releasever = NULL;;
        sql = S_RELEASEVER_FROM_PDID;
        DB_PREP(self->swdb->db, sql, res);
        DB_BIND_INT(res, "@pdid", pdid);
        if(sqlite3_step(res) == SQLITE_ROW)
        {
            cur_releasever = g_strdup((const gchar*)sqlite3_column_text(res, 0));
        }
        sqlite3_finalize(res);
        if(cur_releasever && g_strcmp0(cur_releasever, self->swdb->releasever))
        {
            gchar *rc = g_strjoin(NULL, "@", r_name, "/", cur_releasever, NULL);
            g_free(r_name);
            g_free(cur_releasever);
            self->ui_from_repo = rc;
            return g_strdup(rc);
        }
        g_free(cur_releasever);
    }
    if(r_name)
    {
        self->ui_from_repo = r_name;
        return g_strdup(r_name);
    }
    return g_strdup("(unknown)");
}

/**
 * dnf_swdb_pid_by_nvra:
 * Returns P_ID for package with given nvra or 0
 */
gint dnf_swdb_pid_by_nvra(DnfSwdb *self,
                          const gchar *nvra)
{
    if (dnf_swdb_open(self))
        return 0;
    gint pid = _pid_by_nvra(self->db, nvra);
    return pid;
}

/**
* dnf_swdb_package_by_nvra:
* Returns: (transfer full): #DnfSwdbPkg
*/
DnfSwdbPkg *dnf_swdb_package_by_nvra(DnfSwdb *self, const gchar *nvra)
{
    if (dnf_swdb_open(self))
        return NULL;
    gint pid = _pid_by_nvra(self->db, nvra);
    if (!pid)
    {
        return NULL;
    }
    DnfSwdbPkg *pkg = _get_package_by_pid(self->db, pid);
    pkg->swdb = self;
    return pkg;
}

/**
* dnf_swdb_package_data_by_nvra:
* Returns: (transfer full): #DnfSwdbPkgData
*/
DnfSwdbPkgData *dnf_swdb_package_data_by_nvra(DnfSwdb *self, const gchar *nvra)
{
    if (dnf_swdb_open(self))
        return NULL;
    gint pid = _pid_by_nvra(self->db, nvra);
    if (!pid)
    {
        return NULL;
    }
    DnfSwdbPkgData *pkgdata = _get_package_data_by_pid(self->db, pid);
    return pkgdata;
}

static gint _mark_pkg_as(sqlite3 *db, gint pdid, gint mark)
{
    sqlite3_stmt *res;
    const gchar *sql = U_REASON_BY_PDID;
    DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@reason", mark);
    DB_BIND_INT(res, "@pdid", pdid);
    DB_STEP(res);
    return 0;
}

gint dnf_swdb_set_reason(DnfSwdb *self, const gchar *nvra, const gchar *reason)
{
    if (dnf_swdb_open(self))
        return 1;
    gint pid = _pid_by_nvra(self->db, nvra);
    if (!pid)
    {
        return 1;
    }
    gint pdid = _pdid_from_pid(self->db, pid);
    gint rc = 1;
    gint reason_id;
    reason_id = dnf_swdb_get_reason_type( self, reason);
    rc = _mark_pkg_as(self->db, pdid, reason_id);
    return rc;
}

gint dnf_swdb_mark_user_installed(DnfSwdb *self,
                                  const gchar *pattern,
                                  gboolean user_installed)
{
    gint rc;
    if(user_installed)
    {
        rc = dnf_swdb_set_reason(self, pattern, "user");
    }
    else
    {
        rc = dnf_swdb_set_reason(self, pattern, "dep");
    }
    return rc;
}

/**************************** RPM DATA PERSISTOR *****************************/

static gint _rpm_data_insert (sqlite3 *db, DnfSwdbRpmData *rpm_data)
{
    sqlite3_stmt *res;
    const gchar *sql = INSERT_RPM_DATA;
    DB_PREP(db,sql,res);
    DB_BIND_INT(res, "@pid", rpm_data->pid);
    DB_BIND(res, "@buildtime", rpm_data->buildtime);
    DB_BIND(res, "@buildhost", rpm_data->buildhost);
    DB_BIND(res, "@license", rpm_data->license);
    DB_BIND(res, "@packager", rpm_data->packager);
    DB_BIND(res, "@size", rpm_data->size);
    DB_BIND(res, "@sourcerpm", rpm_data->sourcerpm);
    DB_BIND(res, "@url", rpm_data->url);
    DB_BIND(res, "@vendor", rpm_data->vendor);
    DB_BIND(res, "@committer", rpm_data->committer);
    DB_BIND(res, "@committime", rpm_data->committime);
    DB_STEP(res);
    return 0;
}

gint dnf_swdb_add_rpm_data(DnfSwdb *self, DnfSwdbRpmData *rpm_data)
{
    if (dnf_swdb_open(self))
        return 1;
    gint rc = _rpm_data_insert(self->db, rpm_data);
    return rc;
}


/*************************** TRANS DATA PERSISTOR ****************************/

static gint _trans_data_beg_insert(sqlite3 *db,
                                   gint tid,
                                   gint pdid,
                                   gint tgid,
                                   gint reason,
                                   gint state)
{
    sqlite3_stmt *res;
    const gchar *sql = INSERT_TRANS_DATA_BEG;
    DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@tid", tid);
    DB_BIND_INT(res, "@pdid", pdid);
    DB_BIND_INT(res, "@tgid", tgid);
    DB_BIND_INT(res, "@reason", reason);
    DB_BIND_INT(res, "@state", state);
    return DB_FIND(res);
}

static gint _resolve_group_origin(sqlite3 *db, gint tid, gint pid)
{
    sqlite3_stmt *res;
    const gchar *sql = RESOLVE_GROUP_TRANS;
    DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@pid", pid);
    DB_BIND_INT(res, "@tid", tid);
    return DB_FIND(res);
}

gint dnf_swdb_trans_data_beg(DnfSwdb *self,
                             gint tid,
                             gint pid,
                             const gchar *reason,
                             const gchar *state)
{
    if (dnf_swdb_open(self))
        return 1;
    gint pdid = _pdid_from_pid(self->db, pid);
    gint tgid = 0;
    if(!g_strcmp0(reason, "group"))
    {
        tgid = _resolve_group_origin(self->db, tid, pid);
    }
    gint rc = _trans_data_beg_insert(
        self->db,
        tid,
        pdid,
        tgid,
        dnf_swdb_get_reason_type(self, reason),
        dnf_swdb_get_state_type(self, state)
    );
    return rc;
}

/*
* Mark single package from transaction as done
* Resolve ORIGINAL_TD_ID if any
*/
gint dnf_swdb_trans_data_pid_end(DnfSwdb *self,
                                 gint pid,
                                 gint tid,
                                 const gchar *state)
{
    if (dnf_swdb_open(self))
        return 1;

    sqlite3_stmt *res;
    const gchar *p_sql = S_PDID_TDID_BY_PID;
    DB_PREP(self->db, p_sql, res);
    DB_BIND_INT(res, "@pid", pid);
    gint pdid = 0;
    gint tdid = 0;
    if(sqlite3_step(res) == SQLITE_ROW)
    {
        pdid = sqlite3_column_int(res, 0);
        tdid = sqlite3_column_int(res, 1);
    }
    sqlite3_finalize(res);

    gint _state = dnf_swdb_get_state_type(self, state);

    const gchar *sql = UPDATE_TRANS_DATA_PID_END;
    DB_PREP(self->db, sql, res);
    DB_BIND_INT(res, "@done", 1);
    DB_BIND_INT(res, "@tid", tid);
    DB_BIND_INT(res, "@pdid", pdid);
    DB_BIND_INT(res, "@state", _state);
    DB_STEP(res);

    // resolve ORIGINAL_TD_ID
    // this will be little bit problematic
    if (g_strcmp0("Install", state) &&
        g_strcmp0("Updated", state) &&
        g_strcmp0("Reinstalled", state) &&
        g_strcmp0("Obsoleted", state) &&
        // FIXME we cant tell which package was obsoleted for now
        g_strcmp0("Obsolete", state)
    ) //other than these
    {
        /* get installed package name *sigh* - in future we would want to call
         * this directly with unified DnfSwdbPkg object, so we could skip this
         * and there would be no pkg2pid method in DNF XXX
         * Also, by this method, we cant tell if package was obsoleted
        */

        const gchar * n_sql = S_NAME_BY_PID;
        DB_PREP(self->db, n_sql, res);
        DB_BIND_INT(res, "@pid", pid);
        gchar * name = DB_FIND_STR(res);
        if (name)
        {
            // find last package with same name in database
            const gchar * t_sql;
            if(!g_strcmp0("Update", state))
            {
                t_sql = S_LAST_TDID_BY_NAME;
                DB_PREP(self->db, t_sql, res);
                DB_BIND_INT(res, "@pid", pid);
            }
            else
            {
                t_sql = S_LAST_W_TDID_BY_NAME;
                DB_PREP(self->db, t_sql, res);
            }
            DB_BIND_INT(res, "@tid", tid);
            DB_BIND(res, "@name", name);
            gint orig = DB_FIND(res);
            if(orig)
            {
                const gchar * s_sql = U_ORIGINAL_TDID_BY_TDID;
                DB_PREP(self->db, s_sql, res);
                DB_BIND_INT(res, "@tdid", tdid);
                DB_BIND_INT(res, "@orig", orig);
                DB_STEP(res);
            }
        }
        g_free(name);
    }
    return 0;
}


/****************************** TRANS PERSISTOR ******************************/

gint dnf_swdb_trans_beg(DnfSwdb *self,
                        const gchar *timestamp,
                        const gchar *rpmdb_version,
                        const gchar *cmdline,
                        const gchar *loginuid,
                        const gchar *releasever)
{
    if (dnf_swdb_open(self))
        return 1;

    sqlite3_stmt *res;
    const gchar *sql = INSERT_TRANS_BEG;
    DB_PREP(self->db,sql,res);
    DB_BIND(res, "@beg", timestamp);
    DB_BIND(res, "@rpmdbv", rpmdb_version );
    DB_BIND(res, "@cmdline", cmdline);
    DB_BIND(res, "@loginuid", loginuid);
    DB_BIND(res, "@releasever", releasever);
    DB_STEP(res);
    gint rc = sqlite3_last_insert_rowid(self->db);
    return rc;
}

gint dnf_swdb_trans_end(DnfSwdb *self,
                        gint tid,
                        const gchar *end_timestamp,
                        const gchar *end_rpmdb_version,
                        gint return_code)
{
    if (dnf_swdb_open(self))
        return 1;
    sqlite3_stmt *res;
    const gchar *sql = INSERT_TRANS_END;
    DB_PREP(self->db,sql,res);
    DB_BIND_INT(res, "@tid", tid );
    DB_BIND(res, "@end", end_timestamp );
    DB_BIND(res, "@rpmdbv", end_rpmdb_version);
    DB_BIND_INT(res, "@rc", return_code );
    DB_STEP(res);
    return 0;
}

gchar *dnf_swdb_trans_cmdline(DnfSwdb *self, gint tid)
{
    if (dnf_swdb_open(self))
        return NULL;
    sqlite3_stmt *res;
    const gchar *sql = GET_TRANS_CMDLINE;
    DB_PREP(self->db, sql, res);
    DB_BIND_INT(res, "@tid", tid);
    gchar *cmdline = DB_FIND_STR(res);
    return cmdline;
}

static void _resolve_altered(GPtrArray *trans)
{
    for(guint i = 0; i < (trans->len-1); i++)
    {
        DnfSwdbTrans *las = (DnfSwdbTrans *)g_ptr_array_index(trans, i);
        DnfSwdbTrans *obj = (DnfSwdbTrans *)g_ptr_array_index(trans, i+1);

        if(!las->end_rpmdb_version || !obj->beg_rpmdb_version)
            continue;

        if ( g_strcmp0(obj->end_rpmdb_version, las->beg_rpmdb_version) )
        {
            obj->altered_lt_rpmdb = 1;
            las->altered_gt_rpmdb = 1;
        }
        else
        {
            las->altered_gt_rpmdb = 0;
            obj->altered_lt_rpmdb = 0;
        }
    }
}

static gboolean _test_output(DnfSwdb *self, gint tid, const gchar *o_type)
{
    gint type = dnf_swdb_get_output_type(self, o_type);
    sqlite3_stmt *res;
    const gchar *sql = LOAD_OUTPUT;
    DB_PREP(self->db, sql, res);
    DB_BIND_INT(res, "@tid", tid);
    DB_BIND_INT(res, "@type", type);
    if (sqlite3_step(res) == SQLITE_ROW)
    {
        sqlite3_finalize(res);
        return 1;
    }
    sqlite3_finalize(res);
    return 0;
}


static void _resolve_outputs(DnfSwdb *self, GPtrArray *trans)
{
    for(guint i = 0; i < trans->len; ++i)
    {
        DnfSwdbTrans *obj = (DnfSwdbTrans *)g_ptr_array_index(trans, i);
        obj->is_output = _test_output(self, obj->tid, "stdout");
        obj->is_error = _test_output(self, obj->tid, "stderr");
    }
}

/**
* dnf_swdb_get_trans_data:
* Returns: (element-type DnfSwdbTransData)(array)(transfer container): list of #DnfSwdbTransData
*/
GPtrArray *dnf_swdb_get_trans_data(DnfSwdb *self, DnfSwdbTrans *trans)
{
    if (!trans->tid)
        return NULL;
    if (dnf_swdb_open(self))
        return NULL;
    GPtrArray *node = g_ptr_array_new();
    sqlite3_stmt *res;
    const gchar *sql = S_TRANS_DATA_BY_TID;
    DB_PREP(self->db, sql, res);
    DB_BIND_INT(res, "@tid", trans->tid);
    while(sqlite3_step(res) == SQLITE_ROW)
    {
        gchar *tmp_reason = _get_description(self->db,
                                            sqlite3_column_int(res,6),
                                            S_REASON_TYPE_BY_ID);

        gchar *tmp_state = _get_description(self->db,
                                            sqlite3_column_int(res,7),
                                            S_STATE_TYPE_BY_ID);

        DnfSwdbTransData *data = dnf_swdb_transdata_new(
            sqlite3_column_int(res, 0), //td_id
            sqlite3_column_int(res, 1), //t_id
            sqlite3_column_int(res, 2), //pd_id
            sqlite3_column_int(res, 3), //g_id
            sqlite3_column_int(res, 4), //done
            sqlite3_column_int(res, 5), //ORIGINAL_TD_ID
            tmp_reason, //reason
            tmp_state //state
        );
        g_ptr_array_add(node, (gpointer) data);
        g_free(tmp_reason);
        g_free(tmp_state);
    }
    sqlite3_finalize(res);
    return node;
}

/**
* dnf_swdb_trans_get_trans_data:
* Returns: (element-type DnfSwdbTransData)(array)(transfer container): list of #DnfSwdbTransData
*/
GPtrArray *dnf_swdb_trans_get_trans_data(DnfSwdbTrans *self)
{
    if (self->swdb)
        return dnf_swdb_get_trans_data(self->swdb, self);
    else
        return NULL;
}

/**
* dnf_swdb_trans_old:
* @tids: (element-type gint32)(transfer container): list of constants
* Returns: (element-type DnfSwdbTrans)(array)(transfer container): list of #DnfSwdbTrans
*/
GPtrArray *dnf_swdb_trans_old(DnfSwdb *self,
                              GArray *tids,
                              gint limit,
                              gboolean complete_only)
{
    if (dnf_swdb_open(self))
        return NULL;
    GPtrArray *node = g_ptr_array_new();
    sqlite3_stmt *res;
    if(tids && tids->len)
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
    DB_PREP(self->db, sql, res);
    if (limit)
        DB_BIND_INT(res, "@limit", limit);
    gint tid;
    gboolean match = 0;
    while(sqlite3_step(res) == SQLITE_ROW)
    {
        tid = sqlite3_column_int(res, 0);
        if(tids && tids->len)
        {
            match = 0;
            for(guint i = 0; i < tids->len; ++i)
            {
                if( tid == g_array_index(tids,gint,i))
                {
                    match = 1;
                    break;
                }
            }
            if (!match)
                continue;
        }
        DnfSwdbTrans *trans = dnf_swdb_trans_new(
            tid, //tid
            (gchar *)sqlite3_column_text (res, 1), //beg_t
            (gchar *)sqlite3_column_text (res, 2), //end_t
            (gchar *)sqlite3_column_text (res, 3), //beg_rpmdb_v
            (gchar *)sqlite3_column_text (res, 4), //end_rpmdb_v
            (gchar *)sqlite3_column_text (res, 5), //cmdline
            (gchar *)sqlite3_column_text (res, 6), //loginuid
            (gchar *)sqlite3_column_text (res, 7), //releasever
            sqlite3_column_int  (res, 8) // return_code
        );
        trans->swdb = self;
        g_ptr_array_add(node, (gpointer) trans);
    }
    sqlite3_finalize(res);
    if (node->len)
    {
        _resolve_altered(node);
        _resolve_outputs(self, node);
    }
    return node;
}

/**
* dnf_swdb_last:
* Returns: (transfer full): #DnfSwdbTrans
**/
DnfSwdbTrans *dnf_swdb_last (DnfSwdb *self)
{
    GPtrArray *node = dnf_swdb_trans_old(self, NULL, 1, 0);
    if (!node || !node->len)
        return NULL;
    DnfSwdbTrans *trans = g_ptr_array_index(node, 0);
    g_ptr_array_free(node, TRUE);
    return trans;
}

/**
* dnf_swdb_trans_compare_rpmdbv:
* @self: #DnfSwdbTrans object
* @rpmdbv: current version of rpmdb
*
* Compare current version of rpmdb with version from transaction
* If versions differs, set altered_gt_rpmdb to #True
*
* Returns: #true if self.end_rpmdb_version == rpmdbv
**/
gboolean dnf_swdb_trans_compare_rpmdbv(DnfSwdbTrans *self, const gchar *rpmdbv)
{
    gboolean altered = g_strcmp0(self->end_rpmdb_version, rpmdbv);
    if (altered)
    {
        self->altered_gt_rpmdb = TRUE;
    }
    return !altered;
}


/****************************** OUTPUT PERSISTOR *****************************/

static gint _output_insert(sqlite3 *db, gint tid, const gchar *msg, gint type)
{
    sqlite3_stmt *res;
    const gchar *sql = INSERT_OUTPUT;
    DB_PREP(db,sql,res);
    DB_BIND_INT(res, "@tid", tid);
    DB_BIND(res, "@msg", msg);
    DB_BIND_INT(res, "@type", type);
    DB_STEP(res);
    return 0;
}

gint dnf_swdb_log_error(DnfSwdb *self, gint tid, const gchar *msg)
{
    if (dnf_swdb_open(self))
        return 1;

    gint rc = _output_insert(
        self->db,
        tid,
        msg,
        dnf_swdb_get_output_type(self, "stderr")
    );
    return rc;
}

gint dnf_swdb_log_output(DnfSwdb *self, gint tid, const gchar *msg)
{
    if (dnf_swdb_open(self))
        return 1;

    gint rc = _output_insert(
        self->db,
        tid,
        msg,
        dnf_swdb_get_output_type(self, "stdout")
    );
    return rc;
}

static GPtrArray *_load_output(sqlite3 *db, gint tid, gint type)
{
    sqlite3_stmt *res;
    const gchar *sql = LOAD_OUTPUT;
    DB_PREP(db,sql,res);
    DB_BIND_INT(res, "@tid", tid);
    DB_BIND_INT(res, "@type", type);
    GPtrArray *l = g_ptr_array_new();
    gchar *row;
    while( (row = (gchar *)DB_FIND_STR_MULTI(res)) )
    {
        g_ptr_array_add(l, row);
    }
    return l;
}

/**
* dnf_swdb_load_error:
*
* Returns: (element-type utf8) (transfer container): list of constants
*/
GPtrArray *dnf_swdb_load_error(DnfSwdb *self, gint tid)
{
    if (dnf_swdb_open(self))
        return NULL;
    GPtrArray *rc = _load_output(
        self->db,
        tid,
        dnf_swdb_get_output_type(self, "stderr")
    );
    return rc;
}

/**
* dnf_swdb_load_output:
*
* Returns: (element-type utf8) (transfer container): list of constants
*/
GPtrArray *dnf_swdb_load_output(DnfSwdb *self, gint tid)
{
    if (dnf_swdb_open(self))
        return NULL;
    GPtrArray *rc = _load_output(self->db,
                                 tid,
                                 dnf_swdb_get_output_type(self, "stdout"));
    return rc;
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
gint _get_description_id (sqlite3 *db, const gchar *desc, const gchar *sql)
{
    sqlite3_stmt *res;

    DB_PREP(db, sql, res);
    DB_BIND(res, "@desc", desc);
    return DB_FIND(res);
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
static gint _add_description (sqlite3 *db, const gchar *desc, const gchar *sql)
{
    sqlite3_stmt *res;
    DB_PREP(db, sql, res);
    DB_BIND(res, "@desc", desc);
    DB_STEP(res);
    return sqlite3_last_insert_rowid(db);
}

/**
* _get_description:
* @db: database handle
* @id: description
* @sql: sql statement selecting description with placeholder for @id
* Returns: description for @id
**/
gchar *_get_description(sqlite3 *db, gint id, const gchar *sql)
{
    sqlite3_stmt *res;
    DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@id", id);
    return DB_FIND_STR(res);
}

/**
* dnf_swdb_get_package_type:
* @self: swdb object
* @type: package type description
* Returns: package @type description id
**/
gint dnf_swdb_get_package_type (DnfSwdb *self, const gchar *type)
{
    if(dnf_swdb_open(self))
        return -1;
    gint id = _get_description_id(self->db, type, S_PACKAGE_TYPE_ID);
    if (!id) {
        id = _add_description(self->db, type, I_DESC_PACKAGE);
    }
    return id;
}

/**
* dnf_swdb_get_output_type:
* @self: swdb object
* @type: output type description
* Returns: output @type description id
**/
gint dnf_swdb_get_output_type (DnfSwdb *self, const gchar *type)
{
    if(dnf_swdb_open(self))
        return -1;
    gint id = _get_description_id(self->db, type, S_OUTPUT_TYPE_ID);
    if (!id) {
        id = _add_description(self->db, type, I_DESC_OUTPUT);
    }
    return id;
}

/**
* dnf_swdb_get_reason_type:
* @self: swdb object
* @type: reason type description
* Returns: reason @type description id
**/
gint dnf_swdb_get_reason_type (DnfSwdb *self, const gchar *type)
{
    if(dnf_swdb_open(self))
        return -1;
    gint id = _get_description_id(self->db, type, S_REASON_TYPE_ID);
    if (!id) {
        id = _add_description(self->db, type, I_DESC_REASON);
    }
    return id;
}

/**
* dnf_swdb_get_state_type:
* @self: swdb object
* @type: state type description
* Returns: state @type description id
**/
gint dnf_swdb_get_state_type (DnfSwdb *self, const gchar *type)
{
    if(dnf_swdb_open(self))
        return -1;
    gint id = _get_description_id(self->db, type, S_STATE_TYPE_ID);
    if (!id) {
        id = _add_description(self->db, type, I_DESC_STATE);
    }
    return id;
}

//returns path to swdb, default is /var/lib/dnf/swdb.sqlite
const gchar *dnf_swdb_get_path (DnfSwdb *self)
{
    return self->path;
}

//changes path to swdb
void  dnf_swdb_set_path(DnfSwdb *self, const gchar *path)
{
    if(g_strcmp0(path,self->path) != 0)
    {
        g_free(self->path);
        self->path = g_strdup(path);
    }
}

//TRUE if db exists else false
gboolean dnf_swdb_exist(DnfSwdb *self)
{
    return g_file_test(dnf_swdb_get_path (self),G_FILE_TEST_EXISTS);
}

//open database at self->path
gint dnf_swdb_open(DnfSwdb *self)
{
    if(self->db)
        return 0;

    if(sqlite3_open(self->path, &self->db))
    {
        fprintf(stderr, "ERROR: %s %s\n", sqlite3_errmsg(self->db), self->path);
        return 1;
    }
    return _db_exec(self->db, TRUNCATE_EXCLUSIVE, NULL);
}

void dnf_swdb_close(DnfSwdb *self)
{
    if(self->db)
    {
        if(sqlite3_close(self->db) == SQLITE_BUSY)
        {
            sqlite3_stmt *res;
            while((res = sqlite3_next_stmt(self->db, NULL)))
            {
                sqlite3_finalize(res);
            }
            if(sqlite3_close(self->db))
            {
                fprintf(stderr, "ERROR: %s\n", sqlite3_errmsg(self->db));
            }
        }
        self->db = NULL;
    }
}

//create db at path
gint dnf_swdb_create_db(DnfSwdb *self)
{
    if (dnf_swdb_open(self))
        return 1;

    int failed = _create_db(self->db);
    if (failed)
    {
        fprintf(stderr, "SWDB error: Unable to create %d tables\n", failed);
        sqlite3_close(self->db);
        self->db = NULL;
        return 2;
    }
    //close - allow transformer to work with DB
    dnf_swdb_close(self);
    return 0;
}

//remove and init empty db
gint dnf_swdb_reset_db(DnfSwdb *self)
{
    if(dnf_swdb_exist(self))
    {
        dnf_swdb_close(self);
        if (g_remove(self->path)!= 0)
        {
            fprintf(stderr,"SWDB error: Database reset failed!\n");
            return 1;
        }
    }
    return dnf_swdb_create_db(self);
}
