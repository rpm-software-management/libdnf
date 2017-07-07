/* dnf-swdb-trans.c
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
#include "dnf-swdb-trans.h"
#include "dnf-swdb-trans-sql.h"
#include "dnf-swdb-sql.h"

G_DEFINE_TYPE(DnfSwdbTrans, dnf_swdb_trans, G_TYPE_OBJECT) //transaction

// SWDB Transaction Destructor
static void
dnf_swdb_trans_finalize(GObject *object)
{
    DnfSwdbTrans *trans = (DnfSwdbTrans *) object;
    g_free(trans->beg_timestamp);
    g_free(trans->end_timestamp);
    g_free(trans->beg_rpmdb_version);
    g_free(trans->end_rpmdb_version);
    g_free(trans->cmdline);
    g_free(trans->loginuid);
    g_free(trans->releasever);
    G_OBJECT_CLASS (dnf_swdb_trans_parent_class)->finalize (object);
}

// SWDB Transaction Class initialiser
static void
dnf_swdb_trans_class_init(DnfSwdbTransClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = dnf_swdb_trans_finalize;
}

// SWDB Transaction Object initialiser
static void
dnf_swdb_trans_init(DnfSwdbTrans *self)
{
    self->is_output = 0;
    self->is_error = 0;
    self->swdb = NULL;
    self->altered_lt_rpmdb = 0;
    self->altered_gt_rpmdb = 0;
    self->merged_tids = NULL;
}

/**
 * dnf_swdb_trans_new:
 *
 * Creates a new #DnfSwdbTrans.
 *
 * Returns: a #DnfSwdbTrans
**/
DnfSwdbTrans*
dnf_swdb_trans_new(gint tid,
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
gboolean
dnf_swdb_trans_compare_rpmdbv(DnfSwdbTrans *self, const gchar *rpmdbv)
{
    gboolean altered = g_strcmp0(self->end_rpmdb_version, rpmdbv);
    if (altered)
    {
        self->altered_gt_rpmdb = TRUE;
    }
    return !altered;
}

static gint
_trans_compare (gconstpointer a,
                gconstpointer b)
{
    return *((gint*) a) - *((gint*) b);
}

/**
* dnf_swdb_trans_merge:
* @self: transaction object to merge into
* @other: transaction object to merge with
*
* Merge @self with @other into @self
**/
void
dnf_swdb_trans_merge(DnfSwdbTrans *self,
                     DnfSwdbTrans *other)
{
    if (!self || !other)
    {
        return;
    }

    GArray *merged = self->merged_tids;
    if (!merged)
    {
        //create new array for merged transaction
        merged = g_array_new(0, 0, sizeof(gint));
        g_array_append_val(merged, self->tid);
    }
    else
    {
        //return if transaction is allready merged
        for (guint i = 0; i < merged->len; ++i)
        {
            if (g_array_index(merged, gint, i) == other->tid)
            {
                return;
            }
        }
    }

    //check if we are merging with merged transaction
    if (other->merged_tids)
    {
        g_array_append_vals(
            merged,
            other->merged_tids->data,
            other->merged_tids->len);
    }
    else
    {
        // add merged transaction into merged list
        g_array_append_val(merged, other->tid);
    }

    gdouble beg_time = g_ascii_strtod(self->beg_timestamp, NULL);
    gdouble beg_time_other = g_ascii_strtod(other->beg_timestamp, NULL);

    //compare starting timestamps
    if (beg_time > beg_time_other)
    {
        g_free(self->beg_timestamp);
        self->beg_timestamp = g_strdup(other->beg_timestamp);

        g_free(self->beg_rpmdb_version);
        self->beg_rpmdb_version = g_strdup(other->beg_rpmdb_version);
    }

    if (self->end_timestamp && other->end_timestamp)
    {
        gdouble end_time = g_ascii_strtod(self->end_timestamp, NULL);
        gdouble end_time_other = g_ascii_strtod(other->end_timestamp, NULL);

        //compare ending timestamps
        if (end_time < end_time_other)
        {
            g_free(self->end_timestamp);
            self->end_timestamp = g_strdup(other->end_timestamp);

            g_free(self->end_rpmdb_version);
            self->end_rpmdb_version = g_strdup(other->end_rpmdb_version);
        }
    }

    //sort transactions
    g_array_sort(merged, _trans_compare);

    //remove duplicities
    GArray *nodup = g_array_new(0, 0, sizeof(gint));

    gint last = 0;
    gint tid;
    for (guint i = 0; i < merged->len; ++i)
    {
        tid = g_array_index(merged, gint, i);
        if (last != tid)
        {
            last = tid;
            g_array_append_val(nodup, tid);
        }
    }

    g_array_unref(merged);

    self->merged_tids = nodup;
}

//struct for merging packages in transactions
typedef struct _pkg_pair_t{
    DnfSwdbPkg *first;
    DnfSwdbPkg *second;
} _pkg_pair;

//allocate new structure for transaction merging
static _pkg_pair*
_pkg_pair_new(DnfSwdbPkg *pkg1)
{
    _pkg_pair *p = g_new(_pkg_pair, 1);
    p->first = pkg1;
    p->second = NULL;
    return p;
}

//set latest state of transaction with package
static void
_pkg_pair_set(_pkg_pair *p,
              DnfSwdbPkg *pkg2)
{
    if (p->second)
    {
        g_object_unref(p->second);
    }
    p->second = pkg2;
}

/**
* _merge_altered_packages:
* @prev: transaction package pair
* @pkg: new package
*
* Merge two packages of different versions into Update/Downgrade or Reinstall
**/
static void
_merge_altered_packages(_pkg_pair *prev,
                        DnfSwdbPkg *pkg)
{
    DnfSwdbPkg *pkg1 = prev->first;
    gint64 res = dnf_swdb_pkg_compare(pkg1, pkg);
    if (res < 0) //pkg1 is newer -> downgrade
    {
        g_free(pkg1->state);
        pkg1->state = g_strdup("Downgraded");
        g_free(pkg->state);
        pkg->state = g_strdup("Downgrade");
        _pkg_pair_set(prev, pkg);
    }
    else if (res > 0) //pkg is newer -> upgrade
    {
        g_free(pkg1->state);
        pkg1->state = g_strdup("Upgraded");
        g_free(pkg->state);
        pkg->state = g_strdup("Upgrade");
        _pkg_pair_set(prev, pkg);
    }
    else //same or error -> reinstall
    {
        g_object_unref(pkg1);
        g_free(pkg->state);
        pkg->state = g_strdup("Reinstall");
        prev->first = pkg;
        _pkg_pair_set(prev, NULL);
    }
}


/**
* _merge_handle_erase:
* @prev: package pair
* @pkg: new package
*
* Helper function for dnf_swdb_trans_packages
* Handles scenario when original package was erased/obsoleted
* and new one is being installed or Obsoleting
**/
static void
_merge_handle_erase(_pkg_pair *prev,
                    DnfSwdbPkg *pkg)
{
    DnfSwdbPkg *pkg1 = prev->first;

    //new package is being installed
    if (!g_strcmp0(pkg->state, "Install") ||
        !g_strcmp0(pkg->state, "True-Install") ||
        !g_strcmp0(pkg->state, "Dep-Install"))
    {
        _merge_altered_packages(prev, pkg);
    }
    //new package is Obsoleting some other package
    else if (!g_strcmp0(pkg->state, "Obsoleting"))
    {
        //technically it should be reinstalled
        //XXX not sure
        _pkg_pair_set(prev, pkg);
    }
    // should not happen - rpmdbv problem
    else
    {
        //old transaction was compromised, keep just new one
        g_object_unref(pkg1);
        prev->first = pkg;
        _pkg_pair_set(prev, NULL);
    }
}


/**
* _merge_handle_install:
* @prev: transaction package pair
* @pkg: new package
*
* Handle install and erase/update/downgrade
*   Erase - remove both packages
*   Update/Downgrade - move install to new package
**/
static void
_merge_handle_install(GHashTable *pkgs,
                      _pkg_pair *prev,
                      DnfSwdbPkg *pkg)
{
    DnfSwdbPkg *pkg1 = prev->first;

    //handle Erase/Obsoleted
    if (!g_strcmp0(pkg->state, "Erase") ||
        !g_strcmp0(pkg->state, "Obsoleted"))
    {
        //remove both packages
        g_hash_table_remove(pkgs, pkg->name);
        g_free(prev);
        g_object_unref(pkg);
        g_object_unref(pkg1);
    }
    //handle Updated/Downgrade
    else if (!g_strcmp0(pkg->state, "Update") ||
             !g_strcmp0(pkg->state, "Downgrade"))
    {
        //transfer install to new package
        g_free(pkg->state);
        pkg->state = g_strdup(pkg1->state);
        prev->first = pkg;
        g_object_unref(pkg1);
        _pkg_pair_set(prev, NULL);
    }
    //handle reinstall etc.
    else
    {
        g_object_unref(pkg);
    }
}


/**
* _merge_handle_alter:
* @prev: transaction package pair
* @pkg: new package
*
* Handle scenerio when state of original package is:
*   Downgrade
*   Update
*   Obsoleting
*   ---
*   Used in active transaction -> to construct complete pair
*       Downgraded
*       Updated
* And new package is:
*   Reinstall -> remove new package
*   Erase/Obsoleted -> transfer state to original package
*   Downgraded/Updated ->
*       we have to make a difference between actual transaction
*       and new one. When transaction package pair is not complete,
*       then we are problably still operating original one.
*
*       With complete transaction pair we just need to get a new
*       Update/Downgrade package and compare versions with original
*       package from pair
*
*       State of updated Obsoleting packages will be transfered
*       to a new version
**/
static void
_merge_handle_alter(_pkg_pair *prev,
                    DnfSwdbPkg *pkg)
{
    DnfSwdbPkg *pkg1 = prev->first;

    //handle Erase/Obsoleted
    if (!g_strcmp0(pkg->state, "Erase") ||
        !g_strcmp0(pkg->state, "Obsoleted"))
    {
        //transfer state to original package
        pkg1->state = g_strdup(pkg->state);
        g_object_unref(pkg);
    }
    //handle Downgraded/Updated
    else if (!g_strcmp0(pkg->state, "Downgraded") ||
             !g_strcmp0(pkg->state, "Updated"))
    {
        /*
        * when transaction pair is incomplete
        * then we are probably still in the same transaction
        * we should add package into pair to achieve format
        *   (Updated, Update) or (Downgraded, Downgrade)
        */
        if (!prev->second)
        {
            if (!g_strcmp0(pkg1->state, "Downgrade") ||
                !g_strcmp0(pkg1->state, "Update"))
            {
                //pair is in wrong order
                prev->second = pkg1;
                prev->first = pkg;
            }
        }
        /*
        * handle Obsoleting package by transfering its state
        * to new package
        */
        else if (!g_strcmp0(pkg1->state, "Obsoleting"))
        {
            //transfer Obsoleting state to a new package
            g_free(pkg->state);
            pkg->state = g_strdup(pkg1->state);
            prev->first = pkg;
            g_object_unref(pkg1);
            _pkg_pair_set(prev, NULL);
        }
        /*
        * otherwise we are not interested in this package
        * it should be original package from second transaction
        * which should be same as new package from first one
        */
        else
        {
            g_object_unref(pkg);
        }
    }
    //handle Downgrade/Update
    else if (!g_strcmp0(pkg->state, "Downgrade") ||
             !g_strcmp0(pkg->state, "Update"))
    {
        /*
        * Check if new package is missing in transaction pair
        * If missing, put it in there, otherwise this package is
        * from new transaction and we need to merge them by
        * comparing original package from transaction pair and
        * this new package
        */
        if (!prev->second)
        {
            _pkg_pair_set(prev, pkg);
        }
        /*
        * Here we have a complete transaction pair and new package
        * So we just have to compare versions and merge it
        */
        else
        {
            _merge_altered_packages(prev, pkg);
        }
    }
    //handle reinstall etc.
    else
    {
        //just ignore new one
        g_object_unref(pkg);
    }
}


/**
* _dump_hash_map:
* @map: hash table with transaction pairs
*
* Dump hash table into pointer array
**/
static GPtrArray*
_dump_hash_map(GHashTable *map)
{
    GPtrArray *pkgs = g_ptr_array_new();

    GHashTableIter iter;
    gpointer key, value;

    g_hash_table_iter_init(&iter, map);

    while (g_hash_table_iter_next(&iter, &key, &value))
    {
        _pkg_pair *p = (_pkg_pair *) value;
        if (p->first)
        {
            g_ptr_array_add(pkgs, p->first);
        }
        if (p->second)
        {
            g_ptr_array_add(pkgs, p->second);
        }
    }
    return pkgs;
}


/**
* _merge_package:
* @pkgs: transaction pair hashmap
* @pkg: new package
*
* Merge new package transaction with transaction set of packages
**/
static void
_merge_package(GHashTable *pkgs,
               DnfSwdbPkg *pkg)
{
    //find associate package in map
    _pkg_pair *prev = g_hash_table_lookup(pkgs, pkg->name);

    if (!prev)
    {
        //package not in transaction set - insert and continue
        _pkg_pair *p = _pkg_pair_new(pkg);
        g_hash_table_insert(pkgs, g_strdup(pkg->name), p);
        return;
    }

    //package allready in set - needs merge

    DnfSwdbPkg *pkg1 = prev->first;

    /*
    * Erase -> Install = reinstall/downgrade/update
    * original package was erased/obsoleted
    * new package is being installed or Obsoleting
    */
    if (!g_strcmp0(pkg1->state, "Erase") ||
        !g_strcmp0(pkg1->state, "Obsoleted"))
    {
        _merge_handle_erase(prev, pkg);
    }
    /*
    * reinstall -> reinstall/erase/obsoleted/downgraded/updated
    * remove original package
    * when new state is forbidden on old one
    *   old transaction is compromised
    *   keep just new one anyway
    */
    else if (!g_strcmp0(pkg1->state, "Reinstall"))
    {
        g_object_unref(pkg1);
        prev->first = pkg;
        _pkg_pair_set(prev, NULL);
    }
    /*
    * install -> erase/update/downgrade
    * Erase - remove both packages
    * Update/Downgrade - move install to new package
    */
    else if (!g_strcmp0(pkg1->state, "Install") ||
             !g_strcmp0(pkg1->state, "True-Install") ||
             !g_strcmp0(pkg1->state, "Dep-Install"))
    {
        _merge_handle_install(pkgs, prev, pkg);
    }
    /*
    * Handle states when orignal package is
    * Downgrade, Update, Obsoleting, Downgraded, Updated
    */
    else if (!g_strcmp0(pkg1->state, "Downgrade") ||
             !g_strcmp0(pkg1->state, "Update") ||
             !g_strcmp0(pkg1->state, "Obsoleting") ||
             !g_strcmp0(pkg1->state, "Downgraded") ||
             !g_strcmp0(pkg1->state, "Updated"))
    {
        _merge_handle_alter(prev, pkg);
    }
    //unknown state
    else
    {
        g_object_unref(pkg);
    }
}


/**
* dnf_swdb_trans_packages:
* @self: transaction object
*
* Return list of packages and their actions in merged transaction
*
* Iterate transactions in chronological order
* Iterate over all packages of particular transaction
* If package is allready in transaction set then merge it
* Add (merged) package to transaction set
*
* Returns: (element-type DnfSwdbPkg)(array)(transfer container): list of #DnfSwdbPkg
**/
GPtrArray*
dnf_swdb_trans_packages(DnfSwdbTrans *self)
{
    DnfSwdb *swdb = self->swdb;

    if (!self->merged_tids)
    {
        return dnf_swdb_get_packages_by_tid(swdb, self->tid);
    }

    //empty map for packages
    GHashTable *pkgs = g_hash_table_new(g_str_hash, g_str_equal);

    //iterate trasactions
    for (guint t = 0; t < self->merged_tids->len; ++t)
    {
        //get TID of transaction
        gint tid = g_array_index(self->merged_tids, gint, t);

        //get transaction packages
        GPtrArray *tpkgs = dnf_swdb_get_packages_by_tid(swdb, tid);

        if (!tpkgs)
        {
            continue;
        }

        //iterate packages in transaction
        for (guint i = 0; i < tpkgs->len; ++i)
        {
            DnfSwdbPkg *pkg = g_ptr_array_index(tpkgs, i);

            _merge_package(pkgs, pkg);
        }
    }

    // dump hash map into an array
    GPtrArray *packages = _dump_hash_map(pkgs);

    g_hash_table_unref(pkgs);

    return packages;
}


/**
* _trans_merged_output:
* @self: transaction object
* @type: output type - stdout/stderr
*
* Get output of type @type of transaction @self
* including sub-transactions output if any for merged transaction
*
* Returns: list of outputs
**/
static GPtrArray*
_trans_merged_output(DnfSwdbTrans *self,
                     const gchar *type)
{
    DnfSwdb *swdb = self->swdb;

    if (!self->tid || dnf_swdb_open(swdb))
    {
        return NULL;
    }

    //merged transaction output
    if (self->merged_tids)
    {
        GPtrArray *data = g_ptr_array_new();

        for (guint i = 0; i < self->merged_tids->len; ++i)
        {
            gint tid = g_array_index(self->merged_tids, gint, i);

            GPtrArray *tout = _load_output(
                swdb->db,
                tid,
                dnf_swdb_get_output_type(swdb, type));

            for (guint o = 0; o < tout->len; ++o)
            {
                gchar *out = g_ptr_array_index(tout, o);
                g_ptr_array_add(data, (gpointer) out);
            }

            g_ptr_array_unref(tout);
        }
        return data;
    }

    //single transaction output
    return _load_output(swdb->db,
                        self->tid,
                        dnf_swdb_get_output_type(swdb, type));
}


/**
* dnf_swdb_trans_output:
* @self: transaction object
*
* Load standard output of transaction @self
*
* Returns: (element-type utf8) (transfer container): list of outputs
*/
GPtrArray*
dnf_swdb_trans_output(DnfSwdbTrans *self)
{
    return _trans_merged_output(self, "stdout");
}

/**
* dnf_swdb_trans_error:
* @self: transaction object
*
* Load error output of transaction @self
*
* Returns: (element-type utf8) (transfer container): list of errors
*/
GPtrArray*
dnf_swdb_trans_error(DnfSwdbTrans *self)
{
    return _trans_merged_output(self, "stderr");
}

/**
* dnf_swdb_trans_data:
* @self: transaction object
*
* Get list of transaction data objects for transaction
*
* Returns: (element-type DnfSwdbTransData)(array)(transfer container): list of #DnfSwdbTransData
*/
GPtrArray*
dnf_swdb_trans_data(DnfSwdbTrans *self)
{
    if (!self->tid || dnf_swdb_open(self->swdb))
    {
        return NULL;
    }

    sqlite3 *db = self->swdb->db;

    GPtrArray *node = g_ptr_array_new();
    sqlite3_stmt *res;
    const gchar *sql = S_TRANS_DATA_BY_TID;

    DB_PREP(db, sql, res);
    DB_BIND_INT(res, "@tid", self->tid);

    while(sqlite3_step(res) == SQLITE_ROW)
    {
        gchar *tmp_reason = _get_description(db,
                                             sqlite3_column_int(res, 6),
                                             S_REASON_TYPE_BY_ID);

        gchar *tmp_state = _get_description(db,
                                            sqlite3_column_int(res, 7),
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
* dnf_swdb_trans_performed_with:
* @self: transaction object
*
* Get transaction performed with packages for transaction @self
*
* Returns: (element-type DnfSwdbPkg)(array)(transfer container): list of #DnfSwdbPkg
**/
GPtrArray*
dnf_swdb_trans_performed_with(DnfSwdbTrans *self)
{
    DnfSwdb *swdb = self->swdb;

    if (!self->tid || dnf_swdb_open(swdb))
    {
        return NULL;
    }

    //fetch pids
    sqlite3_stmt *res;
    const gchar *sql = S_TRANS_WITH;
    DB_PREP(swdb->db, sql, res);
    DB_BIND_INT(res, "@tid", self->tid);

    gint pid;
    GArray *pids = g_array_new(0, 0, sizeof(gint));
    while ((pid = DB_FIND_MULTI(res)))
    {
        g_array_append_val(pids, pid);
    }

    //get packages for these pids
    GPtrArray *pkgs = g_ptr_array_new();
    for (guint i = 0; i < pids->len; ++i)
    {
        pid = g_array_index(pids, gint, i);
        DnfSwdbPkg *pkg = _get_package_by_pid(swdb->db, pid);
        if (pkg)
        {
            g_ptr_array_add(pkgs, (gpointer) pkg);
        }
    }
    g_array_unref(pids);
    return pkgs;
}


/**
* dnf_swdb_trans_tids:
* @self: transaction object
*
* Return array of transaction ids merged into transaction
* Or just transaction id when @self is not merged transaction
*
* Returns: (element-type gint32) (transfer container): list of transaction ids
**/
GArray*
dnf_swdb_trans_tids(DnfSwdbTrans *self)
{
    GArray *tids = g_array_new(0, 0, sizeof(gint));

    //merged
    if (self->merged_tids)
    {
        GArray *source = self->merged_tids;
        g_array_append_vals(tids, source->data, source->len);
        return tids;
    }

    //not merged
    g_array_append_val(tids, self->tid);
    return tids;
}


/**
* dnf_swdb_trans_compare:
* @first: first transaction
* @second: second transaction
*
* Returns: @second - @first
**/
gint64
dnf_swdb_trans_compare(DnfSwdbTrans *first,
                       DnfSwdbTrans *second)
{
    if (!first || !second)
    {
        return 0;
    }
    gdouble beg_first = g_ascii_strtod(first->beg_timestamp, NULL);
    gdouble beg_second = g_ascii_strtod(second->beg_timestamp, NULL);

    if (beg_first == beg_second)
    {
        gdouble end_first = g_ascii_strtod(first->end_timestamp, NULL);
        gdouble end_second = g_ascii_strtod(second->end_timestamp, NULL);
        if (end_first == end_second)
        {
            return second->tid - first->tid;
        }
        return end_second - end_first;
    }

    return beg_second - beg_first;
}

/**
* dnf_swdb_trans___lt__:
* @first: first transaction
* @second: second transaction
*
* Returns: %true when @first is less then @second
**/
gboolean
dnf_swdb_trans___lt__(DnfSwdbTrans *first,
                      DnfSwdbTrans *second)
{
    return dnf_swdb_trans_compare(first, second) < 0;
}

/**
* dnf_swdb_trans___gt__:
* @first: first transaction
* @second: second transaction
*
* Returns: %true when @first is greater then @second
**/
gboolean
dnf_swdb_trans___gt__(DnfSwdbTrans *first,
                      DnfSwdbTrans *second)
{
    return dnf_swdb_trans_compare(first, second) > 0;
}
