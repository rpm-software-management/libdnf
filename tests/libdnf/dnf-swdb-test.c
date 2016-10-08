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

/** TODO
* dnf_swdb_search - TODO problem in extended
* dnf_swdb_checksums_by_nvras
* dnf_swdb_select_user_installed
* dnf_swdb_user_installed
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
* dnf_swdb_repo_by_pattern
* dnf_swdb_set_repo
* ----- PACKAGE -----
* dnf_swdb_add_package_nevracht
* dnf_swdb_get_pid_by_nevracht
* dnf_swdb_log_package_data
* dnf_swdb_get_pkg_attr
* dnf_swdb_attr_by_pattern
* dnf_swdb_get_packages_by_tid
* dnf_swdb_pkg_get_ui_from_repo
* dnf_swdb_package_by_pattern
* dnf_swdb_package_data_by_pattern
* dnf_swdb_set_reason
* dnf_swdb_mark_user_installed
* ------ RPM ------
* dnf_swdb_log_rpm_data
* ------ TRANS -----
* dnf_swdb_trans_data_beg
* dnf_swdb_trans_data_end
* dnf_swdb_trans_data_pid_end
* dnf_swdb_trans_beg
* dnf_swdb_trans_end
* dnf_swdb_trans_cmdline
* dnf_swdb_get_old_trans_data
* dnf_swdb_trans_old
* dnf_swdb_last * repair this in dnf - complete only called automaticaly
* dnf_swdb_log_error
* dnf_swdb_log_output
* dnf_swdb_load_error
* dnf_swdb_load_output
* ------ UTIL -----
* dnf_swdb_get_path
* dnf_swdb_set_path
* dnf_swdb_exist
* dnf_swdb_create_db
* dnf_swdb_reset_db
* ------ intern ------
* dnf_swdb_open
* dnf_swdb_close
* dnf_swdb_get_package_type
* dnf_swdb_get_output_type
* dnf_swdb_get_reason_type
* dnf_swdb_get_state_type
*/

#include <glib.h>
#include <glib-object.h>
#include "libdnf/dnf-swdb.h"

int main ()
{
    DnfSwdb *self = dnf_swdb_new( "/tmp/swdb.sqlite", "42"  );


    return 0;
}
