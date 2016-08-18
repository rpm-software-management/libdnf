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

/* TODO: _old_data_pkgs for TRANS
 *
 *
 */


#define default_path "/var/lib/dnf/swdb.sqlite"

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
  	gboolean path_changed; //dont forget to free memory...
  	gboolean running; //if true, db will not be closed
};

G_DEFINE_TYPE(HifSwdb, hif_swdb, G_TYPE_OBJECT)
G_DEFINE_TYPE(HifSwdbPkg, hif_swdb_pkg, G_TYPE_OBJECT) //history package
G_DEFINE_TYPE(HifSwdbTrans, hif_swdb_trans, G_TYPE_OBJECT) //history transaction

/* Table structs */
struct package_t
{
  	const gchar *name;
  	const gchar *epoch;
  	const gchar *version;
  	const gchar *release;
  	const gchar *arch;
  	const gchar *checksum_data;
  	const gchar *checksum_type;
  	gint type;
};

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
  	const gint return_code;
};

struct package_data_t
{
  	const gint   pid;
    const gint   rid;
    const gchar *from_repo_revision;
  	const gchar *from_repo_timestamp;
  	const gchar *installed_by;
  	const gchar *changed_by;
  	const gchar *installonly;
  	const gchar *origin_url;
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
}

/**
 * hif_swdb_new:
 *
 * Creates a new #HifSwdb.
 *
 * Returns: a #HifSwdb
 **/
