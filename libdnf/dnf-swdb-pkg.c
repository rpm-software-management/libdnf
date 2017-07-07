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

#include "dnf-swdb.h"
#include "dnf-swdb-pkg.h"
#include "dnf-swdb-pkg-sql.h"

G_DEFINE_TYPE(DnfSwdbPkg, dnf_swdb_pkg, G_TYPE_OBJECT) //history package

// SWDB package Destructor
static void
dnf_swdb_pkg_finalize(GObject *object)
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

// SWDB Package Class initialiser
static void
dnf_swdb_pkg_class_init(DnfSwdbPkgClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = dnf_swdb_pkg_finalize;
}

// SWDB Package object initialiser
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
DnfSwdbPkg*
dnf_swdb_pkg_new(const gchar* name,
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
    if (type)
    {
        swdbpkg->type = g_strdup(type);
    }
    else
    {
        swdbpkg->type = NULL;
    }
    return swdbpkg;
}

/**
* dnf_swdb_pkg_get_reason:
* @self: package object
*
* Returns: package reason string
**/
gchar*
dnf_swdb_pkg_get_reason(DnfSwdbPkg *self)
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

/**
* dnf_swdb_pkg_get_ui_from_repo:
* @self: package object
*
* Get UI reponame for package @self
* If package was installed with some other releasever, then
* print repository name in format @repo/releasever
*
* Returns: repository UI name
**/
gchar*
dnf_swdb_pkg_get_ui_from_repo(DnfSwdbPkg *self)
{
    if(self->ui_from_repo)
        return g_strdup(self->ui_from_repo);
    if(!self->swdb || !self->pid || dnf_swdb_open(self->swdb))
        return g_strdup("unknown");
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
    return g_strdup("unknown");
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
dnf_swdb_pkg_compare(DnfSwdbPkg *pkg1,
                     DnfSwdbPkg *pkg2)
{
    //compare epochs
    if (pkg1->epoch && pkg2->epoch)
    {
        gint64 epoch1 = g_ascii_strtoll(pkg1->epoch, NULL, 0);
        gint64 epoch2 = g_ascii_strtoll(pkg2->epoch, NULL, 0);
        gint64 res = epoch2 - epoch1;
        if (res)
        {
            return res;
        }
    }
    else if (pkg1->epoch && !pkg2->epoch)
    {
        return -1;
    }
    else if (!pkg1->epoch && pkg2->epoch)
    {
        return 1;
    }

    //compare versions

    //split version string into substrings
    gchar **version1 = g_strsplit(pkg1->version, ".", 0);
    gchar **version2 = g_strsplit(pkg2->version, ".", 0);

    if (!version1 || !version2)
    {
        return 0;
    }

    //compare subversions
    const gchar *subv1 = *version1;
    const gchar *subv2 = *version2;

    gint i = 0;
    gint64 res = 0;
    while (subv1 && subv2)
    {
        //convert subversions into int
        gint64 v1 = g_ascii_strtoll(subv1, NULL, 0);
        gint64 v2 = g_ascii_strtoll(subv2, NULL, 0);

        //compare subversions
        res = v2 - v1;
        if (res)
        {
            break;
        }

        //move to next subversion
        i++;
        subv1 = *(version1 + i);
        subv2 = *(version2 + i);
    }

    g_strfreev(version1);
    g_strfreev(version2);

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
dnf_swdb_pkg___lt__(DnfSwdbPkg *first,
                    DnfSwdbPkg *second)
{
    return dnf_swdb_pkg_compare(first, second) > 0;
}


/**
* dnf_swdb_pkg___gt__:
* @first: first package
* @second: second package
*
* Returns: %true when @first > @second - @first is newer
**/
gboolean
dnf_swdb_pkg___gt__(DnfSwdbPkg *first,
                    DnfSwdbPkg *second)
{
    return dnf_swdb_pkg_compare(first, second) < 0;
}
