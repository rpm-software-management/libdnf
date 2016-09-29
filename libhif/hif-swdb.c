/* hif-swdb.c
*
* Copyright (C) 2016 Red Hat, Inc.
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

/* TODO:    -   Fix fixmes
            -   Replace structs with Gobjects
            -   Think about GOM
 */

#define default_path "/var/lib/dnf/history/swdb.sqlite"

#define DB_PREP(db, sql, res) assert(_db_prepare(db, sql, &res))
#define DB_BIND(res, id, source) assert(_db_bind(res, id, source))
#define DB_BIND_INT(res, id, source) assert(_db_bind_int(res, id, source))
#define DB_STEP(res) assert(_db_step(res))
#define DB_FIND(res) _db_find(res)
#define DB_FIND_MULTI(res) _db_find_multi(res)
#define DB_FIND_STR(res) _db_find_str(res)
#define DB_FIND_STR_MULTI(res) _db_find_str_multi(res)

// Leave DB open when in multi-insert transaction
#define DB_TRANS_BEGIN 	self->running = 1;
#define DB_TRANS_END	self->running = 0;

#include "hif-swdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "hif-swdb-sql.h"

struct _HifSwdb
{
    GObject parent_instance;
    gchar   *path;
    sqlite3 *db;
    gboolean ready; //db opened
  	gboolean path_changed;
  	gboolean running; //if true, db will not be closed
    gchar *releasever;
};

G_DEFINE_TYPE(HifSwdb, hif_swdb, G_TYPE_OBJECT)
G_DEFINE_TYPE(HifSwdbPkg, hif_swdb_pkg, G_TYPE_OBJECT) //history package
G_DEFINE_TYPE(HifSwdbTrans, hif_swdb_trans, G_TYPE_OBJECT) //history transaction
G_DEFINE_TYPE(HifSwdbTransData, hif_swdb_transdata, G_TYPE_OBJECT) //history transaction data
G_DEFINE_TYPE(HifSwdbGroup, hif_swdb_group, G_TYPE_OBJECT)
G_DEFINE_TYPE(HifSwdbEnv, hif_swdb_env, G_TYPE_OBJECT)
G_DEFINE_TYPE(HifSwdbPkgData, hif_swdb_pkgdata, G_TYPE_OBJECT)

/* Table structs */

struct output_t
{
  	const gint tid;
  	const gchar *msg;
  	gint type;
};

struct trans_beg_t
{
  	const gchar *beg_timestamp;
  	const gchar *rpmdb_version;
  	const gchar *cmdline;
  	const gchar *loginuid;
  	const gchar *releasever;
};

struct trans_end_t
{
  	const gint tid;
   	const gchar *end_timestamp;
    const gchar *end_rpmdb_version;
  	const gint return_code;
};

struct trans_data_beg_t
{
  	const gint 	tid;
	const gint 	pdid;
  	const gint 	reason;
	const gint 	state;
};

struct rpm_data_t
{
    const gint   pid;
    const gchar *buildtime;
    const gchar *buildhost;
    const gchar *license;
    const gchar *packager;
    const gchar *size;
    const gchar *sourcerpm;
    const gchar *url;
    const gchar *vendor;
    const gchar *committer;
    const gchar *committime;
};

// SWDB Destructor
static void hif_swdb_finalize(GObject *object)
{
    G_OBJECT_CLASS (hif_swdb_parent_class)->finalize (object);
}

// SWDB Class initialiser
static void
hif_swdb_class_init(HifSwdbClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = hif_swdb_finalize;
}

// SWDB Object initialiser
static void
hif_swdb_init(HifSwdb *self)
{
  	self->path = (gchar*) default_path;
	self->ready = 0;
  	self->path_changed = 0;
  	self->running = 0;
    self->releasever = NULL;
}

/**
 * hif_swdb_new:
 *
 * Creates a new #HifSwdb.
 *
 * Returns: a #HifSwdb
 **/
HifSwdb* hif_swdb_new   (   const gchar* db_path,
                            const gchar* releasever)
{
    HifSwdb *swdb = g_object_new(HIF_TYPE_SWDB, NULL);
    if (releasever)
    {
        swdb->releasever = g_strdup(releasever);
    }
    if (db_path)
    {
        swdb->path = g_strjoin("/",db_path,"swdb.sqlite",NULL);
    }
    return swdb;
}


// PKG Destructor
static void hif_swdb_pkg_finalize(GObject *object)
{
    G_OBJECT_CLASS (hif_swdb_pkg_parent_class)->finalize (object);
}

// PKG Class initialiser
static void
hif_swdb_pkg_class_init(HifSwdbPkgClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = hif_swdb_pkg_finalize;
}

// PKG Object initialiser
static void
hif_swdb_pkg_init(HifSwdbPkg *self)
{
    self->done = 0;
    self->state = NULL;
    self->pid = 0;
    self->ui_from_repo = NULL;
    self->swdb = NULL;
}

/**
 * hif_swdb_pkg_new:
 *
 * Creates a new #HifSwdbPkg.
 *
 * Returns: a #HifSwdbPkg
 **/
HifSwdbPkg* hif_swdb_pkg_new(   const gchar* name,
                                const gchar* epoch,
                                const gchar* version,
                                const gchar* release,
                                const gchar* arch,
                                const gchar* checksum_data,
                                const gchar* checksum_type,
                                const gchar* type)
{
    HifSwdbPkg *swdbpkg = g_object_new(HIF_TYPE_SWDB_PKG, NULL);
    swdbpkg->name = g_strdup(name);
    swdbpkg->epoch = g_strdup(epoch);
    swdbpkg->version = g_strdup(version);
    swdbpkg->release = g_strdup(release);
    swdbpkg->arch = g_strdup(arch);
    swdbpkg->checksum_data = g_strdup(checksum_data);
    swdbpkg->checksum_type = g_strdup(checksum_type);
    if (type) swdbpkg->type = g_strdup(type); else swdbpkg->type = NULL;
  	return swdbpkg;
}

// PKG DATA Destructor
static void hif_swdb_pkgdata_finalize(GObject *object)
{
    G_OBJECT_CLASS (hif_swdb_pkgdata_parent_class)->finalize (object);
}

// PKG DATA Class initialiser
static void
hif_swdb_pkgdata_class_init(HifSwdbPkgDataClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = hif_swdb_pkgdata_finalize;
}

// PKG DATA Object initialiser
static void
hif_swdb_pkgdata_init(HifSwdbPkgData *self)
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
 * hif_swdb_pkgdata_new:
 *
 * Creates a new #HifSwdbPkgData.
 *
 * Returns: a #HifSwdbPkgData
 **/
HifSwdbPkgData* hif_swdb_pkgdata_new(   const gchar* from_repo_revision,
                                        const gchar* from_repo_timestamp,
                                        const gchar* installed_by,
                                        const gchar* changed_by,
                                        const gchar* installonly,
                                        const gchar* origin_url)
{
    HifSwdbPkgData *pkgdata = g_object_new(HIF_TYPE_SWDB_PKGDATA, NULL);
    pkgdata->from_repo_revision = g_strdup(from_repo_revision);
    pkgdata->from_repo_timestamp = g_strdup(from_repo_timestamp);
    pkgdata->installed_by = g_strdup(installed_by);
    pkgdata->changed_by = g_strdup(changed_by);
    pkgdata->installonly = g_strdup(installonly);
    pkgdata->origin_url = g_strdup(origin_url);
  	return pkgdata;
}

// TRANS Destructor
static void hif_swdb_trans_finalize(GObject *object)
{
    G_OBJECT_CLASS (hif_swdb_trans_parent_class)->finalize (object);
}

// TRANS Class initialiser
static void
hif_swdb_trans_class_init(HifSwdbTransClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = hif_swdb_trans_finalize;
}

// TRANS Object initialiser
static void
hif_swdb_trans_init(HifSwdbTrans *self)
{
    self->is_output = 0;
    self->is_error = 0;
    self->swdb = NULL;
    self->altered_lt_rpmdb = 0;
	self->altered_gt_rpmdb = 0;
}

/**
 * hif_swdb_trans_new:
 *
 * Creates a new #HifSwdbTrans.
 *
 * Returns: a #HifSwdbTrans
 **/