HifSwdb* hif_swdb_new(void)
{
    HifSwdb *swdb = g_object_new(HIF_TYPE_SWDB, NULL);
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
									const gchar *rpmdb_version,
									const gchar *cmdline,
									const gchar *loginuid,
									const gchar *releasever,
									const gint return_code)
{
    HifSwdbTrans *trans = g_object_new(HIF_TYPE_SWDB_TRANS, NULL);
    trans->tid = tid;
    trans->beg_timestamp = g_strdup(beg_timestamp);
    trans->end_timestamp = g_strdup(end_timestamp);
    trans->rpmdb_version = g_strdup(rpmdb_version);
    trans->cmdline = g_strdup(cmdline);
    trans->loginuid = g_strdup(loginuid);
    trans->releasever = g_strdup(releasever);
    trans->return_code = return_code;
  	return trans;
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
    else if (!g_strcmp0(attr,"RPMDB_version")) table = 2;
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

static GSList *_get_subpatterns(const gchar* pattern)
{
    GSList *subpatterns = NULL;
    subpatterns = g_slist_append (subpatterns, g_strdup(pattern));
    const gchar * pattern_r = g_utf8_strreverse(pattern, -1);
    gint delimiters1 = 1;
    gint delimiters2 = 1;
    for (int i = 0; pattern[i]; ++i)
    {
        if (pattern_r[i] == '-') //there will be substring for each delimiter
        {
            delimiters1++;
            gchar **subs = g_strsplit(pattern_r,"-",delimiters1);
            subpatterns = g_slist_append (subpatterns, g_utf8_strreverse(subs[g_strv_length (subs)-1],-1));
        }
        if (pattern_r[i] == '.') //there will be substring for each delimiter
        {
            delimiters2++;
            gchar **subs = g_strsplit(pattern_r,".",delimiters2);
            subpatterns = g_slist_append (subpatterns, g_utf8_strreverse(subs[g_strv_length (subs)-1],-1));
        }
    }
    return subpatterns;
}

static GSList *_simple_search(sqlite3* db, const gchar * pattern)
{
    GSList *tids = NULL;
    sqlite3_stmt *res_simple;
    const gchar *sql_simple = SIMPLE_SEARCH;
    DB_PREP(db, sql_simple, res_simple);
    DB_BIND(res_simple, "@pat", pattern);
    gint pid_simple;
    GSList *simple = NULL;
    while( (pid_simple = DB_FIND_MULTI(res_simple)))
    {
        simple = g_slist_append (simple, GINT_TO_POINTER(pid_simple));
    }
    while(simple)
    {
        pid_simple = GPOINTER_TO_INT(simple->data);
        simple = simple->next;
        GSList *pdids = _all_pdid_for_pid(db, pid_simple);
        while(pdids)
        {
            gint pdid = GPOINTER_TO_INT(pdids->data);
            pdids = pdids->next;
            gint tid = _tid_from_pdid(db, pdid);
            if(tid)
            {
                tids = g_slist_append (tids, GINT_TO_POINTER(tid));
            }
        }
    }
    return tids;
}

static GSList *_extended_search (sqlite3* db, const gchar *pattern)
{
    GSList *tids = NULL;
    //split pattern into subpatterns - minimalising data processed
    GSList *subpatterns = _get_subpatterns(pattern);
    gchar *best_match = NULL;
    gint best_match_count = 0;
    while(subpatterns)
    {
        const gchar *subpattern = subpatterns->data;
        subpatterns = subpatterns->next;

        //find best subpattern
        sqlite3_stmt *res;
        const gchar *sql = FIND_PIDS_BY_NAME;
        DB_PREP( db, sql, res);
        DB_BIND(res, "@pattern", subpattern);
        gint pid;
        gint matches = 0;
        while ((pid = DB_FIND_MULTI(res)))
        {
            matches++;
        }
        if (matches > 0)
        {
            if(best_match_count == 0 || matches < best_match_count)
            {
                best_match_count = matches;
                best_match = (gchar *)subpattern;
            }
        }
    }
    sqlite3_stmt *res;
    const gchar *sql = SEARCH_PATTERN;
    DB_PREP( db, sql, res);
    //lot of patterns...
    DB_BIND(res, "@sub", best_match);
    DB_BIND(res, "@pat", pattern);
    DB_BIND(res, "@pat", pattern);
    DB_BIND(res, "@pat", pattern);
    DB_BIND(res, "@pat", pattern);
    DB_BIND(res, "@pat", pattern);
    DB_BIND(res, "@pat", pattern);
    gint pid = DB_FIND(res);
    if(pid)
    {
        GSList *pdids = _all_pdid_for_pid(db, pid);
        while(pdids)
        {
            gint pdid = GPOINTER_TO_INT(pdids->data);
            pdids = pdids->next;
            gint tid = _tid_from_pdid(db, pdid);
            if(tid)
            {
                tids = g_slist_append (tids, GINT_TO_POINTER(tid));
            }
        }
    }
    return tids;
}

/**
* hif_swdb_search:
* @patterns: (element-type utf8) (transfer container): list of constants
* Returns: (element-type gint32) (transfer container): list of constants
*/
GSList *hif_swdb_search (   HifSwdb *self,
                            const GSList *patterns)
{
    if (hif_swdb_open(self))
        return NULL;
    DB_TRANS_BEGIN

    GSList *tids = NULL;
    while(patterns)
    {
        const gchar *pattern = patterns->data;
        patterns = patterns->next;

        //try simple search
        GSList *simple =  _simple_search(self->db, pattern);
        if(simple)
        {
            tids = g_slist_concat(tids,simple);
            continue;
        }
        //need for extended search
        GSList *extended = _extended_search(self->db, pattern);
        if(extended)
        {
            tids = g_slist_concat(tids, extended);
        }
    }
    DB_TRANS_END
    hif_swdb_close(self);
    return tids;
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

//add new group package
gint hif_swdb_add_group_package (HifSwdb *self, gint gid, const gchar *name)
{
    if (hif_swdb_open(self))
    return 1;

    gint rc = _insert_id_name (self->db, "GROUPS_PACKAGE", gid, name);
	hif_swdb_close(self);
  	return rc;
}

//add new group exclude
gint hif_swdb_add_group_exclude (HifSwdb *self, gint gid, const gchar *name)
{

    if (hif_swdb_open(self))
    return 1;

    gint rc = _insert_id_name (self->db, "GROUPS_EXCLUDE", gid, name);
	hif_swdb_close(self);
  	return rc;
}

//add new environments exclude
gint hif_swdb_add_environments_exclude (HifSwdb *self, gint eid, const gchar *name)
{

    if (hif_swdb_open(self))
    return 1;

    gint rc = _insert_id_name (self->db, "ENVIRONMENTS_EXCLUDE", eid, name);
	hif_swdb_close(self);
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

/**************************** PACKAGE PERSISTOR ******************************/

/**
 * _package_insert: (skip)
 * Insert package into database
 * @db - database
 * @package - package meta struct
 **/
static gint _package_insert(sqlite3 *db, struct package_t *package)
{
    sqlite3_stmt *res;
   	const gchar *sql = INSERT_PKG;
	DB_PREP(db,sql,res);
  	DB_BIND(res, "@name", package->name);
  	DB_BIND(res, "@epoch", package->epoch);
  	DB_BIND(res, "@version", package->version);
  	DB_BIND(res, "@release", package->release);
  	DB_BIND(res, "@arch", package->arch);
  	DB_BIND(res, "@cdata", package->checksum_data);
  	DB_BIND(res, "@ctype", package->checksum_type);
  	DB_BIND_INT(res, "@type", package->type);
	DB_STEP(res);
  	return sqlite3_last_insert_rowid(db);
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
 	struct package_t package = {name, epoch, version, release, arch,
		checksum_data, checksum_type, hif_swdb_get_package_type(self,type)};

  	gint rc = _package_insert(self->db, &package);
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

static gint _package_data_update (sqlite3 *db, struct package_data_t *package_data)
{
  	sqlite3_stmt *res;
   	const gchar *sql = UPDATE_PKG_DATA;
	DB_PREP(db,sql,res);
  	DB_BIND_INT(res, "@pid", package_data->pid);
  	DB_BIND_INT(res, "@rid", package_data->rid);
  	DB_BIND(res, "@repo_r", package_data->from_repo_revision);
  	DB_BIND(res, "@repo_t", package_data->from_repo_timestamp);
  	DB_BIND(res, "@installed_by", package_data->installed_by);
  	DB_BIND(res, "@changed_by", package_data->changed_by);
  	DB_BIND(res, "@installonly", package_data->installonly);
  	DB_BIND(res, "@origin_url", package_data->origin_url);
	DB_STEP(res);
  	return 0;
}

gint 	hif_swdb_log_package_data(	HifSwdb *self,
									const gint   pid,
                                  	const gchar *from_repo,
                                  	const gchar *from_repo_revision,
                                  	const gchar *from_repo_timestamp,
                                  	const gchar *installed_by,
                                  	const gchar *changed_by,
                                  	const gchar *installonly,
                                  	const gchar *origin_url )
{
  	if (hif_swdb_open(self))
    	return 1;

  	struct package_data_t package_data = { pid, _bind_repo_by_name(self->db, from_repo),
	  from_repo_revision, from_repo_timestamp, installed_by, changed_by, installonly, origin_url};

  	gint rc = _package_data_update(self->db, &package_data);

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

static GSList* _all_pdid_for_pid (	sqlite3 *db,
								    const gint pid )
{
    GSList *pdids = NULL;
    sqlite3_stmt *res;
  	const gchar *sql = FIND_ALL_PDID_FOR_PID;
  	DB_PREP(db, sql, res);
  	DB_BIND_INT(res, "@pid", pid);
    gint pdid;
    while( (pdid = DB_FIND_MULTI(res)))
    {
        pdids = g_slist_append (pdids, GINT_TO_POINTER(pdid));
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

static GSList *_pids_by_tid (    sqlite3 *db,
                                const gint tid)
{
    GSList *pids = NULL;
    sqlite3_stmt *res;
    const gchar* sql = PID_BY_TID;
    DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@tid", tid);
    gint pid;
    while ( (pid = DB_FIND_MULTI(res)))
    {
        pids = g_slist_append(pids, GINT_TO_POINTER(pid));
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
            (gchar *)sqlite3_column_text(res, 0), //name
            (gchar *)sqlite3_column_text(res, 1), //epoch
            (gchar *)sqlite3_column_text(res, 2), //version
            (gchar *)sqlite3_column_text(res, 3), //release
            (gchar *)sqlite3_column_text(res, 4), //arch
            (gchar *)sqlite3_column_text(res, 5), //checksum_data
            (gchar *)sqlite3_column_text(res, 6), //checksum_type
            NULL //type for now
        );
        gint type = sqlite3_column_int(res, 7); //unresolved type
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
    GSList *pids = _pids_by_tid(self->db, tid);
    gint pid;
    HifSwdbPkg *pkg;
    while (pids)
    {
        pid = GPOINTER_TO_INT(pids->data);
        pids = pids->next;
        pkg = _get_package_by_pid(self->db, pid);
        if(pkg)
        {
            g_ptr_array_add(node, (gpointer) pkg);
        }
    }
    DB_TRANS_END
    hif_swdb_close(self);
    return node;
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
  	DB_BIND_INT(res, "@rc", trans_end->return_code );
  	DB_STEP(res);
  	return 0;
}

gint 	hif_swdb_trans_end 	(	HifSwdb *self,
							 	const gint tid,
							 	const gchar *end_timestamp,
								const gint return_code)
{
	if (hif_swdb_open(self))
    	return 1;
  	struct trans_end_t trans_end = { tid, end_timestamp, return_code};
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
    for(int i = trans->len-1; i > 0; --i)
    {
        HifSwdbTrans *las = (HifSwdbTrans *)g_ptr_array_index(trans, i);
        HifSwdbTrans *obj = (HifSwdbTrans *)g_ptr_array_index(trans, i-1);
        if(g_strcmp0(las->rpmdb_version, obj->rpmdb_version)) //rpmdb_version changed
        {
            obj->altered_lt_rpmdb = 1;
            las->altered_gt_rpmdb = 1;
        }
    }
}

GPtrArray *hif_swdb_trans_get_old_trans_data(HifSwdbTrans *self)
{
    //TODO:
}

/**
* hif_swdb_trans_old:
* @tids: (element-type void)(transfer container): list of constants
* Returns: (element-type HifSwdbTrans)(array)(transfer container): list of #HifSwdbTrans
*/
GPtrArray *hif_swdb_trans_old(	HifSwdb *self,
                                GPtrArray *tids,
								gint limit,
								const gboolean complete_only)
{
    if (hif_swdb_open(self))
    	return NULL;
    GPtrArray *node = g_ptr_array_new();
    sqlite3_stmt *res;
    if(tids->len)
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
        if(tids->len)
        {
            match = 0;
            for(guint i = 0; i < tids->len; ++i)
            {
                if( tid == GPOINTER_TO_INT(g_ptr_array_index(tids,i)))
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
                                                    (gchar *)sqlite3_column_text (res, 3), //rpmdb_v
                                                    (gchar *)sqlite3_column_text (res, 4), //cmdline
                                                    (gchar *)sqlite3_column_text (res, 5), //loginuid
                                                    (gchar *)sqlite3_column_text (res, 6), //releasever
                                                    sqlite3_column_int  (res, 7)); // return_code
        g_ptr_array_add(node, (gpointer) trans);
    }
    _resolve_altered(node);
    sqlite3_finalize(res);
    hif_swdb_close(self);
    return node;
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
