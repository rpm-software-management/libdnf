/* dnf-swdb-test.c
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

/** TODO Checklist
* dnf_swdb_search
* dnf_swdb_checksums_by_nvras
* dnf_swdb_select_user_installed #
* dnf_swdb_user_installed #
* ----- GROUPS -----
* dnf_swdb_group_add_package
* dnf_swdb_group_add_exclude
* dnf_swdb_env_add_exclude
* dnf_swdb_env_add_group
* dnf_swdb_add_group
* dnf_swdb_add_env
* dnf_swdb_get_group
* dnf_swdb_get_env
* dnf_swdb_groups_by_pattern
* dnf_swdb_env_by_pattern
* dnf_swdb_group_get_exclude
* dnf_swdb_group_get_full_list
* dnf_swdb_group_update_full_list
* dnf_swdb_uninstall_group
* dnf_swdb_env_get_grp_list
* dnf_swdb_env_is_installed
* dnf_swdb_env_get_exclude
* dnf_swdb_groups_commit
* dnf_swdb_log_group_trans
* ------ REPOS -----
* dnf_swdb_repo_by_nvra #
* dnf_swdb_set_repo #
* ----- PACKAGE -----
* dnf_swdb_log_package_data #
* dnf_swdb_get_packages_by_tid #
* dnf_swdb_pkg_get_ui_from_repo #
* dnf_swdb_package_by_nvra #
* dnf_swdb_package_data_by_nvra #
* dnf_swdb_set_reason #
* dnf_swdb_mark_user_installed #
* dnf_swdb_add_package #
* dnf_swdb_reason_by_nvra #
* ------ RPM ------
* dnf_swdb_add_rpm_data #
* ------ TRANS -----
* dnf_swdb_trans_data_beg #
* dnf_swdb_trans_data_end #
* dnf_swdb_trans_data_pid_end #
* dnf_swdb_trans_beg #
* dnf_swdb_trans_end #
* dnf_swdb_trans_cmdline
* dnf_swdb_get_old_trans_data #
* dnf_swdb_trans_old
* dnf_swdb_last #
* dnf_swdb_log_error
* dnf_swdb_log_output #
* dnf_swdb_load_error
* dnf_swdb_load_output #
* ------ UTIL -----
* dnf_swdb_get_path #
* dnf_swdb_set_path #
* dnf_swdb_exist #
* dnf_swdb_create_db #
* dnf_swdb_reset_db #
* ------ intern ------
* dnf_swdb_open #
* dnf_swdb_close #
* dnf_swdb_get_package_type #
* dnf_swdb_get_output_type #
* dnf_swdb_get_reason_type #
* dnf_swdb_get_state_type #
* # = covered
*/

#include <glib.h>
#include <glib-object.h>
#include "libdnf/dnf-swdb.h"
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define INIT_PACAKGES 32
#define TRANS_OUT "initial transaction 001"

char *random_package_nvra = NULL;

/**
 * Generate some random string with given length
 */
static gchar * generate_str(guint len)
{
    gchar *str = malloc (sizeof(gchar)*(len+1));
    for (guint i = 0; i< len; ++i)
    {
        str[i] = rand() % 26 + 97;
    }
    str[len] = 0;
    return str;
}

/**
 * Generate ordinary package with some random attributes
 */
static DnfSwdbPkg * generate_package ()
{
    gchar* name = generate_str(8);
    const gchar* epoch = (rand() % 2) ? "0":"1";
    gchar* version = generate_str(4);
    gchar* release = generate_str(6);
    gchar* arch = generate_str(4);
    gchar* checksum_data = generate_str(32);
    const gchar* checksum_type = (rand() % 10) ? "sha256" : "sha1";
    const gchar *type = (rand() % 10) ? "rpm" : "source";
    DnfSwdbPkg *pkg = dnf_swdb_pkg_new(
        name,
        epoch,
        version,
        release,
        arch,
        checksum_data,
        checksum_type,
        type
    );
    g_free(name);
    g_free(version);
    g_free(release);
    g_free(arch);
    g_free(checksum_data);
    g_assert(pkg);
    return pkg;
}

/**
 * Generate rpm data with some random attributes
 */
