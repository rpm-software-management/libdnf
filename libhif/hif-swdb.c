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

#define default_path "/var/lib/dnf/swdb.sqlite"

#define DB_PREP(db, sql, res) assert(_db_prepare(db, sql, &res))
#define DB_BIND(res, id, source) assert(_db_bind(res, id, source))
#define DB_BIND_INT(res, id, source) assert(_db_bind_int(res, id, source))
#define DB_STEP(res) assert(_db_step(res))
#define DB_FIND(res) _db_find(res)

// Leave DB open when in multi-insert transaction
#define DB_TRANS_BEGIN 	self->running = 1;
#define DB_TRANS_END	self->running = 0;

#define INSERT_PKG "insert into PACKAGE values(null,@name,@epoch,@version,@release,@arch,@cdata,@ctype,@type)"
#define INSERT_OUTPUT "insert into OUTPUT values(null,@tid,@msg,@type)"
#define INSERT_TRANS_BEG "insert into TRANS values(null,@beg,null,@rpmdbv,@cmdline,@loginuid,@releasever,null)"
#define INSERT_TRANS_END "UPDATE TRANS SET end_timestamp=@end,return_code=@rc WHERE T_ID=@tid"
#define INSERT_REPO "insert into REPO values(null,@name,null,null)"
#define INSERT_PKG_DATA "insert into PACKAGE_DATA values(null,@pid,@rid,@repo_r,@repo_t,@installed_by,@changed_by,@installonly,@origin_url)"
#define INSERT_TRANS_DATA_BEG "insert into TRANS_DATA values(null,@tid,@pdid,null,@done,null,@reason,@state)"
#define UPDATE_TRANS_DATA_END "UPDATE TRANS_DATA SET done=@done WHERE T_ID=@tid"

#define FIND_REPO_BY_NAME "SELECT R_ID FROM REPO WHERE name=@name"
#define FIND_PDID_FROM_PID "SELECT PD_ID FROM PACKAGE_DATA WHERE P_ID=@pid"

#include "hif-swdb.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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
  	const gchar *instalonly;
  	const gchar *origin_url;
};

struct trans_data_beg_t
{
  	const gint 	tid;
	const gint 	pdid;
  	const gint 	reason;
	const gint 	state;
};

// Destructor
static void hif_swdb_finalize(GObject *object)
{
    G_OBJECT_CLASS (hif_swdb_parent_class)->finalize (object);
}

// Class initialiser
static void
hif_swdb_class_init(HifSwdbClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = hif_swdb_finalize;

}

// Object initialiser
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

/******************************* Functions *************************************/

static gint _db_step(sqlite3_stmt *res)
{
  	if (sqlite3_step(res) != SQLITE_DONE)
    {
        fprintf(stderr, "SQL error: Could not execute statement in _db_step()\n");
        sqlite3_finalize(res);
        return 0;
	}
  	sqlite3_finalize(res);
  	return 1; //true because of assert
}

// assumes only one parameter on output e.g "SELECT ID FROM DUMMY WHERE ..."
static gint _db_find(sqlite3_stmt *res)
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


static gint _db_prepare(sqlite3 *db, const gchar *sql, sqlite3_stmt **res)
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

static gint _db_bind(sqlite3_stmt *res, const gchar *id, const gchar *source)
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

static gint _db_bind_int(sqlite3_stmt *res, const gchar *id, gint source)
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

static gint _db_exec(sqlite3 *db, const gchar *cmd, int (*callback)(void *, int, char **, char**))
{
    gchar *err_msg;
    gint result = sqlite3_exec(db, cmd, callback, 0, &err_msg);
    if ( result != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        return 1;
    }
    else
    {
        return 0;
    }
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
	  	return _bind_repo_by_name(db, name);
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
  	return 0;
}

gint hif_swdb_add_package_naevrcht(	HifSwdb *self,
				  					const gchar *name,
				  					const gchar *arch,
				  					const gchar *epoch,
				  					const gchar *version,
				  					const gchar *release,
				 					const gchar *checksum_data,
								   	const gchar *checksum_type,
								  	const gchar *type)
{
  	if (hif_swdb_open(self))
    	return 1;
  	DB_TRANS_BEGIN

  	//transform data into nevra format
 	struct package_t package = {name, epoch, version, release, arch,
		checksum_data, checksum_type, hif_swdb_get_package_type(self,type)};

  	gint rc = _package_insert(self->db, &package);
  	DB_TRANS_END
  	hif_swdb_close(self);
  	return rc;
}