HifSwdbTrans* hif_swdb_trans_new(	const gint tid,
                                    const gchar *beg_timestamp,
									const gchar *end_timestamp,
									const gchar *beg_rpmdb_version,
                                    const gchar *end_rpmdb_version,
									const gchar *cmdline,
									const gchar *loginuid,
									const gchar *releasever,
									const gint return_code)
{
    HifSwdbTrans *trans = g_object_new(HIF_TYPE_SWDB_TRANS, NULL);
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
static void hif_swdb_transdata_finalize(GObject *object)
{
    G_OBJECT_CLASS (hif_swdb_transdata_parent_class)->finalize (object);
}

// TRANS DATA Class initialiser
static void
hif_swdb_transdata_class_init(HifSwdbTransDataClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = hif_swdb_transdata_finalize;
}

// TRANS DATA Object initialiser
static void
hif_swdb_transdata_init(HifSwdbTransData *self)
{
}

/**
 * hif_swdb_transdata_new:
 *
 * Creates a new #HifSwdbTransData.
 *
 * Returns: a #HifSwdbTransData
 **/
HifSwdbTransData* hif_swdb_transdata_new(   gint tdid,
                                            gint tid,
                                            gint pdid,
                                            gint gid,
                                            gint done,
                                            gint ORIGINAL_TD_ID,
                                            gchar *reason,
                                            gchar *state)
{
    HifSwdbTransData *data = g_object_new(HIF_TYPE_SWDB_TRANSDATA, NULL);
    data->tdid = tdid;
    data->tid = tid;
    data->pdid = pdid;
    data->gid = gid;
    data->done = done;
    data->ORIGINAL_TD_ID = ORIGINAL_TD_ID;
    data->reason = g_strdup(reason);
    data->state = g_strdup(state);
    return data;
}


// Group destructor
static void hif_swdb_group_finalize(GObject *object)
{
    G_OBJECT_CLASS (hif_swdb_group_parent_class)->finalize (object);
}

// Group Class initialiser
static void
hif_swdb_group_class_init(HifSwdbGroupClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = hif_swdb_group_finalize;
}

// Group Object initialiser
static void
hif_swdb_group_init(HifSwdbGroup *self)
{
    self->gid = 0;
    self->swdb = NULL;
    self->is_installed = 0;
}

/**
 * hif_swdb_group_new:
 *
 * Creates a new #HifSwdbGroup.
 *
 * Returns: a #HifSwdbGroup
 **/
HifSwdbGroup* hif_swdb_group_new(	const gchar* name_id,
									gchar* name,
									gchar* ui_name,
									gint is_installed,
									gint pkg_types,
									gint grp_types,
                                    HifSwdb *swdb)
{
    HifSwdbGroup *group = g_object_new(HIF_TYPE_SWDB_GROUP, NULL);
    group->name_id = g_strdup(name_id);
    group->name = g_strdup(name);
    group->ui_name = g_strdup(ui_name);
    group->is_installed = is_installed;
    group->pkg_types = pkg_types;
    group->grp_types = grp_types;
    group->swdb = swdb;
    return group;
}

// environment destructor
static void hif_swdb_env_finalize(GObject *object)
{
    G_OBJECT_CLASS (hif_swdb_env_parent_class)->finalize (object);
}

// environment Class initialiser
static void
hif_swdb_env_class_init(HifSwdbEnvClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = hif_swdb_env_finalize;
}

// environment Object initialiser
static void
hif_swdb_env_init(HifSwdbEnv *self)
{
    self->eid = 0;
    self->swdb = NULL;
}

/**
 * hif_swdb_env_new:
 *
 * Creates a new #HifSwdbEnv.
 *
 * Returns: a #HifSwdbEnv
 **/
HifSwdbEnv* hif_swdb_env_new(	    const gchar* name_id,
									gchar* name,
									gchar* ui_name,
									gint pkg_types,
									gint grp_types,
                                    HifSwdb *swdb)
{
    HifSwdbEnv *env = g_object_new(HIF_TYPE_SWDB_ENV, NULL);
    env->name_id = g_strdup(name_id);
    env->name = g_strdup(name);
    env->ui_name = g_strdup(ui_name);
    env->pkg_types = pkg_types;
    env->grp_types = grp_types;
    env->swdb = swdb;
    return env;
}

/******************************* Functions *************************************/

static gint _db_step	(sqlite3_stmt *res)
{
  	if (sqlite3_step(res) != SQLITE_DONE)
    {
        fprintf(stderr, "SQL error: Could not execute statement in _db_step() - try again as root?\n");
        sqlite3_finalize(res);
        return 0;
	}
  	sqlite3_finalize(res);
  	return 1; //true because of assert
}

// assumes only one parameter on output e.g "SELECT ID FROM DUMMY WHERE ..."
static gint _db_find	(sqlite3_stmt *res)
{
  	if (sqlite3_step(res) == SQLITE_ROW ) // id for description found
    {
        gint result = sqlite3_column_int(res, 0);
        sqlite3_finalize(res);
        return result;
    }
  	else
    {
        sqlite3_finalize(res);
	    return 0;
    }
}

// assumes only one text parameter on output e.g "SELECT DESC FROM DUMMY WHERE ..."
static gchar *_db_find_str(sqlite3_stmt *res)
{
  	if (sqlite3_step(res) == SQLITE_ROW ) // id for description found
    {
        gchar * result = g_strdup((gchar *)sqlite3_column_text(res, 0));
        sqlite3_finalize(res);
        return result;
    }
  	else
    {
        sqlite3_finalize(res);
        return NULL;
    }
}

static gchar *_db_find_str_multi(sqlite3_stmt *res)
{
  	if (sqlite3_step(res) == SQLITE_ROW ) // id for description found
    {
        gchar * result = g_strdup((gchar *)sqlite3_column_text(res, 0));
        return result;
    }
  	else
    {
        sqlite3_finalize(res);
        return NULL;
    }
}

static gint _db_find_multi	(sqlite3_stmt *res)
{
  	if (sqlite3_step(res) == SQLITE_ROW ) // id for description found
    {
        gint result = sqlite3_column_int(res, 0);
        return result;
    }
  	else
    {
        sqlite3_finalize(res);
	    return 0;
    }
}

static gint _db_prepare	(	sqlite3 *db,
						 	const gchar *sql,
						   	sqlite3_stmt **res)
{
  	gint rc = sqlite3_prepare_v2(db, sql, -1, res, NULL);
    if(rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
	  	fprintf(stderr, "SQL error: Prepare failed!\n");
        sqlite3_finalize(*res);
        return 0;
    }
  	return 1; //true because of assert
}

static gint _db_bind	(	sqlite3_stmt *res,
						 	const gchar *id,
						 	const gchar *source)
{
  	gint idx = sqlite3_bind_parameter_index(res, id);
    gint rc = sqlite3_bind_text(res, idx, source, -1, SQLITE_STATIC);

  	if (rc) //something went wrong
    {
        fprintf(stderr, "SQL error: Could not bind parameters(%d|%s|%s)\n",idx,id,source);
        sqlite3_finalize(res);
        return 0;
    }
  	return 1; //true because of assert
}

static gint _db_bind_int (	sqlite3_stmt *res,
						  	const gchar *id,
						  	gint source)
{
  	gint idx = sqlite3_bind_parameter_index(res, id);
    gint rc = sqlite3_bind_int(res, idx, source);

  	if (rc) //something went wrong
    {
        fprintf(stderr, "SQL error: Could not bind parameters(%s|%d)\n",id,source);
        sqlite3_finalize(res);
        return 0;
    }
  	return 1; //true because of assert
}

//use macros DB_[PREP,BIND,STEP] for parametrised queries
static gint _db_exec	(sqlite3 *db,
						 const gchar *cmd,
						 int (*callback)(void *, int, char **, char**))
{
    gchar *err_msg;
    gint result = sqlite3_exec(db, cmd, callback, 0, &err_msg);
    if ( result != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 1;
    }
    return 0;
}

static const gchar *_table_by_attribute(const gchar *attr)
{
    gint table = 0;
    if      (!g_strcmp0(attr,"from_repo_revision")) table = 1;
    else if (!g_strcmp0(attr,"from_repo_timestamp")) table = 1;
    else if (!g_strcmp0(attr,"installed_by")) table = 1;
    else if (!g_strcmp0(attr,"changed_by")) table = 1;
    else if (!g_strcmp0(attr,"installonly")) table = 1;
    else if (!g_strcmp0(attr,"origin_url")) table = 1;
    else if (!g_strcmp0(attr,"beg_timestamp")) table = 2;
    else if (!g_strcmp0(attr,"end_timestamp")) table = 2;
    else if (!g_strcmp0(attr,"beg_RPMDB_version")) table = 2;
    else if (!g_strcmp0(attr,"end_RPMDB_version")) table = 2;
    else if (!g_strcmp0(attr,"cmdline")) table = 2;
    else if (!g_strcmp0(attr,"loginuid")) table = 2;
    else if (!g_strcmp0(attr,"releasever")) table = 2;
    else if (!g_strcmp0(attr,"return_code")) table = 2;
    else if (!g_strcmp0(attr,"done")) table = 3;
    else if (!g_strcmp0(attr,"ORIGINAL_TD_ID")) table = 3;
    else if (!g_strcmp0(attr,"reason")) table = 3;
    else if (!g_strcmp0(attr,"state")) table = 3;
    else if (!g_strcmp0(attr,"buildtime")) table = 4;
    else if (!g_strcmp0(attr,"buildhost")) table = 4;
    else if (!g_strcmp0(attr,"license")) table = 4;
    else if (!g_strcmp0(attr,"packager")) table = 4;
    else if (!g_strcmp0(attr,"size")) table = 4;
    else if (!g_strcmp0(attr,"sourcerpm")) table = 4;
    else if (!g_strcmp0(attr,"url")) table = 4;
    else if (!g_strcmp0(attr,"vendor")) table = 4;
    else if (!g_strcmp0(attr,"committer")) table = 4;
    else if (!g_strcmp0(attr,"committime")) table = 4;

    if (table == 1)
    {
        return "PACKAGE_DATA";
    }
    if (table == 2)
    {
        return "TRANS";
    }
    if (table == 3)
    {
        return "TRANS_DATA";
    }
    if (table == 4)
    {
        return "RPM_DATA";
    }
    return NULL; //attr not found
}

/*********************************** UTILS *************************************/

// Pattern on input, transaction IDs on output
static GArray *_simple_search(sqlite3* db, const gchar * pattern)
{
    GArray *tids = g_array_new(0,0,sizeof(gint));
    sqlite3_stmt *res_simple;
    const gchar *sql_simple = SIMPLE_SEARCH;
    DB_PREP(db, sql_simple, res_simple);
    DB_BIND(res_simple, "@pat", pattern);
    gint pid_simple;
    GArray *simple = g_array_new(0,0,sizeof(gint));
    while( (pid_simple = DB_FIND_MULTI(res_simple)))
    {
        g_array_append_val(simple, pid_simple);
    }
    gint pdid;
    for(guint i = 0; i < simple->len; i++)
    {
        pid_simple = g_array_index(simple, gint, i);
        GArray *pdids = _all_pdid_for_pid(db, pid_simple);
        for( guint j = 0; j < pdids->len; j++)
        {
            pdid = g_array_index(pdids, gint, j);
            GArray *tids_for_pdid = _tids_from_pdid(db, pdid);
            tids = g_array_append_vals (tids, tids_for_pdid->data, tids_for_pdid->len);
        }
    }
    return tids;
}

/* XXX Garbage just for testing
#include <sys/time.h>
static struct timeval tm1;

static inline void start()
{
    gettimeofday(&tm1, NULL);
}

static inline void stop()
{
    struct timeval tm2;
    gettimeofday(&tm2, NULL);

    unsigned long long t = 1000 * (tm2.tv_sec - tm1.tv_sec) + (tm2.tv_usec - tm1.tv_usec) / 1000;
    printf("%llu ms\n", t);
}
*/

/*
* Provides package search with extended pattern
*/
static GArray *_extended_search (sqlite3* db, const gchar *pattern)
{
    GArray *tids = g_array_new(0, 0, sizeof(gint));
    sqlite3_stmt *res;
    const gchar *sql = SEARCH_PATTERN;
    DB_PREP( db, sql, res);
    //lot of patterns...
    DB_BIND(res, "@pat", pattern);
    DB_BIND(res, "@pat", pattern);
    DB_BIND(res, "@pat", pattern);
    DB_BIND(res, "@pat", pattern);
    DB_BIND(res, "@pat", pattern);
    DB_BIND(res, "@pat", pattern);
    gint pid = DB_FIND(res);
    gint pdid;
    if(pid)
    {
        GArray *pdids = _all_pdid_for_pid(db, pid);
        for(guint i = 0; i < pdids->len; i++)
        {
            pdid = g_array_index(pdids, gint, i);
            GArray *tids_for_pdid = _tids_from_pdid(db, pdid);
            tids = g_array_append_vals (tids, tids_for_pdid->data, tids_for_pdid->len);
        }
    }
    return tids;
}

/**
* hif_swdb_search:
* @patterns: (element-type utf8) (transfer container): list of constants
* Returns: (element-type gint32) (transfer container): list of constants
*/
GArray *hif_swdb_search (   HifSwdb *self,
                            const GSList *patterns)
{
    if (hif_swdb_open(self))
        return NULL;
    DB_TRANS_BEGIN

    GArray *tids = g_array_new(0, 0, sizeof(gint));
    while(patterns)
    {
        const gchar *pattern = patterns->data;
        patterns = patterns->next;

        //try simple search
        GArray *simple =  _simple_search(self->db, pattern);
        if(simple->len)
        {
            tids = g_array_append_vals(tids, simple->data, simple->len);
            continue;
        }
        //need for extended search
        GArray *extended = _extended_search(self->db, pattern);
        if(extended)
        {
            tids = g_array_append_vals(tids, extended->data, extended->len);
        }
    }
    DB_TRANS_END
    hif_swdb_close(self);
    return tids;
}
static gint _pid_by_nvra    (   sqlite3 *db,
                                const gchar *nvra)
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
* hif_swdb_checksums_by_nvras:
* @nvras: (element-type utf8) (transfer container): list of constants
* Returns: (element-type utf8) (transfer container): list of constants
*/
GPtrArray * hif_swdb_checksums_by_nvras(    HifSwdb *self,
                                            GPtrArray *nvras)
{
    if (hif_swdb_open(self))
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
    hif_swdb_close(self);
    return checksums;
}

/******************************* GROUP PERSISTOR *******************************/

/* insert into groups/env package tables
 * Returns: 0 if successfull
 * usage: rc = _insert_id_name(db, "TABLE", "Xid", "pkg_name");
 * Requires opened DB
 */
static gint _insert_id_name (sqlite3 *db, const gchar *table, gint id, const gchar *name)
{
    sqlite3_stmt *res;
    gchar *sql = g_strjoin(" ","insert into",table,"values (null, @id, @name)", NULL);

    DB_PREP(db,sql,res);

    //entity id
	DB_BIND_INT(res, "@id", id);

    //package name
    DB_BIND(res, "@name", name);

    DB_STEP(res);
  	return 0;
}

static gint _insert_group_additional(   HifSwdb *self,
                                        HifSwdbGroup *group,
                                        GPtrArray *data,
                                        const gchar *table)
{
    if (hif_swdb_open(self))
        return 1;
    for(guint i = 0; i < data->len; i++)
    {
        _insert_id_name (self->db, table, group->gid, (gchar *)g_ptr_array_index(data, i));
    }
	hif_swdb_close(self);
  	return 0;
}

//add new group package
/**
* hif_swdb_group_add_package:
* @packages: (element-type utf8)(transfer container): list of constants
*/
gint hif_swdb_group_add_package (       HifSwdbGroup *group,
                                        GPtrArray *packages)
{
    if(group->gid)
        return _insert_group_additional(group->swdb, group, packages, "GROUPS_PACKAGE");
    else
        return 0;
}

//add new group exclude
/**
* hif_swdb_group_add_exclude:
* @exclude: (element-type utf8)(transfer container): list of constants
*/
gint hif_swdb_group_add_exclude (       HifSwdbGroup *group,
                                        GPtrArray *exclude)
{
    if(group->gid)
        return _insert_group_additional(group->swdb, group, exclude, "GROUPS_EXCLUDE");
    else
        return 0;
}

//add new environments exclude
/**
* hif_swdb_env_add_exclude:
* @exclude: (element-type utf8)(transfer container): list of constants
*/
gint hif_swdb_env_add_exclude (     HifSwdbEnv *env,
                                    GPtrArray *exclude)
{
    if(env->eid)
    {
        if (hif_swdb_open(env->swdb))
            return 1;
        for(guint i = 0; i < exclude->len; i++)
        {
            _insert_id_name (env->swdb->db, "ENVIRONMENTS_EXCLUDE", env->eid, (gchar *)g_ptr_array_index(exclude, i));
        }
        hif_swdb_close(env->swdb);
        return 0;
    }
    else
        return 1;
}

static gint _group_id_to_gid(sqlite3 *db, const gchar *group_id)
{
    sqlite3_stmt *res;
    const gchar* sql = S_GID_BY_NAME_ID;
    DB_PREP(db, sql, res);
    DB_BIND(res, "@id", group_id);
    return DB_FIND(res);
}

/**
* hif_swdb_env_add_group:
* @groups: (element-type utf8)(transfer container): list of constants
*/
gint hif_swdb_env_add_group (   HifSwdbEnv *env,
                                GPtrArray *groups)
{
    if(env->eid)
    {
        if (hif_swdb_open(env->swdb))
            return 1;
        const gchar *sql = I_ENV_GROUP;
        gint gid;
        for(guint i = 0; i < groups->len; i++)
        {
            //bind group_id to gid
            gid = _group_id_to_gid(env->swdb->db, (const gchar*)g_ptr_array_index(groups, i));
            if (!gid)
                continue;
            sqlite3_stmt *res;
            DB_PREP(env->swdb->db, sql, res);
            DB_BIND_INT(res, "@eid", env->eid);
            DB_BIND_INT(res, "@gid", gid);
            DB_STEP(res);
        }
        hif_swdb_close(env->swdb);
        return 0;
    }
    else
        return 1;
}

static void _add_group  (   sqlite3 *db,
                            HifSwdbGroup *group)
{
    sqlite3_stmt *res;
    const gchar *sql = I_GROUP;
    DB_PREP(db, sql, res);
    DB_BIND(res, "@name_id", group->name_id);
    DB_BIND(res, "@name", group->name);
    DB_BIND(res, "@ui_name", group->ui_name);
    DB_BIND_INT(res, "@is_installed", group->is_installed);
    DB_BIND_INT(res, "@pkg_types", group->pkg_types);
    DB_BIND_INT(res, "@grp_types", group->grp_types);
    DB_STEP(res);
    group->gid = sqlite3_last_insert_rowid(db);
}

gint hif_swdb_add_group (   HifSwdb *self,
                            HifSwdbGroup *group)
{
    if (hif_swdb_open(self))
        return 1;
    _add_group  (self->db, group);
    hif_swdb_close(self);
    return 0;
}

static void _add_env(sqlite3 *db, HifSwdbEnv *env)
{
    sqlite3_stmt *res;
    const gchar *sql = I_ENV;
    DB_PREP(db, sql, res);
    DB_BIND(res, "@name_id", env->name_id);
    DB_BIND(res, "@name", env->name);
    DB_BIND(res, "@ui_name", env->ui_name);
    DB_BIND_INT(res, "@pkg_types", env->pkg_types);
    DB_BIND_INT(res, "@grp_types", env->grp_types);
    DB_STEP(res);
    env->eid = sqlite3_last_insert_rowid(db);

}

gint hif_swdb_add_env (     HifSwdb *self,
                            HifSwdbEnv *env)
{
    if (hif_swdb_open(self))
        return 1;
    _add_env  (self->db, env);
    hif_swdb_close(self);
    return 0;
}

static HifSwdbGroup *_get_group(sqlite3 *db,
                                const gchar *name_id)
{
    sqlite3_stmt *res;
    const gchar *sql = S_GROUP_BY_NAME_ID;
    DB_PREP(db, sql, res);
    DB_BIND(res, "@id", name_id);
    if (sqlite3_step(res) == SQLITE_ROW)
    {
        HifSwdbGroup *group = hif_swdb_group_new(
                                name_id, //name_id
                                (gchar*)sqlite3_column_text(res, 2),//name
                                (gchar*)sqlite3_column_text(res, 3),//ui_name
                                sqlite3_column_int(res, 4),//is_installed
                                sqlite3_column_int(res, 5),//pkg_types
                                sqlite3_column_int(res, 6),//grp_types
                                NULL); //swdb
        group->gid = sqlite3_column_int(res,0);
        sqlite3_finalize(res);
        return group;
    }
    return NULL;
}

/**
* hif_swdb_get_group:
* Returns: (transfer full): #HifSwdbGroup
*/
HifSwdbGroup *hif_swdb_get_group	(HifSwdb * self,
	 								const gchar* name_id)
{
    if (hif_swdb_open(self))
        return NULL;

    HifSwdbGroup *group = _get_group(self->db, name_id);
    if(group)
        group->swdb = self;
    hif_swdb_close(self);
    return group;
}

static HifSwdbEnv *_get_env(    sqlite3 *db,
                                const gchar *name_id)
{
    sqlite3_stmt *res;
    const gchar *sql = S_ENV_BY_NAME_ID;
    DB_PREP(db, sql, res);
    DB_BIND(res, "@id", name_id);
    if (sqlite3_step(res) == SQLITE_ROW)
    {
        HifSwdbEnv *env = hif_swdb_env_new(
                                name_id, //name_id
                                (gchar*)sqlite3_column_text(res, 2),//name
                                (gchar*)sqlite3_column_text(res, 3),//ui_name
                                sqlite3_column_int(res, 4),//pkg_types
                                sqlite3_column_int(res, 5),//grp_types
                                NULL); //swdb
        env->eid = sqlite3_column_int(res,0);
        sqlite3_finalize(res);
        return env;
    }
    return NULL;
}

/**
* hif_swdb_get_env:
* Returns: (transfer full): #HifSwdbEnv
*/
HifSwdbEnv *hif_swdb_get_env(   HifSwdb * self,
	 							const gchar* name_id)
{
    if (hif_swdb_open(self))
        return NULL;

    HifSwdbEnv *env = _get_env(self->db, name_id);
    if(env)
        env->swdb = self;
    hif_swdb_close(self);
    return env;
}

/**
* hif_swdb_groups_by_pattern:
* Returns: (element-type HifSwdbGroup)(array)(transfer container): list of #HifSwdbGroup
*/
GPtrArray *hif_swdb_groups_by_pattern   (HifSwdb *self,
                                        const gchar *pattern)
{
    if (hif_swdb_open(self))
        return NULL;
    GPtrArray *node = g_ptr_array_new();
    sqlite3_stmt *res;
    const gchar *sql = S_GROUPS_BY_PATTERN;
    DB_PREP(self->db, sql, res);
    DB_BIND(res, "@pat", pattern);
    DB_BIND(res, "@pat", pattern);
    DB_BIND(res, "@pat", pattern);
    while(sqlite3_step(res) == SQLITE_ROW)
    {
        HifSwdbGroup *group = hif_swdb_group_new(
                                (const gchar*)sqlite3_column_text(res,1), //name_id
                                (gchar*)sqlite3_column_text(res, 2),//name
                                (gchar*)sqlite3_column_text(res, 3),//ui_name
                                sqlite3_column_int(res, 4),//is_installed
                                sqlite3_column_int(res, 5),//pkg_types
                                sqlite3_column_int(res, 6),//grp_types
                                self); //swdb
        group->gid = sqlite3_column_int(res,0);
        g_ptr_array_add(node, (gpointer) group);
    }
    hif_swdb_close(self);
    return node;
}

/**
* hif_swdb_env_by_pattern:
* Returns: (element-type HifSwdbEnv)(array)(transfer container): list of #HifSwdbEnv
*/
GPtrArray *hif_swdb_env_by_pattern      (HifSwdb *self,
                                        const gchar *pattern)
{
    if (hif_swdb_open(self))
        return NULL;
    GPtrArray *node = g_ptr_array_new();
    sqlite3_stmt *res;
    const gchar *sql = S_ENV_BY_PATTERN;
    DB_PREP(self->db, sql, res);
    DB_BIND(res, "@pat", pattern);
    DB_BIND(res, "@pat", pattern);
    DB_BIND(res, "@pat", pattern);
    while(sqlite3_step(res) == SQLITE_ROW)
    {
        HifSwdbEnv *env = hif_swdb_env_new(
                                (const gchar*)sqlite3_column_text(res,1), //name_id
                                (gchar*)sqlite3_column_text(res, 2),//name
                                (gchar*)sqlite3_column_text(res, 3),//ui_name
                                sqlite3_column_int(res, 4),//pkg_types
                                sqlite3_column_int(res, 5),//grp_types
                                self); //swdb
        env->eid = sqlite3_column_int(res,0);
        g_ptr_array_add(node, (gpointer) env);
    }
    hif_swdb_close(self);
    return node;
}

static inline GPtrArray *_get_list_from_table(sqlite3_stmt *res)
{
    GPtrArray *node = g_ptr_array_new();
    while(sqlite3_step(res) == SQLITE_ROW)
    {
        gchar *str = g_strdup((const gchar*)sqlite3_column_text(res,0));
        g_ptr_array_add(node, (gpointer) str);
    }
    sqlite3_finalize(res);
    return node;
}

/**
* hif_swdb_group_get_exclude:
* Returns: (element-type utf8)(array)(transfer container): list of utf8
*/
GPtrArray * hif_swdb_group_get_exclude(HifSwdbGroup *self)
{
    if(!self->gid)
        return NULL;
    if (hif_swdb_open(self->swdb) )
        return NULL;
    sqlite3_stmt *res;
    const gchar *sql = S_GROUP_EXCLUDE_BY_ID;
    DB_PREP(self->swdb->db, sql, res);
    DB_BIND_INT(res, "@gid", self->gid);
    GPtrArray *node = _get_list_from_table(res);
    hif_swdb_close(self->swdb);
    return node;
}

/**
* hif_swdb_group_get_full_list:
* Returns: (element-type utf8)(array)(transfer container): list of utf8
*/
GPtrArray * hif_swdb_group_get_full_list(HifSwdbGroup *self)
{
    if(!self->gid)
        return NULL;
    if (hif_swdb_open(self->swdb) )
        return NULL;
    sqlite3_stmt *res;
    const gchar *sql = S_GROUP_PACKAGE_BY_ID;
    DB_PREP(self->swdb->db, sql, res);
    DB_BIND_INT(res, "@gid", self->gid);
    GPtrArray *node = _get_list_from_table(res);
    hif_swdb_close(self->swdb);
    return node;
}

/**
* hif_swdb_group_update_full_list:
* @full_list: (element-type utf8)(transfer container): list of constants
*/
gint hif_swdb_group_update_full_list(   HifSwdbGroup *group,
                                        GPtrArray *full_list)
{
    if(!group->gid)
        return 1;
    if (hif_swdb_open(group->swdb) )
        return 1;
    sqlite3_stmt *res;
    const gchar *sql = R_FULL_LIST_BY_ID;
    DB_PREP(group->swdb->db, sql, res);
    DB_BIND_INT(res, "@gid", group->gid);
    DB_STEP(res);
    hif_swdb_close(group->swdb);
    hif_swdb_group_add_package(group, full_list);
    return 0;
}

static void _update_group   (   sqlite3 *db,
                                HifSwdbGroup *group)
{
    sqlite3_stmt *res;
    const gchar *sql = U_GROUP;
    DB_PREP(db, sql, res);
    DB_BIND(res, "@name", group->name);
    DB_BIND(res, "@ui_name", group->ui_name);
    DB_BIND_INT(res, "@is_installed", group->is_installed);
    DB_BIND_INT(res, "@pkg_types", group->pkg_types);
    DB_BIND_INT(res, "@grp_types", group->grp_types);
    DB_BIND_INT(res, "@gid", group->gid);
    DB_STEP(res);
}

gint hif_swdb_uninstall_group( HifSwdb *self,
                            HifSwdbGroup *group)
{
    if(!group->gid)
        return 1;
    if (hif_swdb_open(self) )
        return 1;
    group->is_installed = 0;
    _update_group( self->db, group);
    hif_swdb_close(self);
    return 0;
}

/**
* hif_swdb_env_get_grp_list:
* Returns: (element-type utf8)(array)(transfer container): list of utf8
*/
GPtrArray *hif_swdb_env_get_grp_list    (HifSwdbEnv* env)
{
    if(!env->eid)
        return NULL;
    if (hif_swdb_open(env->swdb) )
        return NULL;
    GPtrArray *node = g_ptr_array_new();
    sqlite3_stmt *res;
    const gchar *sql = S_GROUP_NAME_ID_BY_EID;
    DB_PREP(env->swdb->db, sql, res);
    DB_BIND_INT(res, "@eid", env->eid);
    while (sqlite3_step(res) == SQLITE_ROW)
    {
        gchar *name_id = g_strdup((const gchar*)sqlite3_column_text(res,0));
        g_ptr_array_add(node, (gpointer) name_id);
    }
    sqlite3_finalize(res);
    hif_swdb_close(env->swdb);
    return node;
}

gboolean hif_swdb_env_is_installed  (HifSwdbEnv *env )
{
    if(!env->eid)
        return 0;
    if (hif_swdb_open(env->swdb) )
        return 0;
    gboolean found = 1;
    sqlite3_stmt *res;
    const gchar* sql = S_IS_INSTALLED_BY_EID;
    DB_PREP(env->swdb->db, sql, res);
    DB_BIND_INT(res, "@eid", env->eid);
    while(sqlite3_step(res) == SQLITE_ROW)
    {
        if(!sqlite3_column_int(res, 0)) //is_installed is not True
        {
            found = 0;
            break;
        }
    }
    sqlite3_finalize(res);
    hif_swdb_close(env->swdb);
    return found;
}

/**
* hif_swdb_env_get_exclude:
* Returns: (element-type utf8)(array)(transfer container): list of utf8
*/
GPtrArray *hif_swdb_env_get_exclude    (HifSwdbEnv* self)
{
    if(!self->eid)
        return NULL;
    if (hif_swdb_open(self->swdb) )
        return NULL;
    sqlite3_stmt *res;
    const gchar *sql = S_ENV_EXCLUDE_BY_ID;
    DB_PREP(self->swdb->db, sql, res);
    DB_BIND_INT(res, "@eid", self->eid);
    GPtrArray *node = _get_list_from_table(res);
    hif_swdb_close(self->swdb);
    return node;
}


/**
* hif_swdb_groups_commit:
* @groups: (element-type utf8)(transfer container): list of constants
*/
gint hif_swdb_groups_commit(    HifSwdb *self,
                                GPtrArray *groups)
{
    if (hif_swdb_open(self) )
        return 1;
    const gchar *sql = U_GROUP_COMMIT;
    for(guint i = 0; i < groups->len; ++i)
    {
        sqlite3_stmt *res;
        DB_PREP(self->db, sql, res);
        DB_BIND(res, "@id", (const gchar *)g_ptr_array_index(groups, i));
        DB_STEP(res);
    }
    hif_swdb_close(self);
    return 0;
}

static void _log_group_trans (   sqlite3 *db,
                                const gint tid,
                                GPtrArray *groups,
                                const gint is_installed)
{
    const gchar *sql = I_TRANS_GROUP_DATA;
    for(guint i = 0; i< groups->len; ++i)
    {
        sqlite3_stmt *res;
        HifSwdbGroup *group = g_ptr_array_index(groups, i);
        DB_PREP(db, sql, res);
        DB_BIND_INT(res, "@tid", tid);
        DB_BIND_INT(res, "@gid", group->gid);
        DB_BIND(res, "@name_id", group->name_id);
        DB_BIND(res, "@name", group->name);
        DB_BIND(res, "@ui_name", group->ui_name);
        DB_BIND_INT(res, "@is_installed", is_installed);
        DB_BIND_INT(res, "@pkg_types", group->pkg_types);
        DB_BIND_INT(res, "@grp_types", group->grp_types);
        DB_STEP(res);
    }

}

/**
* hif_swdb_log_group_trans:
* @installing: (element-type HifSwdbGroup)(array)(transfer container): list of #HifSwdbGroup
* @removing: (element-type HifSwdbGroup)(array)(transfer container): list of #HifSwdbGroup
*/
gint hif_swdb_log_group_trans(  HifSwdb *self,
                                const gint tid,
                                GPtrArray *installing,
                                GPtrArray *removing)
{
    if (hif_swdb_open(self) )
        return 1;
    _log_group_trans(self->db, tid, installing, 1);
    _log_group_trans(self->db, tid, removing, 0);

    hif_swdb_close(self);
    return 0;
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

static const gchar* _repo_by_rid(   sqlite3 *db,
                                    const gint rid)
{
    sqlite3_stmt *res;
    const gchar *sql = S_REPO_BY_RID;
    DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@rid", rid);
    return DB_FIND_STR(res);
}

const gchar *hif_swdb_repo_by_pattern (     HifSwdb *self,
                                            const gchar *pattern)
{
    if (hif_swdb_open(self))
    	return NULL;
    gint pid = _pid_by_nvra(self->db, pattern);
    if(!pid)
    {
        hif_swdb_close(self);
        return "unknown";
    }
    sqlite3_stmt *res;
    const gchar *sql = S_REPO_FROM_PID2;
    DB_PREP(self->db, sql, res);
    DB_BIND_INT(res, "@pid", pid);
    const gchar *r_name = DB_FIND_STR(res);
    hif_swdb_close(self);
    return r_name;
}

const gint hif_swdb_set_repo (      HifSwdb *self,
                                    const gchar *nvra,
                                    const gchar *repo)
{
    if (hif_swdb_open(self))
        return 1;
    DB_TRANS_BEGIN
    gint pid = _pid_by_nvra(self->db, nvra);
    if (!pid)
    {
        hif_swdb_close(self);
        return 1;
    }
    const gint rid = _bind_repo_by_name(self->db, repo);
    sqlite3_stmt *res;
    const gchar *sql = U_REPO_BY_PID;
    DB_PREP(self->db, sql, res);
    DB_BIND_INT(res, "@rid", rid);
    DB_BIND_INT(res, "@pid", pid);
    DB_STEP(res);
    DB_TRANS_END
    hif_swdb_close(self);
    return 0;
}

/**************************** PACKAGE PERSISTOR ******************************/

static gint _package_insert(HifSwdb *self, HifSwdbPkg *package)
{
    gint type_id = hif_swdb_get_package_type(self,package->type);
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
  	return sqlite3_last_insert_rowid(self->db);
}

gint hif_swdb_add_package_nevracht(	HifSwdb *self,
				  					const gchar *name,
									const gchar *epoch,
									const gchar *version,
									const gchar *release,
				  					const gchar *arch,
				 					const gchar *checksum_data,
								   	const gchar *checksum_type,
								  	const gchar *type)
{
  	if (hif_swdb_open(self))
    	return 1;
  	DB_TRANS_BEGIN
 	HifSwdbPkg *package = hif_swdb_pkg_new(name, epoch, version, release, arch,
		checksum_data, checksum_type, type);
    gint rc = _package_insert(self, package);
  	DB_TRANS_END
  	hif_swdb_close(self);
  	return rc;
}

const gint 	hif_swdb_get_pid_by_nevracht(	HifSwdb *self,
											const gchar *name,
											const gchar *epoch,
											const gchar *version,
											const gchar *release,
											const gchar *arch,
											const gchar *checksum_data,
											const gchar *checksum_type,
											const gchar *type,
											const gboolean create)
{
    if (hif_swdb_open(self))
    	return 1;
  	DB_TRANS_BEGIN
 	const gint _type = hif_swdb_get_package_type(self,type);
    sqlite3_stmt *res;
    gint rc = 0;
    if (g_strcmp0(checksum_type,"") && g_strcmp0(checksum_data,""))
    {
        const gchar *sql = FIND_PKG_BY_NEVRACHT;
        DB_PREP(self->db, sql, res);
        DB_BIND(res, "@name", name);
        DB_BIND(res, "@epoch", epoch);
        DB_BIND(res, "@version", version);
        DB_BIND(res, "@release", release);
        DB_BIND(res, "@arch", arch);
        DB_BIND_INT(res, "@type", _type);
        DB_BIND(res, "@cdata", checksum_data);
        DB_BIND(res, "@ctype", checksum_type);
        rc = DB_FIND(res);
    }
    else
    {
        const gchar *sql = FIND_PKG_BY_NEVRA;
        DB_PREP(self->db, sql, res);
        DB_BIND(res, "@name", name);
        DB_BIND(res, "@epoch", epoch);
        DB_BIND(res, "@version", version);
        DB_BIND(res, "@release", release);
        DB_BIND(res, "@arch", arch);
        DB_BIND_INT(res, "@type", _type);
        rc = DB_FIND(res);
    }
    if(rc) //found
    {
        DB_TRANS_END
      	hif_swdb_close(self);
      	return rc;
    }

    if(create)
    {
        return hif_swdb_add_package_nevracht(self, name, epoch, version, release, arch, checksum_data, checksum_type, type);
    }
    DB_TRANS_END
    hif_swdb_close(self);
    return 0;
}

static gint _package_data_update (  sqlite3 *db,
                                    const gint pid,
                                    HifSwdbPkgData *pkgdata)
{
    gint rid = _bind_repo_by_name(db, pkgdata->from_repo);
  	sqlite3_stmt *res;
   	const gchar *sql = UPDATE_PKG_DATA;
	DB_PREP(db,sql,res);
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

gint 	hif_swdb_log_package_data(	HifSwdb *self,
                                    const gint pid,
									HifSwdbPkgData *pkgdata )
{
  	if (hif_swdb_open(self))
    	return 1;
  	gint rc = _package_data_update(self->db, pid, pkgdata);
  	hif_swdb_close(self);
  	return rc;
}

static gint _pdid_from_pid (	sqlite3 *db,
								const gint pid )
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

static GArray* _all_pdid_for_pid (	sqlite3 *db,
								    const gint pid )
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

static gint _tid_from_pdid (	sqlite3 *db,
								const gint pdid )
{
  	sqlite3_stmt *res;
  	const gchar *sql = FIND_TID_FROM_PDID;
  	DB_PREP(db, sql, res);
  	DB_BIND_INT(res, "@pdid", pdid);
  	return DB_FIND(res);
}

static GArray * _tids_from_pdid (	sqlite3 *db,
								    const gint pdid )
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

static GArray *_pids_by_tid (    sqlite3 *db,
                                const gint tid)
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

static const gchar *_insert_attr(const gchar *sql, const gchar* attr)
{
    return  g_strjoin(" ", "SELECT", attr, sql, NULL);
}

const gchar *hif_swdb_get_pkg_attr( HifSwdb *self,
                                    const gint pid,
                                    const gchar *attribute)
{
    if (hif_swdb_open(self))
    {
    	return NULL;
    }

    const gchar *table = _table_by_attribute(attribute);
    if (!table) //attribute not found
    {
        if (!g_strcmp0(attribute, "command_line")) //compatibility issue
            table = "TRANS";
        else
            return NULL;
    }
    sqlite3_stmt *res;
    if (!g_strcmp0(table,"PACKAGE_DATA"))
    {
        const gchar *sql = _insert_attr(PKG_DATA_ATTR_BY_PID, attribute);
        DB_PREP(self->db, sql, res);
        DB_BIND_INT(res, "@pid", pid);
        gchar *output = DB_FIND_STR(res);
        hif_swdb_close(self);
        return output;
    }
    if (!g_strcmp0(table,"TRANS_DATA"))
    {
        //need to get PD_ID first
        const gint pdid = _pdid_from_pid(self->db, pid);
        const gchar *sql = _insert_attr( TRANS_DATA_ATTR_BY_PDID, attribute);
        DB_PREP(self->db, sql, res);
        DB_BIND_INT(res, "@pdid", pdid);
        gchar *rv;
        if (!g_strcmp0(attribute,"reason"))
        {
            const gint rc_id = DB_FIND(res);
            rv = _look_for_desc(self->db, "REASON_TYPE", rc_id);
            hif_swdb_close(self);
            if (!rv)
                return (gchar*)"Unknown";
            else
                return rv;
        }
        if (!g_strcmp0(attribute,"state"))
        {
            const gint rc_id = DB_FIND(res);
            rv = _look_for_desc(self->db, "STATE_TYPE", rc_id);
            hif_swdb_close(self);
            if (!rv)
                return (gchar*)"Unknown";
            else
                return rv;
        }
        gchar *output = DB_FIND_STR(res);
        hif_swdb_close(self);
        return output;
    }
    if (!g_strcmp0(table,"TRANS"))
    {
        //need to get T_ID first via PD_ID
        const gint pdid = _pdid_from_pid(self->db, pid);
        const gint tid = _tid_from_pdid(self->db, pdid);
        const gchar *sql = _insert_attr( TRANS_ATTR_BY_TID, attribute);
        DB_PREP(self->db, sql, res);
        DB_BIND_INT(res, "@tid", tid);
        gchar *output = DB_FIND_STR(res);
        hif_swdb_close(self);
        return output;
    }
    if (!g_strcmp0(table,"RPM_DATA"))
    {
        const gchar *sql = _insert_attr (RPM_ATTR_BY_PID, attribute);
        DB_PREP(self->db, sql, res);
        DB_BIND_INT(res, "@pid", pid);
        gchar *output= DB_FIND_STR(res);
        hif_swdb_close(self);
        return output;
    }
    hif_swdb_close(self);
    return NULL;
}

const gchar *hif_swdb_attr_by_pattern (   HifSwdb *self,
                                            const gchar *attr,
                                            const gchar *pattern)
{
    if (hif_swdb_open(self))
    	return NULL;
    gint pid = _pid_by_nvra(self->db, pattern);
    if(!pid)
    {
        hif_swdb_close(self);
        return NULL;
    }
    return hif_swdb_get_pkg_attr(self, pid, attr);
}

static void _resolve_package_state  (   HifSwdb *self,
                                        HifSwdbPkg *pkg)
{
    sqlite3_stmt *res;
    const gchar *sql = S_PACKAGE_STATE;
    DB_PREP(self->db, sql, res);
    DB_BIND_INT(res, "@pid", pkg->pid);
    if (sqlite3_step(res) == SQLITE_ROW)
    {
        gint state_code = sqlite3_column_int(res, 2);
        pkg->done = sqlite3_column_int(res, 1);
        sqlite3_finalize(res);
        pkg->state = _look_for_desc(self->db, "STATE_TYPE", state_code);
    }
}

static HifSwdbPkgData *_get_package_data_by_pid (   sqlite3 *db,
                                                    const gint pid)
{
    sqlite3_stmt *res;
    const gchar *sql = S_PACKAGE_DATA_BY_PID;
    DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@pid", pid);
    if (sqlite3_step(res) == SQLITE_ROW)
    {
        HifSwdbPkgData *pkgdata = hif_swdb_pkgdata_new(
            (gchar *)sqlite3_column_text(res, 3), //from_repo_revision
            (gchar *)sqlite3_column_text(res, 4), //from_repo_timestamp
            (gchar *)sqlite3_column_text(res, 5), //installed_by
            (gchar *)sqlite3_column_text(res, 6), //changed_by
            (gchar *)sqlite3_column_text(res, 7), //installonly
            (gchar *)sqlite3_column_text(res, 8) //origin_url
        );
        pkgdata->pdid = sqlite3_column_int(res, 0); //pdid
        pkgdata->pid = pid;
        gint rid = sqlite3_column_int(res, 2);
        sqlite3_finalize(res);
        pkgdata->from_repo = (gchar *)_repo_by_rid(db, rid); //from repo
        return pkgdata;
    }
    sqlite3_finalize(res);
    return NULL;
}

static HifSwdbPkg *_get_package_by_pid (    sqlite3 *db,
                                            const gint pid)
{
    sqlite3_stmt *res;
    const gchar *sql = S_PACKAGE_BY_PID;
    DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@pid", pid);
    if (sqlite3_step(res) == SQLITE_ROW)
    {
        HifSwdbPkg *pkg = hif_swdb_pkg_new(
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
        pkg->type = _look_for_desc(db, "PACKAGE_TYPE", type);
        return pkg;
    }
    sqlite3_finalize(res);
    return NULL;
}

/**
* hif_swdb_get_packages_by_tid:
*
* Returns: (element-type HifSwdbPkg)(array)(transfer container): list of #HifSwdbPkg
*/
GPtrArray *hif_swdb_get_packages_by_tid(   HifSwdb *self,
                                            const gint tid)
{
    if (hif_swdb_open(self))
    	return NULL;
    DB_TRANS_BEGIN

    GPtrArray *node = g_ptr_array_new();
    GArray *pids = _pids_by_tid(self->db, tid);
    gint pid;
    HifSwdbPkg *pkg;
    for(guint i = 0; i < pids->len ;i++)
    {
        pid = g_array_index(pids, gint, i);
        pkg = _get_package_by_pid(self->db, pid);
        if(pkg)
        {
            pkg->swdb = self;
            _resolve_package_state(self, pkg);
            g_ptr_array_add(node, (gpointer) pkg);
        }
    }
    DB_TRANS_END
    hif_swdb_close(self);
    return node;
}

const gchar* hif_swdb_pkg_ui_from_repo	( HifSwdbPkg *self)
{
    if(self->ui_from_repo)
        return g_strdup(self->ui_from_repo);
    if(!self->swdb || !self->pid || hif_swdb_open(self->swdb))
        return "(unknown)";
    sqlite3_stmt *res;
    gchar *sql = (gchar*)S_REPO_FROM_PID;
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
        sql = (gchar*)S_RELEASEVER_FROM_PDID;
        DB_PREP(self->swdb->db, sql, res);
        DB_BIND_INT(res, "@pdid", pdid);
        if(sqlite3_step(res) == SQLITE_ROW)
        {
            cur_releasever = g_strdup((const gchar*)sqlite3_column_text(res, 0));
        }
        sqlite3_finalize(res);
        if(g_strcmp0(cur_releasever, self->swdb->releasever))
        {
            hif_swdb_close(self->swdb);
            return g_strjoin(NULL, "@", r_name, "/", cur_releasever, NULL);
        }
    }
    hif_swdb_close(self->swdb);
    if(r_name)
        return r_name;
    else
        return "(unknown)";
}

/**
* hif_swdb_package_by_pattern:
* Returns: (transfer full): #HifSwdbPkg
*/
HifSwdbPkg *hif_swdb_package_by_pattern (   HifSwdb *self,
                                            const gchar *pattern)
{
    if (hif_swdb_open(self))
        return NULL;
    gint pid = _pid_by_nvra(self->db, pattern);
    if (!pid)
    {
        hif_swdb_close(self);
        return NULL;
    }
    HifSwdbPkg *pkg = _get_package_by_pid(self->db, pid);
    hif_swdb_close(self);
    return pkg;
}

/**
* hif_swdb_package_data_by_pattern:
* Returns: (transfer full): #HifSwdbPkgData
*/
HifSwdbPkgData *hif_swdb_package_data_by_pattern (  HifSwdb *self,
                                                    const gchar *pattern)
{
    if (hif_swdb_open(self))
        return NULL;
    gint pid = _pid_by_nvra(self->db, pattern);
    if (!pid)
    {
        hif_swdb_close(self);
        return NULL;
    }
    HifSwdbPkgData *pkgdata = _get_package_data_by_pid(self->db, pid);
    hif_swdb_close(self);
    return pkgdata;
}

static const gint _mark_pkg_as (   sqlite3 *db,
                            const gint pdid,
                            gint mark)
{
    sqlite3_stmt *res;
    const gchar *sql = U_REASON_BY_PDID;
    DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@reason", mark);
    DB_BIND_INT(res, "@pdid", pdid);
    DB_STEP(res);
    return 0;
}

const gint hif_swdb_set_reason (    HifSwdb *self,
                                    const gchar *nvra,
                                    const gchar *reason)
{
    if (hif_swdb_open(self))
        return 1;
    DB_TRANS_BEGIN
    gint pid = _pid_by_nvra(self->db, nvra);
    if (!pid)
    {
        hif_swdb_close(self);
        return 1;
    }
    const gint pdid = _pdid_from_pid(self->db, pid);
    gint rc = 1;
    gint reason_id;
    reason_id = hif_swdb_get_reason_type( self, reason);
    rc = _mark_pkg_as(self->db, pdid, reason_id);
    DB_TRANS_END
    hif_swdb_close(self);
    return rc;
}

const gint hif_swdb_mark_user_installed (   HifSwdb *self,
                                            const gchar *pattern,
                                            gboolean user_installed)
{
    gint rc;
    if(user_installed)
    {
        rc = hif_swdb_set_reason(self, pattern, "user");
    }
    else
    {
        rc = hif_swdb_set_reason(self, pattern, "dep");
    }
    return rc;
}

/**************************** RPM DATA PERSISTOR *****************************/

static gint _rpm_data_insert (sqlite3 *db, struct rpm_data_t *rpm_data)
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

gint 	hif_swdb_log_rpm_data(	HifSwdb *self,
									const gint   pid,
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
  	if (hif_swdb_open(self))
    	return 1;

  	struct rpm_data_t rpm_data = {  pid, buildtime, buildhost, license, packager,
                                    size, sourcerpm, url, vendor, committer, committime};


  	gint rc = _rpm_data_insert(self->db, &rpm_data);
  	hif_swdb_close(self);
  	return rc;
}


/*************************** TRANS DATA PERSISTOR ****************************/

static gint _trans_data_beg_insert	(sqlite3 *db, struct trans_data_beg_t *trans_data_beg)
{
  	sqlite3_stmt *res;
  	const gchar *sql = INSERT_TRANS_DATA_BEG;
  	DB_PREP(db, sql, res);
  	DB_BIND_INT(res, "@tid", trans_data_beg->tid);
  	DB_BIND_INT(res, "@pdid", trans_data_beg->pdid);
  	DB_BIND_INT(res, "@done", 0);
  	DB_BIND_INT(res, "@reason", trans_data_beg->reason);
  	DB_BIND_INT(res, "@state", trans_data_beg->state);
  	return DB_FIND(res);
}

gint 	hif_swdb_trans_data_beg	(	HifSwdb *self,
									const gint 	 tid,
									const gint 	 pid,
									const gchar *reason,
									const gchar *state )
{
  	if (hif_swdb_open(self))
    	return 1;
  	DB_TRANS_BEGIN
    const gint pdid = _pdid_from_pid(self->db, pid);
 	struct trans_data_beg_t trans_data_beg = {tid, pdid,
	  	hif_swdb_get_reason_type(self, reason), hif_swdb_get_state_type(self,state)};
  	gint rc = _trans_data_beg_insert(self->db, &trans_data_beg);
  	DB_TRANS_END
  	hif_swdb_close(self);
  	return rc;
}

static gint _trans_data_end_update( sqlite3 *db,
								    const gint tid)
{
  	sqlite3_stmt *res;
  	const gchar *sql = UPDATE_TRANS_DATA_END;
  	DB_PREP(db, sql, res);
  	DB_BIND_INT(res, "@done", 1);
  	DB_BIND_INT(res, "@tid", tid);
  	DB_STEP(res);
  	return 0;
}

/*
 * Mark all packages from transaction as done
 */
gint 	hif_swdb_trans_data_end	(	HifSwdb *self,
									const gint tid)
{
  	if (hif_swdb_open(self))
    	return 1;
  	gint rc = _trans_data_end_update(self->db, tid);
  	hif_swdb_close(self);
  	return rc;
}

/*
 * Mark single package from transaction as done
 */
gint    hif_swdb_trans_data_pid_end (   HifSwdb *self,
                                        const gint pid,
                                        const gint tid,
                                        const gchar *state)
{
    if (hif_swdb_open(self))
    	return 1;
    DB_TRANS_BEGIN

    const gint pdid = _pdid_from_pid(self->db, pid);
    const gint _state = hif_swdb_get_state_type(self,state);
    sqlite3_stmt *res;
  	const gchar *sql = UPDATE_TRANS_DATA_PID_END;
  	DB_PREP(self->db, sql, res);
  	DB_BIND_INT(res, "@done", 1);
  	DB_BIND_INT(res, "@tid", tid);
    DB_BIND_INT(res, "@pdid", pdid);
    DB_BIND_INT(res, "@state", _state);
  	DB_STEP(res);

    DB_TRANS_END
    hif_swdb_close(self);
    return 0;
}


/****************************** TRANS PERSISTOR ******************************/

static gint _trans_beg_insert(sqlite3 *db, struct trans_beg_t *trans_beg)
{
  	sqlite3_stmt *res;
  	const gchar *sql = INSERT_TRANS_BEG;
  	DB_PREP(db,sql,res);
	DB_BIND(res, "@beg", trans_beg->beg_timestamp);
  	DB_BIND(res, "@rpmdbv", trans_beg->rpmdb_version );
  	DB_BIND(res, "@cmdline", trans_beg->cmdline);
  	DB_BIND(res, "@loginuid", trans_beg->loginuid);
  	DB_BIND(res, "@releasever", trans_beg->releasever);
  	DB_STEP(res);
  	return sqlite3_last_insert_rowid(db);
}

gint 	hif_swdb_trans_beg 	(	HifSwdb *self,
							 	const gchar *timestamp,
							 	const gchar *rpmdb_version,
								const gchar *cmdline,
								const gchar *loginuid,
								const gchar *releasever)
{
	if (hif_swdb_open(self))
    	return 1;
  	struct trans_beg_t trans_beg = { timestamp, rpmdb_version, cmdline, loginuid, releasever};
  	gint rc = _trans_beg_insert(self->db, &trans_beg);
  	hif_swdb_close(self);
  	return rc;
}

static gint _trans_end_insert(sqlite3 *db, struct trans_end_t *trans_end)
{
  	sqlite3_stmt *res;
  	const gchar *sql = INSERT_TRANS_END;
  	DB_PREP(db,sql,res);
  	DB_BIND_INT(res, "@tid", trans_end->tid );
	DB_BIND(res, "@end", trans_end->end_timestamp );
    DB_BIND(res, "@rpmdbv", trans_end->end_rpmdb_version);
  	DB_BIND_INT(res, "@rc", trans_end->return_code );
  	DB_STEP(res);
  	return 0;
}

gint 	hif_swdb_trans_end 	(	HifSwdb *self,
							 	const gint tid,
							 	const gchar *end_timestamp,
                                const gchar *end_rpmdb_version,
								const gint return_code)
{
	if (hif_swdb_open(self))
    	return 1;
  	struct trans_end_t trans_end = { tid, end_timestamp, end_rpmdb_version, return_code};
  	gint rc = _trans_end_insert(self->db, &trans_end);
  	hif_swdb_close(self);
  	return rc;
}

const gchar *hif_swdb_trans_cmdline (   HifSwdb *self,
							 	        const gint tid)
{
	if (hif_swdb_open(self))
    	return NULL;
    sqlite3_stmt *res;
    const gchar *sql = GET_TRANS_CMDLINE;
    DB_PREP(self->db, sql, res);
    DB_BIND_INT(res, "@tid", tid);
    const gchar *cmdline = DB_FIND_STR(res);
    hif_swdb_close(self);
  	return cmdline;
}

static void _resolve_altered    (GPtrArray *trans)
{
    for(guint i = 0; i < (trans->len-1); i++)
    {
        HifSwdbTrans *las = (HifSwdbTrans *)g_ptr_array_index(trans, i);
        HifSwdbTrans *obj = (HifSwdbTrans *)g_ptr_array_index(trans, i+1);

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

static gboolean _test_output (HifSwdb *self, gint tid, const gchar *o_type)
{
    const gint type = hif_swdb_get_output_type(self, o_type);
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


static void _resolve_outputs (HifSwdb *self, GPtrArray *trans)
{
    for(guint i = 0; i < trans->len; ++i)
    {
        HifSwdbTrans *obj = (HifSwdbTrans *)g_ptr_array_index(trans, i);
        obj->is_output = _test_output(self, obj->tid, "stdout");
        obj->is_error = _test_output(self, obj->tid, "stderr");
    }
}

/**
* hif_swdb_get_old_trans_data:
* Returns: (element-type HifSwdbTransData)(array)(transfer container): list of #HifSwdbTransData
*/
GPtrArray *hif_swdb_get_old_trans_data (    HifSwdb *self,
                                            HifSwdbTrans *trans)
{
    if (!trans->tid)
        return NULL;
    if (hif_swdb_open(self))
    	return NULL;
    GPtrArray *node = g_ptr_array_new();
    sqlite3_stmt *res;
    const gchar *sql = S_TRANS_DATA_BY_TID;
    DB_PREP(self->db, sql, res);
    DB_BIND_INT(res, "@tid", trans->tid);
    while(sqlite3_step(res) == SQLITE_ROW)
    {
        HifSwdbTransData *data = hif_swdb_transdata_new(
                                    sqlite3_column_int(res, 0), //td_id
                                    sqlite3_column_int(res, 1), //t_id
                                    sqlite3_column_int(res, 2), //pd_id
                                    sqlite3_column_int(res, 3), //g_id
                                    sqlite3_column_int(res, 4), //done
                                    sqlite3_column_int(res, 5), //ORIGINAL_TD_ID
                                    // Is this allright?
                                    _look_for_desc(self->db, "REASON_TYPE", sqlite3_column_int(res,6)), //reason
                                    _look_for_desc(self->db, "STATE_TYPE", sqlite3_column_int(res,7))); //state
        g_ptr_array_add(node, (gpointer) data);
    }
    sqlite3_finalize(res);
    hif_swdb_close(self);
    return node;
}

/**
* hif_swdb_trans_get_old_trans_data:
* Returns: (element-type HifSwdbTransData)(array)(transfer container): list of #HifSwdbTransData
*/
GPtrArray *hif_swdb_trans_get_old_trans_data(HifSwdbTrans *self)
{
    if (self->swdb)
        return hif_swdb_get_old_trans_data( self->swdb, self);
    else
        return NULL;
}

/**
* hif_swdb_trans_old:
* @tids: (element-type gint32)(transfer container): list of constants
* Returns: (element-type HifSwdbTrans)(array)(transfer container): list of #HifSwdbTrans
*/
GPtrArray *hif_swdb_trans_old(	HifSwdb *self,
                                GArray *tids,
								gint limit,
								const gboolean complete_only)
{
    if (hif_swdb_open(self))
    	return NULL;
    DB_TRANS_BEGIN
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
        HifSwdbTrans *trans = hif_swdb_trans_new(   tid, //tid
                                                    (gchar *)sqlite3_column_text (res, 1), //beg_t
                                                    (gchar *)sqlite3_column_text (res, 2), //end_t
                                                    (gchar *)sqlite3_column_text (res, 3), //beg_rpmdb_v
                                                    (gchar *)sqlite3_column_text (res, 4), //end_rpmdb_v
                                                    (gchar *)sqlite3_column_text (res, 5), //cmdline
                                                    (gchar *)sqlite3_column_text (res, 6), //loginuid
                                                    (gchar *)sqlite3_column_text (res, 7), //releasever
                                                    sqlite3_column_int  (res, 8)); // return_code
        trans->swdb = self;
        g_ptr_array_add(node, (gpointer) trans);
    }
    sqlite3_finalize(res);
    _resolve_altered(node);
    _resolve_outputs(self,node);
    DB_TRANS_END
    hif_swdb_close(self);
    return node;
}

/**
* hif_swdb_last:
* Returns: (transfer full): #HifSwdbTrans
**/
HifSwdbTrans *hif_swdb_last (HifSwdb *self)
{
    GPtrArray *node = hif_swdb_trans_old(self, NULL, 1, 1);
    if (!node || !node->len)
        return NULL;
    HifSwdbTrans *trans = g_ptr_array_index(node, 0);
    return trans;
}


/****************************** OUTPUT PERSISTOR *****************************/

static gint _output_insert(sqlite3 *db, struct output_t *output)
{
  	sqlite3_stmt *res;
  	const gchar *sql = INSERT_OUTPUT;
  	DB_PREP(db,sql,res);
  	DB_BIND_INT(res, "@tid", output->tid);
  	DB_BIND(res, "@msg", output->msg);
  	DB_BIND_INT(res, "@type", output->type);
  	DB_STEP(res);
  	return 0;
}

gint hif_swdb_log_error	(	HifSwdb *self,
						 	const gint tid,
							const gchar *msg)
{
  	if (hif_swdb_open(self))
    	return 1;

  	DB_TRANS_BEGIN

  	struct output_t output = { tid, msg, hif_swdb_get_output_type(self, "stderr") };
  	gint rc = _output_insert( self->db, &output);

  	DB_TRANS_END
  	hif_swdb_close(self);
  	return rc;
}

gint hif_swdb_log_output	(	HifSwdb *self,
						 	    const gint tid,
							    const gchar *msg)
{
  	if (hif_swdb_open(self))
    	return 1;

  	DB_TRANS_BEGIN

  	struct output_t output = { tid, msg, hif_swdb_get_output_type(self, "stdout") };
  	gint rc = _output_insert( self->db, &output);

  	DB_TRANS_END
  	hif_swdb_close(self);
  	return rc;
}

static GPtrArray *_load_output (       sqlite3 *db,
                                    const gint tid,
                                    const gint type)
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
        g_ptr_array_add(l, g_strdup(row));
    }
    return l;
}

/**
* hif_swdb_load_error:
*
* Returns: (element-type utf8) (transfer container): list of constants
*/
GPtrArray *hif_swdb_load_error (       HifSwdb *self,
                                    const gint tid)
{
    if (hif_swdb_open(self))
    	return NULL;
    DB_TRANS_BEGIN
    GPtrArray *rc = _load_output(      self->db,
                                    tid,
                                    hif_swdb_get_output_type(self, "stderr"));
    DB_TRANS_END
    hif_swdb_close(self);
    return rc;
}

/**
* hif_swdb_load_output:
*
* Returns: (element-type utf8) (transfer container): list of constants
*/
GPtrArray *hif_swdb_load_output (      HifSwdb *self,
                                    const gint tid)
{
    if (hif_swdb_open(self))
    	return NULL;
    DB_TRANS_BEGIN
    GPtrArray *rc = _load_output( self->db,
                                    tid,
                                    hif_swdb_get_output_type(self, "stdout"));
    DB_TRANS_END
    hif_swdb_close(self);
    return rc;
}


/****************************** TYPE BINDERS *********************************/

/* Finds ID by description in @table
 * Returns ID for description or 0 if not found
 * Requires opened DB
 */
static gint _find_match_by_desc(sqlite3 *db, const gchar *table, const gchar *desc)
{
    sqlite3_stmt *res;
    const gchar *sql = g_strjoin(" ","select ID from",table,"where description=@desc", NULL);

    DB_PREP(db,sql,res);
    DB_BIND(res, "@desc", desc);
    return DB_FIND(res);
}

/* Inserts description into @table
 * Returns 0 if successfull
 * Requires opened DB
 */
static gint _insert_desc(sqlite3 *db, const gchar *table, const gchar *desc)
{
    sqlite3_stmt *res;
    gchar *sql = g_strjoin(" ","insert into",table,"values (null, @desc)", NULL);
  	DB_PREP(db, sql, res);
    DB_BIND(res, "@desc", desc);
	DB_STEP(res);
  	return sqlite3_last_insert_rowid(db);
}

static gchar* _look_for_desc(sqlite3 *db, const gchar *table, const gint id)
{
    sqlite3_stmt *res;
    gchar *sql = g_strjoin(" ","select description from",table,"where ID=@id", NULL);
  	DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@id", id);
    return DB_FIND_STR(res);
}

/* Bind description to id in chosen table
 * Usage: _bind_desc_id(db, table, description)
 * Requires opened DB
 */
static gint _bind_desc_id(sqlite3 *db, const gchar *table, const gchar *desc)
{
    gint id = _find_match_by_desc(db,table,desc);
  	if(id) //found or error
	{
	  	return id;
	}
    else // id for desc not found, try to add it
    {
        return _insert_desc(db,table,desc);
    }
}


/* Binder for PACKAGE_TYPE
 * Returns: ID of description in PACKAGE_TYPE table
 * Usage: hif_swdb_get_package_type( HifSwdb, description)
 */
gint hif_swdb_get_package_type (HifSwdb *self, const gchar *type)
{

 	if(hif_swdb_open(self))
 		return -1;
  	gint rc = _bind_desc_id(self->db, "PACKAGE_TYPE", type);
	hif_swdb_close(self);
  	return rc;
}

/* OUTPUT_TYPE binder
 * Returns: ID of description in OUTPUT_TYPE table
 * Usage: hif_swdb_get_output_type( HifSwdb, description)
 */
gint hif_swdb_get_output_type (HifSwdb *self, const gchar *type)
{
	if(hif_swdb_open(self))
 		return -1;
  	gint rc = _bind_desc_id(self->db, "OUTPUT_TYPE", type);
  	hif_swdb_close(self);
  	return rc;
}

/* REASON_TYPE binder
 * Returns: ID of description in REASON_TYPE table
 * Usage: hif_swdb_get_reason_type( HifSwdb, description)
 */
gint hif_swdb_get_reason_type (HifSwdb *self, const gchar *type)
{

	if(hif_swdb_open(self))
 		return -1;
  	gint rc = _bind_desc_id(self->db, "REASON_TYPE", type);
  	hif_swdb_close(self);
  	return rc;
}

/* STATE_TYPE binder
 * Returns: ID of description in STATE_TYPE table
 * Usage: hif_swdb_get_state_type( HifSwdb, description)
 */
gint hif_swdb_get_state_type (HifSwdb *self, const gchar *type)
{

	if(hif_swdb_open(self))
 		return -1;
  	gint rc = _bind_desc_id(self->db, "STATE_TYPE", type);
  	hif_swdb_close(self);
  	return rc;
}

/******************************* Database operators ************************************/

//returns path to swdb, default is /var/lib/dnf/swdb.sqlite
const gchar   *hif_swdb_get_path  (HifSwdb *self)
{
    return self->path;
}

//changes path to swdb
void  hif_swdb_set_path   (HifSwdb *self, const gchar *path)
{
    if(g_strcmp0(path,self->path) != 0)
    {
        hif_swdb_close(self);
	    if (self->path_changed)
		{
		  g_free(self->path);
		}
	  	self->path_changed = 1;
		self->path = g_strdup(path);
    }
}

//TRUE if db exists else false
gboolean hif_swdb_exist(HifSwdb *self)
{
    return g_file_test(hif_swdb_get_path (self),G_FILE_TEST_EXISTS);
}

//open database at self->path
gint hif_swdb_open(HifSwdb *self)
{

    if(self->ready)
    return 0;

    if( sqlite3_open(hif_swdb_get_path (self), &self->db))
    {
        fprintf(stderr, "ERROR: %s (try again as root)\n", sqlite3_errmsg(self->db));
        return 1;
    }

    self->ready = 1;
    return 0;
}

void hif_swdb_close(HifSwdb *self)
{
    if( self->ready && !self->running )
    {
        sqlite3_close(self->db);
        self->ready = 0;
    }
}

//create db at path
gint hif_swdb_create_db (HifSwdb *self)
{

    if (hif_swdb_open(self))
    	return 1;

    // Create all tables
    gint failed = 0;
    failed += _db_exec (self->db, C_PKG_DATA, NULL);
    failed += _db_exec (self->db, C_PKG, NULL);
    failed += _db_exec (self->db, C_REPO, NULL);
    failed += _db_exec (self->db, C_TRANS_DATA, NULL);
    failed += _db_exec (self->db, C_TRANS, NULL);
    failed += _db_exec (self->db, C_OUTPUT, NULL);
    failed += _db_exec (self->db, C_STATE_TYPE, NULL);
    failed += _db_exec (self->db, C_REASON_TYPE, NULL);
    failed += _db_exec (self->db, C_OUTPUT_TYPE, NULL);
    failed += _db_exec (self->db, C_PKG_TYPE, NULL);
    failed += _db_exec (self->db, C_GROUPS, NULL);
    failed += _db_exec (self->db, C_T_GROUP_DATA, NULL);
    failed += _db_exec (self->db, C_GROUPS_PKG, NULL);
    failed += _db_exec (self->db, C_GROUPS_EX, NULL);
    failed += _db_exec (self->db, C_ENV_GROUPS, NULL);
    failed += _db_exec (self->db, C_ENV, NULL);
    failed += _db_exec (self->db, C_ENV_EX, NULL);
    failed += _db_exec (self->db, C_RPM_DATA, NULL);
    failed += _db_exec (self->db, C_INDEX_NVRA, NULL);

  	if (failed != 0)
    {
        fprintf(stderr, "SQL error: Unable to create %d tables\n",failed);
        sqlite3_close(self->db);
        return 2;
    }

    hif_swdb_close(self);
    return 0;
}

//remove and init empty db
gint hif_swdb_reset_db (HifSwdb *self)
{

    if(hif_swdb_exist(self))
    {
        hif_swdb_close(self);
        if (g_remove(self->path)!= 0)
        {
            fprintf(stderr,"SWDB error: Could not reset database (try again as root)\n");
            return 1;
        }
    }
    return hif_swdb_create_db(self);
}