static DnfSwdbRpmData * generate_rpm_data (gint pid)
{
    gchar* buildtime = generate_str(10);
    gchar* buildhost = generate_str(6);
    gchar* license = generate_str(4);
    gchar* packager = generate_str(6);
    gchar* size = generate_str(2);
    gchar* sourcerpm = generate_str(16);
    gchar* url = generate_str(16);
    gchar* vendor = generate_str(6);
    gchar* committer = generate_str(6);
    gchar* committime = generate_str(10);
    DnfSwdbRpmData *data = dnf_swdb_rpmdata_new(pid,
                                                buildtime,
                                                buildhost,
                                                license,
                                                packager,
                                                size,
                                                sourcerpm,
                                                url,
                                                vendor,
                                                committer,
                                                committime);
    g_free(buildtime);
    g_free(buildhost);
    g_free(license);
    g_free(packager);
    g_free(size);
    g_free(sourcerpm);
    g_free(url);
    g_free(vendor);
    g_free(committer);
    g_free(committime);
    g_assert(data);
    return data;
}

/**
 * Generate some package data with random attributes
 */
static DnfSwdbPkgData * generate_package_data ()
{
	const gchar *from_repo = "testaconda";
    gchar *from_repo_revision = generate_str(16);
  	gchar *from_repo_timestamp = malloc (11*sizeof(gchar));
    snprintf(from_repo_timestamp, 11, "%d", (gint)time(NULL));
  	const gchar *installed_by = "1000";
  	const gchar *changed_by = (rand()% 25) ? NULL : "4242" ;
  	const gchar *installonly = (rand()% 100) ? NULL : "True";
  	const gchar *origin_url =
        (rand()% 10) ? NULL : "https://github.com/edynox/libhif";

    DnfSwdbPkgData *pkg_data = dnf_swdb_pkgdata_new(from_repo_revision,
                                                    from_repo_timestamp,
                                                    installed_by,
                                                    changed_by,
                                                    installonly,
                                                    origin_url,
                                                    from_repo);
    g_free(from_repo_revision);
    g_free(from_repo_timestamp);
    return pkg_data;
}

static gint begin_trans( DnfSwdb *self)
{
    gchar *time_str = malloc (11*sizeof(gchar));
    snprintf(time_str, 11, "%d", (gint)time(NULL));

    gchar *rpmdb_version = generate_str(32);
    gchar *cmdline = generate_str(16);

    //open initial transaction
    guint tid =  dnf_swdb_trans_beg(    self,
                                        time_str,
                                        rpmdb_version,
    					                cmdline,
    					                 "1000",
    					                "42");
    g_free(time_str);
    g_free(rpmdb_version);
    g_free(cmdline);
    return tid;
}

static gint end_trans ( DnfSwdb *self,
                        gint tid)
{
    gint rc;
    gchar *time_str = malloc (11*sizeof(gchar));
    gchar *rpmdb_version = generate_str(32);

    snprintf(time_str, 11, "%d", (gint)time(NULL));

    //close transaction as success
    rc =  dnf_swdb_trans_end(   self,
                                tid,
                                time_str,
                                rpmdb_version,
    					        0);
    g_free(time_str);
    g_free(rpmdb_version);
    return rc;
}

static void check_package_persistor(DnfSwdb *self, DnfSwdbPkg *pkg)
{
    gchar *repo = dnf_swdb_repo_by_nvra(self, pkg->nvra);
    g_assert(!g_strcmp0(pkg->ui_from_repo, repo));
    g_free(repo);

    const gchar* new_repo = (rand() % 2) ? "testdora" : "testfusion";
    g_assert(!dnf_swdb_set_repo(self, pkg->nvra, new_repo));
    repo = dnf_swdb_repo_by_nvra(self, pkg->nvra);

    //*Sigh...* here we have problem of non object - nvra approach
    //repo changed in database but not in our package object...
    //We would like to do:
    //g_assert(!g_strcmp0(new_repo, pkg->from_repo));
    //but we only can:
    g_assert(!g_strcmp0(new_repo, repo));
    g_free(repo);

    gint pid = dnf_swdb_pid_by_nvra(self, pkg->nvra);
    g_assert(pid && pid == pkg->pid);

    //test another method how to obtain same package
    DnfSwdbPkg *same_pkg = dnf_swdb_package_by_nvra(self, pkg->nvra);
    g_assert(same_pkg);
    g_assert(same_pkg->pid == pid);
    g_assert(!g_strcmp0(pkg->checksum_data, same_pkg->checksum_data));

    //obtained package should contain new repo settings
    g_assert(!g_strcmp0(dnf_swdb_pkg_get_ui_from_repo(same_pkg), new_repo));

    g_object_unref(same_pkg);

    //playing with reason
    g_assert(dnf_swdb_user_installed(self, pkg->nvra));

    g_assert(!dnf_swdb_mark_user_installed(self, pkg->nvra, FALSE));

    g_assert(!dnf_swdb_user_installed(self, pkg->nvra));

    g_assert(!dnf_swdb_set_reason(self, pkg->nvra, "user"));

    g_assert(dnf_swdb_user_installed(self, pkg->nvra));

}