static gint _package_data_insert (sqlite3 *db, struct package_data_t *package_data)
{
  	sqlite3_stmt *res;
   	const gchar *sql = INSERT_PKG_DATA;
	DB_PREP(db,sql,res);
  	DB_BIND_INT(res, "@pid", package_data->pid);
  	DB_BIND_INT(res, "@rid", package_data->rid);
  	DB_BIND(res, "@repo_r", package_data->from_repo_revision);
  	DB_BIND(res, "@repo_t", package_data->from_repo_timestamp);
  	DB_BIND(res, "@installed_by", package_data->installed_by);
  	DB_BIND(res, "@changed_by", package_data->changed_by);
  	DB_BIND(res, "@installonly", package_data->instalonly);
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
                                  	const gchar *instalonly,
                                  	const gchar *origin_url )
{
  	if (hif_swdb_open(self))
    	return 1;

  	struct package_data_t pacakge_data = { pid, _bind_repo_by_name(self->db, from_repo),
	  from_repo_revision, from_repo_timestamp, installed_by, changed_by, instalonly, origin_url};

  	gint rc = _package_data_insert(self->db, &pacakge_data);

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
  	return DB_FIND(res);
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

 	struct trans_data_beg_t trans_data_beg = {tid, _pdid_from_pid(self->db, pid),
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

gint 	hif_swdb_trans_data_end	(	HifSwdb *self,
									const gint tid)
{
  	if (hif_swdb_open(self))
    	return 1;
  	gint rc = _trans_data_end_update(self->db, tid);
  	hif_swdb_close(self);
  	return rc;
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
  	return 0;
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

  	struct output_t output = { tid, msg, hif_swdb_get_output_type(self, "stout") };
  	gint rc = _output_insert( self->db, &output);

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
  	return 0;
}

/* Bind description to id in chosen table
 * Returns: ID of desctiption (adds new element if description not present), <= 0 if error
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
        id = _insert_desc(db,table,desc);
	  	if(id) //error
		{
		  return id;
		}
	  	return _find_match_by_desc(db,table,desc);
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

    //PACKAGE_DATA
    failed += _db_exec (self->db," CREATE TABLE PACKAGE_DATA ( PD_ID integer PRIMARY KEY,"
                                    "P_ID integer, R_ID integer, from_repo_revision text,"
                                    "from_repo_timestamp text, installed_by text, changed_by text,"
                                    "installonly text, origin_url text)", NULL);
    //PACKAGE
    failed += _db_exec (self->db, "CREATE TABLE PACKAGE ( P_ID integer primary key, name text, epoch text,"
                                    "version text, release text, arch text, checksum_data text,"
                                    "checksum_type text, type integer )", NULL);
    //REPO
    failed += _db_exec (self->db, "CREATE TABLE REPO (R_ID INTEGER PRIMARY KEY, name text,"
                                    "last_synced text, is_expired text)", NULL);
    //TRANS_DATA
    failed += _db_exec (self->db, "CREATE TABLE TRANS_DATA (TD_ID INTEGER PRIMARY KEY,"
                                    "T_ID integer,PD_ID integer, G_ID integer, done INTEGER,"
                                    "ORIGINAL_TD_ID integer, reason integer, state integer)", NULL);
    //TRANS
    failed += _db_exec (self->db," CREATE TABLE TRANS (T_ID integer primary key, beg_timestamp text,"
                                    "end_timestamp text, RPMDB_version text, cmdline text,"
                                    "loginuid text, releasever text, return_code integer) ", NULL);
    //OUTPUT
    failed += _db_exec (self->db, "CREATE TABLE OUTPUT (O_ID integer primary key,T_ID INTEGER, msg text, type integer)", NULL);

    //STATE_TYPE
    failed += _db_exec (self->db, "CREATE TABLE STATE_TYPE (ID INTEGER PRIMARY KEY, description text)", NULL);

    //REASON_TYPE
    failed += _db_exec (self->db, "CREATE TABLE REASON_TYPE (ID INTEGER PRIMARY KEY, description text)", NULL);

    //OUTPUT_TYPE
    failed += _db_exec (self->db, "CREATE TABLE OUTPUT_TYPE (ID INTEGER PRIMARY KEY, description text)", NULL);

    //PACKAGE_TYPE
    failed += _db_exec (self->db, "CREATE TABLE PACKAGE_TYPE (ID INTEGER PRIMARY KEY, description text)", NULL);

    //GROUPS
    failed += _db_exec (self->db, "CREATE TABLE GROUPS (G_ID INTEGER PRIMARY KEY, name_id text, name text,"
                                    "ui_name text, is_installed integer, pkg_types integer, grp_types integer)", NULL);
    //TRANS_GROUP_DATA
    failed += _db_exec (self->db, "CREATE TABLE TRANS_GROUP_DATA (TG_ID INTEGER PRIMARY KEY, T_ID integer,"
                                    "G_ID integer, name_id text, name text, ui_name text,"
                                    "is_installed integer, pkg_types integer, grp_types integer)", NULL);
    //GROUPS_PACKAGE
    failed += _db_exec (self->db, "CREATE TABLE GROUPS_PACKAGE (GP_ID INTEGER PRIMARY KEY,"
                                    "G_ID integer, name text)", NULL);
    //GROUPS_EXCLUDE
    failed += _db_exec (self->db, "CREATE TABLE GROUPS_EXCLUDE (GE_ID INTEGER PRIMARY KEY,"
                                    "G_ID integer, name text)", NULL);
    //ENVIRONMENTS_GROUPS
    failed += _db_exec (self->db, "CREATE TABLE ENVIRONMENTS_GROUPS (EG_ID INTEGER PRIMARY KEY,"
                                    "E_ID integer, G_ID integer)", NULL);
    //ENVIRONMENTS
    failed += _db_exec (self->db, "CREATE TABLE ENVIRONMENTS (E_ID INTEGER PRIMARY KEY, name_id text,"
                                    "name text, ui_name text, pkg_types integer, grp_types integer)", NULL);
    //ENVIRONMENTS_EXCLUDE
    failed += _db_exec (self->db, "CREATE TABLE ENVIRONMENTS_EXCLUDE (EE_ID INTEGER PRIMARY KEY,"
                                    "E_ID integer, name text)", NULL);

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