static void check_initial_transaction(DnfSwdb *self)
{
    DnfSwdbTrans *trans = dnf_swdb_last(self);
    g_assert(trans->tid == 1);
    g_assert(trans->return_code == 0);
    g_assert(!trans->altered_lt_rpmdb);
    g_assert(!trans->altered_gt_rpmdb);

    //get me all packages from that transaction and verify them
    GPtrArray *packages = dnf_swdb_get_packages_by_tid(self, trans->tid);
    GPtrArray *trans_data = dnf_swdb_get_old_trans_data(self, trans);
    g_assert(trans_data && trans_data->len == INIT_PACAKGES);
    g_assert(packages && packages->len == INIT_PACAKGES);

    DnfSwdbPkg *rpkg = g_ptr_array_index(packages, rand() % packages->len);
    random_package_nvra = g_strdup(rpkg->nvra);

    GPtrArray *nvras = g_ptr_array_new();
    //get packages and check if they are user installed
    for(guint i = 0; i < packages->len; ++i)
    {
        DnfSwdbPkg *pkg = g_ptr_array_index(packages, i);
        g_ptr_array_add(nvras, pkg->nvra);
    }
    GArray *user_installed = dnf_swdb_select_user_installed(self, nvras);
    g_assert(user_installed);
    g_assert(user_installed->len == packages->len);
    g_assert(g_array_index(user_installed,
                           gint,
                           packages->len - 1) == ((int)packages->len -1));

    g_array_free(user_installed, TRUE);
    g_ptr_array_free(nvras, TRUE);


    for (guint i = 0; i < packages->len; ++i)
    {
        //check object attributes
        DnfSwdbPkg *pkg = g_ptr_array_index(packages, i);
        g_assert(pkg->name && *pkg->name);
        g_assert(pkg->epoch && *pkg->epoch);
        g_assert(pkg->version && *pkg->version);
        g_assert(pkg->release && *pkg->release);
        g_assert(pkg->arch && *pkg->arch);
        g_assert(pkg->checksum_data && *pkg->checksum_data);
        g_assert(pkg->checksum_type && *pkg->checksum_type);
        g_assert(pkg->type && *pkg->type);
        g_assert(pkg->done);
        g_assert(pkg->state && *pkg->state);
        g_assert(pkg->pid);
        g_assert(pkg->nvra && *pkg->nvra);
        g_assert(dnf_swdb_pkg_get_ui_from_repo(pkg));

        //check package trans and package data
        DnfSwdbTransData *transdata = g_ptr_array_index(trans_data, i);
        DnfSwdbPkgData *pkgdata = dnf_swdb_package_data_by_nvra(self,
                                                                pkg->nvra);
        g_assert(pkgdata);
        g_assert(pkgdata->from_repo && *pkgdata->from_repo);
        g_assert(pkgdata->from_repo_revision && *pkgdata->from_repo_revision);
        g_assert(pkgdata->from_repo_timestamp && *pkgdata->from_repo_timestamp);
        g_assert(pkgdata->installed_by && *pkgdata->installed_by);
        g_assert(!g_strcmp0(pkgdata->from_repo, pkg->ui_from_repo));

        g_assert(transdata->done);

        g_assert(transdata->reason);
        g_assert(!g_strcmp0(transdata->reason, "user"));

        g_assert(transdata->state);
        g_assert(!g_strcmp0(transdata->state, "Install"));

        //check bindings
        g_assert(pkgdata->pdid);
        g_assert(transdata->pdid);
        g_assert(pkgdata->pdid == transdata->pdid);

        g_assert(pkgdata->pid);
        g_assert(pkgdata->pid == pkg->pid);

        g_assert(transdata->tid);
        g_assert(transdata->tid == trans->tid);

        //check methods operating with package
        check_package_persistor(self, pkg);

        g_object_unref(pkg);
        g_object_unref(pkgdata);
        g_object_unref(transdata);
    }
    g_ptr_array_free(packages, TRUE);
    g_ptr_array_free(trans_data, TRUE);

    //verify trans output
    GPtrArray *output = dnf_swdb_load_output(self, trans->tid);
    g_assert(!g_strcmp0(g_ptr_array_index(output,0), TRANS_OUT));
    g_free(g_ptr_array_index(output, 0));
    g_ptr_array_free(output, TRUE);

    g_object_unref(trans);
}

static void run_initial_transaction(DnfSwdb *self)
{
    guint tid = begin_trans(self);
    g_assert(tid); //tid not 0

    //now add some packages
    for (guint i = 0; i < INIT_PACAKGES; ++i)
    {
        DnfSwdbPkg *pkg = generate_package();

        //add pkg to database
        g_assert(dnf_swdb_add_package(self, pkg));

        DnfSwdbRpmData *rpm_data = generate_rpm_data(pkg->pid);

        //add rpm data to package
        g_assert(!dnf_swdb_add_rpm_data(self, rpm_data));

        DnfSwdbPkgData *pkg_data = generate_package_data();

        //add package data
        g_assert(!dnf_swdb_log_package_data(self, pkg->pid, pkg_data));

        //dont forget trans data
        g_assert(!dnf_swdb_trans_data_beg(  self,
                                            tid,
                                            pkg->pid,
                                            "user", //reason
                                            "Install")); //state

        //package is being installed...

        //package installed successfully
        g_assert(!dnf_swdb_trans_data_pid_end(  self,
                                                pkg->pid,
                                                tid,
                                                "Install"));

        g_object_unref(pkg);
        g_object_unref(rpm_data);
        g_object_unref(pkg_data);
    }

    //add some output
    g_assert(!dnf_swdb_log_output(self, tid, TRANS_OUT));


    //close transaction
    g_assert(!end_trans(self, tid));

    //all done, mark all packages as done
    //XXX we dont need to do that here, not sure about DNF
    //g_assert(!dnf_swdb_trans_data_end( self, tid));
}

static gint update_package(DnfSwdb *self, DnfSwdbPkg *pkg)
{
    gint tid = begin_trans(self);
    g_assert(tid); //tid not 0
    g_assert(dnf_swdb_add_package(self, pkg)); //pkg added
    DnfSwdbRpmData *rpm_data = generate_rpm_data(pkg->pid); //add rpm_data
    g_assert(!dnf_swdb_add_rpm_data(self, rpm_data));
    DnfSwdbPkgData *pkg_data = generate_package_data();
    g_assert(!dnf_swdb_log_package_data(self, pkg->pid, pkg_data));
    g_assert(!dnf_swdb_trans_data_beg(  self,
                                        tid,
                                        pkg->pid,
                                        "user", //reason
                                        "Update")); //state
    g_assert(!dnf_swdb_trans_data_pid_end(  self,
                                            pkg->pid,
                                            tid,
                                            "Update"));
    g_assert(!end_trans(self, tid));
    DnfSwdbTrans *last_trans = dnf_swdb_last(self);
    g_assert(last_trans);
    GPtrArray *trans_data_array = dnf_swdb_get_old_trans_data(self, last_trans);
    g_assert(trans_data_array || trans_data_array->len);
    DnfSwdbTransData *trans_data = g_ptr_array_index(trans_data_array, 0);
    g_assert(trans_data);
    g_assert(trans_data->ORIGINAL_TD_ID);
    gint tdid = trans_data->tdid;
    g_assert(tdid); //to check up reinstall
    g_ptr_array_free(trans_data_array, TRUE);
    g_object_unref(trans_data);
    g_object_unref(rpm_data);
    g_object_unref(pkg_data);
    g_object_unref(last_trans);
    return tdid;
}

static gint reinstall_package(DnfSwdb *self, DnfSwdbPkg *pkg, gint last_tdid)
{
    gint tid = begin_trans(self);
    g_assert(tid); //tid not 0
    DnfSwdbPkgData *pkg_data = generate_package_data();
    g_assert(!dnf_swdb_log_package_data(self, pkg->pid, pkg_data));
    g_assert(!dnf_swdb_trans_data_beg(  self,
                                        tid,
                                        pkg->pid,
                                        "user", //reason
                                        "Reinstall")); //state
    g_assert(!dnf_swdb_trans_data_pid_end(  self,
                                            pkg->pid,
                                            tid,
                                            "Reinstall"));
    g_assert(!end_trans(self, tid));
    DnfSwdbTrans *last_trans = dnf_swdb_last(self);
    g_assert(last_trans);
    GPtrArray *trans_data_array = dnf_swdb_get_old_trans_data(self, last_trans);
    g_assert(trans_data_array || trans_data_array->len);
    DnfSwdbTransData *trans_data = g_ptr_array_index(trans_data_array, 0);
    g_assert(trans_data);
    g_assert(trans_data->ORIGINAL_TD_ID == last_tdid);
    gint tdid = trans_data->tdid;
    g_assert(tdid);
    g_ptr_array_free(trans_data_array, TRUE);
    g_object_unref(last_trans);
    g_object_unref(trans_data);
    g_object_unref(pkg_data);
    return tdid;
}

static void erase_package(DnfSwdb *self, DnfSwdbPkg *pkg, gint last_tdid)
{
    gint tid = begin_trans(self);
    g_assert(tid); //tid not 0
    DnfSwdbPkgData *pkg_data = generate_package_data();
    g_assert(!dnf_swdb_log_package_data(self, pkg->pid, pkg_data));
    g_assert(!dnf_swdb_trans_data_beg(  self,
                                        tid,
                                        pkg->pid,
                                        "user", //reason
                                        "Erase")); //state
    g_assert(!dnf_swdb_trans_data_pid_end(  self,
                                            pkg->pid,
                                            tid,
                                            "Erase"));
    g_assert(!end_trans(self, tid));
    DnfSwdbTrans *last_trans = dnf_swdb_last(self);
    g_assert(last_trans);
    GPtrArray *trans_data_array = dnf_swdb_get_old_trans_data(self, last_trans);
    g_assert(trans_data_array || trans_data_array->len);
    DnfSwdbTransData *trans_data = g_ptr_array_index(trans_data_array, 0);
    g_assert(trans_data);
    g_assert(trans_data->ORIGINAL_TD_ID == last_tdid);
    gint tdid = trans_data->tdid;
    g_assert(tdid);
    g_ptr_array_free(trans_data_array, TRUE);
    g_object_unref(trans_data);
    g_object_unref(pkg_data);
    g_object_unref(last_trans);
}

gint main ()
{
    const gchar *_path = "./tmp_swdb.sqlite";
    //create DB object
    DnfSwdb *self = dnf_swdb_new(_path, "42");

    g_assert(!g_strcmp0(dnf_swdb_get_path(self), _path));

    //reset database
    dnf_swdb_reset_db(self);

    //check if db exists
    g_assert(dnf_swdb_exist(self));

    srand(time(NULL));

    //lets do some intial transaction
    run_initial_transaction(self);

    //take initial transaction and verify it
    check_initial_transaction(self);

    //search some random package
    DnfSwdbPkg *pkg = dnf_swdb_package_by_nvra(self, random_package_nvra);
    g_assert(!g_strcmp0(pkg->nvra, random_package_nvra));
    GPtrArray *patts = g_ptr_array_new();
    g_ptr_array_add(patts, (gpointer) pkg->name);
    GArray *tids = dnf_swdb_search(self, patts);
    g_ptr_array_free(patts, TRUE);
    g_assert(tids);
    g_assert(tids->len == 1);
    g_assert(g_array_index(tids, gint, 0) == 1); // initial trans
    g_array_free(tids, TRUE);

    //update
    //change package version and checksum
    g_free((gchar*) pkg->version);
    pkg->version = generate_str(4);
    g_free((gchar*) pkg->checksum_data);
    pkg->checksum_data = generate_str(32);
    gint tdid = update_package(self, pkg);
    tdid = reinstall_package(self, pkg, tdid);
    erase_package(self, pkg, tdid);

    //TODO create some group transaction




    g_object_unref(pkg);
    g_object_unref(self);
    return 0;
}
