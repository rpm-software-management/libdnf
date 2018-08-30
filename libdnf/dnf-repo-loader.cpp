/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2013-2015 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or(at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

/**
 * SECTION:dnf-repo-loader
 * @short_description: An object for loading several #DnfRepo objects.
 * @include: libdnf.h
 * @stability: Stable
 *
 * This object can create #DnfRepo objects from either DVD media or from .repo
 * files in the repodir.
 *
 * See also: #DnfRepo
 */

#include <strings.h>

#include <gio/gunixmounts.h>
#include <librepo/util.h>
#include <string.h>

#include "dnf-package.h"
#include "dnf-repo-loader.h"
#include "dnf-utils.h"

typedef struct
{
    GFileMonitor    *monitor_repos;
    DnfContext      *context;    /* weak reference */
    GPtrArray       *repos;
    GVolumeMonitor  *volume_monitor;
    gboolean         loaded;
} DnfRepoLoaderPrivate;

enum {
    SIGNAL_CHANGED,
    SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE(DnfRepoLoader, dnf_repo_loader, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (static_cast<DnfRepoLoaderPrivate *>(dnf_repo_loader_get_instance_private (o)))

/**
 * dnf_repo_loader_finalize:
 **/
static void
dnf_repo_loader_finalize(GObject *object)
{
    DnfRepoLoader *self = DNF_REPO_LOADER(object);
    DnfRepoLoaderPrivate *priv = GET_PRIVATE(self);

    if (priv->context != NULL)
        g_object_remove_weak_pointer(G_OBJECT(priv->context),
                                     (void **) &priv->context);
    if (priv->monitor_repos != NULL)
        g_object_unref(priv->monitor_repos);
    g_object_unref(priv->volume_monitor);
    g_ptr_array_unref(priv->repos);

    G_OBJECT_CLASS(dnf_repo_loader_parent_class)->finalize(object);
}

/**
 * dnf_repo_loader_invalidate:
 */
static void
dnf_repo_loader_invalidate(DnfRepoLoader *self)
{
    DnfRepoLoaderPrivate *priv = GET_PRIVATE(self);
    priv->loaded = FALSE;
    dnf_context_invalidate_full(priv->context, "repos.d invalidated",
                     DNF_CONTEXT_INVALIDATE_FLAG_ENROLLMENT);
}

/**
 * dnf_repo_loader_mount_changed_cb:
 */
static void
dnf_repo_loader_mount_changed_cb(GVolumeMonitor *vm, GMount *mount, DnfRepoLoader *self)
{
    /* invalidate all repos if a CD is inserted / removed */
    g_debug("emit changed(mounts changed)");
    g_signal_emit(self, signals[SIGNAL_CHANGED], 0);
    dnf_repo_loader_invalidate(self);
}

/**
 * dnf_repo_loader_init:
 **/
static void
dnf_repo_loader_init(DnfRepoLoader *self)
{
    DnfRepoLoaderPrivate *priv = GET_PRIVATE(self);
    priv->repos = g_ptr_array_new_with_free_func((GDestroyNotify) g_object_unref);
    priv->volume_monitor = g_volume_monitor_get();
    g_signal_connect(priv->volume_monitor, "mount-added",
                     G_CALLBACK(dnf_repo_loader_mount_changed_cb), self);
    g_signal_connect(priv->volume_monitor, "mount-removed",
                     G_CALLBACK(dnf_repo_loader_mount_changed_cb), self);
}

/**
 * dnf_repo_loader_class_init:
 **/
static void
dnf_repo_loader_class_init(DnfRepoLoaderClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /**
     * DnfRepoLoader::changed:
     **/
    signals[SIGNAL_CHANGED] =
        g_signal_new("changed",
                  G_TYPE_FROM_CLASS(object_class), G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET(DnfRepoLoaderClass, changed),
                  NULL, NULL, g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

    object_class->finalize = dnf_repo_loader_finalize;
}

/**
 * dnf_repo_loader_add_media:
 **/
static gboolean
dnf_repo_loader_add_media(DnfRepoLoader *self,
             const gchar *mount_point,
             guint idx,
             GError **error)
{
    DnfRepoLoaderPrivate *priv = GET_PRIVATE(self);
    DnfRepo *repo;
    g_autofree gchar *treeinfo_fn = NULL;
    g_autoptr(GKeyFile) treeinfo = NULL;

    /* get common things */
    treeinfo_fn = g_build_filename(mount_point, ".treeinfo", NULL);
    treeinfo = g_key_file_new();
    if (!g_key_file_load_from_file(treeinfo, treeinfo_fn, G_KEY_FILE_NONE, error))
        return FALSE;

    /* create read-only location */
    repo = dnf_repo_new(priv->context);
    dnf_repo_set_enabled(repo, DNF_REPO_ENABLED_PACKAGES);
    dnf_repo_set_gpgcheck(repo, TRUE);
    dnf_repo_set_kind(repo, DNF_REPO_KIND_MEDIA);
    dnf_repo_set_cost(repo, 100);
    dnf_repo_set_keyfile(repo, treeinfo);
    if (idx == 0) {
        dnf_repo_set_id(repo, "media");
    } else {
        g_autofree gchar *tmp = NULL;
        tmp = g_strdup_printf("media-%i", idx);
        dnf_repo_set_id(repo, tmp);
    }
    dnf_repo_set_location(repo, mount_point);
    if (!dnf_repo_setup(repo, error))
        return FALSE;

    g_debug("added repo %s", dnf_repo_get_id(repo));
    g_ptr_array_add(priv->repos, repo);
    return TRUE;
}

/**
 * dnf_repo_loader_add_sack_from_mount_point:
 */
static gboolean
dnf_repo_loader_add_sack_from_mount_point(DnfRepoLoader *self,
                                          const gchar *root,
                                          guint *idx,
                                          GError **error)
{
    const gchar *id = ".treeinfo";
    gboolean exists;
    g_autofree gchar *treeinfo_fn = NULL;

    /* check if any installed media is an install disk */
    treeinfo_fn = g_build_filename(root, id, NULL);
    exists = g_file_test(treeinfo_fn, G_FILE_TEST_EXISTS);
    g_debug("checking %s for %s: %s", root, id, exists ? "yes" : "no");
    if (!exists)
        return TRUE;

    /* add the repodata/repomd.xml as a repo */
    if (!dnf_repo_loader_add_media(self, root, *idx, error))
        return FALSE;
   (*idx)++;
    return TRUE;
}

/**
 * dnf_repo_loader_get_repos_removable:
 */
static gboolean
dnf_repo_loader_get_repos_removable(DnfRepoLoader *self, GError **error)
{
    GList *mounts;
    GList *l;
    gboolean ret = TRUE;
    guint idx = 0;

    /* coldplug the mounts */
    mounts = g_unix_mounts_get(NULL);
    for (l = mounts; l != NULL; l = l->next) {
        GUnixMountEntry *e =(GUnixMountEntry *) l->data;
        if (!g_unix_mount_is_readonly(e))
            continue;
        if (g_strcmp0(g_unix_mount_get_fs_type(e), "iso9660") != 0)
            continue;
        ret = dnf_repo_loader_add_sack_from_mount_point(self,
                                                  g_unix_mount_get_mount_path(e),
                                                  &idx,
                                                  error);
        if (!ret)
            goto out;
    }
out:
    g_list_foreach(mounts,(GFunc) g_unix_mount_free, NULL);
    g_list_free(mounts);
    return ret;
}

/**
 * dnf_repo_loader_repo_cost_fn:
 */
static gint
dnf_repo_loader_repo_cost_fn(gconstpointer a, gconstpointer b)
{
    DnfRepo *repo_a = *((DnfRepo **) a);
    DnfRepo *repo_b = *((DnfRepo **) b);
    if (dnf_repo_get_cost(repo_a) < dnf_repo_get_cost(repo_b))
        return -1;
    if (dnf_repo_get_cost(repo_a) > dnf_repo_get_cost(repo_b))
        return 1;
    return 0;
}

/**
 * dnf_repo_loader_load_multiline_key_file:
 **/
static GKeyFile *
dnf_repo_loader_load_multiline_key_file(const gchar *filename, GError **error)
{
    GKeyFile *file = NULL;
    gboolean ret;
    gsize len;
    guint i;
    g_autofree gchar *data = NULL;
    g_autoptr(GString) string = NULL;
    g_auto(GStrv) lines = NULL;

    /* load file */
    if (!g_file_get_contents(filename, &data, &len, error))
        return NULL;

    /* split into lines */
    string = g_string_new("");
    lines = g_strsplit(data, "\n", -1);
    for (i = 0; lines[i] != NULL; i++) {

        /* convert tabs to spaces */
        g_strdelimit(lines[i], "\t", ' ');

        /* if a line starts with whitespace, then append it on
         * the previous line */
        if (lines[i][0] == ' ' && string->len > 0) {

            /* whitespace strip this new line */
            g_strstrip(lines[i]);

            /* skip over the line if it was only whitespace */
            if (strlen(lines[i]) == 0)
                continue;

            /* remove old newline from previous line */
            g_string_set_size(string, string->len - 1);

            /* only add a ';' if we have anything after the '=' */
            if (string->str[string->len - 1] == '=') {
                g_string_append_printf(string, "%s\n", lines[i]);
            } else {
                g_string_append_printf(string, ";%s\n", lines[i]);
            }
        } else {
            g_string_append_printf(string, "%s\n", lines[i]);
        }
    }

    /* remove final newline */
    if (string->len > 0)
        g_string_set_size(string, string->len - 1);

    /* load modified lines */
    file = g_key_file_new();
    ret = g_key_file_load_from_data(file,
                                    string->str,
                                    -1,
                                    G_KEY_FILE_KEEP_COMMENTS,
                                    error);
    if (!ret) {
        g_key_file_free(file);
        return NULL;
    }
    return file;
}

/**
 * dnf_repo_loader_repo_parse_id:
 **/
static gboolean
dnf_repo_loader_repo_parse_id(DnfRepoLoader *self,
                              const gchar *id,
                              const gchar *filename,
                              GKeyFile *keyfile,
                              GError **error)
{
    DnfRepoLoaderPrivate *priv = GET_PRIVATE(self);
    g_autoptr(DnfRepo) repo = NULL;

    repo = dnf_repo_new(priv->context);
    dnf_repo_set_kind(repo, DNF_REPO_KIND_REMOTE);
    dnf_repo_set_keyfile(repo, keyfile);
    dnf_repo_set_filename(repo, filename);
    dnf_repo_set_id(repo, id);

    /* set up the repo ready for use */
    if (!dnf_repo_setup(repo, error))
        return FALSE;

    g_debug("added repo %s\t%s", filename, id);
    g_ptr_array_add(priv->repos, g_object_ref(repo));
    return TRUE;
}

/**
 * dnf_repo_loader_repo_parse:
 **/
static gboolean
dnf_repo_loader_repo_parse(DnfRepoLoader *self,
                           const gchar *filename,
                           GError **error)
{
    gboolean ret = TRUE;
    guint i;
    g_auto(GStrv) groups = NULL;
    g_autoptr(GKeyFile) keyfile = NULL;

    /* load non-standard keyfile */
    keyfile = dnf_repo_loader_load_multiline_key_file(filename, error);
    if (keyfile == NULL) {
        g_prefix_error(error, "Failed to load %s: ", filename);
        return FALSE;
    }

    /* save all the repos listed in the file */
    groups = g_key_file_get_groups(keyfile, NULL);
    for (i = 0; groups[i] != NULL; i++) {
        ret = dnf_repo_loader_repo_parse_id(self,
                                            groups[i],
                                            filename,
                                            keyfile,
                                            error);
        if (!ret)
            return FALSE;
    }
    return TRUE;
}

/**
 * dnf_repo_loader_refresh:
 */
static gboolean
dnf_repo_loader_refresh(DnfRepoLoader *self, GError **error)
{
    DnfRepoLoaderPrivate *priv = GET_PRIVATE(self);
    const gchar *file;
    const gchar *repo_path;
    g_autoptr(GDir) dir = NULL;

    if (!dnf_context_plugin_hook(priv->context, PLUGIN_HOOK_ID_CONTEXT_PRE_REPOS_RELOAD, nullptr, nullptr))
        return FALSE;

    /* no longer loaded */
    dnf_repo_loader_invalidate(self);
    g_ptr_array_set_size(priv->repos, 0);

    /* re-populate redhat.repo */
    if (!dnf_context_setup_enrollments(priv->context, error))
        return FALSE;

    /* open dir */
    repo_path = dnf_context_get_repo_dir(priv->context);
    dir = g_dir_open(repo_path, 0, error);
    if (dir == NULL)
        return FALSE;

    /* find all the .repo files */
    while ((file = g_dir_read_name(dir)) != NULL) {
        g_autofree gchar *path_tmp = NULL;
        if (!g_str_has_suffix(file, ".repo"))
            continue;
        path_tmp = g_build_filename(repo_path, file, NULL);
        if (!dnf_repo_loader_repo_parse(self, path_tmp, error))
            return FALSE;
    }

    /* add any DVD repos */
    if (!dnf_repo_loader_get_repos_removable(self, error))
        return FALSE;

    /* all okay */
    priv->loaded = TRUE;

    /* sort these in order of cost */
    g_ptr_array_sort(priv->repos, dnf_repo_loader_repo_cost_fn);
    return TRUE;
}

/**
 * dnf_repo_loader_has_removable_repos:
 */
gboolean
dnf_repo_loader_has_removable_repos(DnfRepoLoader *self)
{
    DnfRepoLoaderPrivate *priv = GET_PRIVATE(self);
    guint i;

    g_return_val_if_fail(DNF_IS_REPO_LOADER(self), FALSE);

    /* are there any media repos */
    for (i = 0; i < priv->repos->len; i++) {
        auto repo = static_cast<DnfRepo *>(g_ptr_array_index(priv->repos, i));
        if (dnf_repo_get_kind(repo) == DNF_REPO_KIND_MEDIA)
            return TRUE;
    }
    return FALSE;
}

/**
 * dnf_repo_loader_get_repos:
 *
 * Returns:(transfer container)(element-type DnfRepo): Array of repos
 */
GPtrArray *
dnf_repo_loader_get_repos(DnfRepoLoader *self, GError **error)
{
    DnfRepoLoaderPrivate *priv = GET_PRIVATE(self);

    g_return_val_if_fail(DNF_IS_REPO_LOADER(self), NULL);
    g_return_val_if_fail(error == NULL || *error == NULL, NULL);

    /* nothing set yet */
    if (!priv->loaded) {
        if (!dnf_repo_loader_refresh(self, error))
            return NULL;
    }

    /* all okay */
    return g_ptr_array_ref(priv->repos);
}

/**
 * dnf_repo_loader_get_repo_by_id:
 */
DnfRepo *
dnf_repo_loader_get_repo_by_id(DnfRepoLoader *self, const gchar *id, GError **error)
{
    DnfRepoLoaderPrivate *priv = GET_PRIVATE(self);
    guint i;

    g_return_val_if_fail(DNF_IS_REPO_LOADER(self), NULL);
    g_return_val_if_fail(id != NULL, NULL);
    g_return_val_if_fail(error == NULL || *error == NULL, NULL);

    /* nothing set yet */
    if (!priv->loaded) {
        if (!dnf_repo_loader_refresh(self, error))
            return NULL;
    }

    for (i = 0; i < priv->repos->len; i++) {
        auto tmp = static_cast<DnfRepo *>(g_ptr_array_index(priv->repos, i));
        if (g_strcmp0(dnf_repo_get_id(tmp), id) == 0)
            return tmp;
    }

    /* we didn't find anything */
    g_set_error(error,
                DNF_ERROR,
                DNF_ERROR_REPO_NOT_FOUND,
                "failed to find %s", id);
    return NULL;
}

/**
 * dnf_repo_loader_directory_changed_cb:
 **/
static void
dnf_repo_loader_directory_changed_cb(GFileMonitor *monitor_,
                                     GFile *file, GFile *other_file,
                                     GFileMonitorEvent event_type,
                                     DnfRepoLoader *self)
{
    g_debug("emit changed(ReposDir changed)");
    g_signal_emit(self, signals[SIGNAL_CHANGED], 0);
    dnf_repo_loader_invalidate(self);
}

/**
 * dnf_repo_loader_setup_watch:
 */
static void
dnf_repo_loader_setup_watch(DnfRepoLoader *self)
{
    DnfRepoLoaderPrivate *priv = GET_PRIVATE(self);
    const gchar *repo_dir;
    g_autoptr(GError) error = NULL;
    g_autoptr(GFile) file_repos = NULL;

    /* setup a file monitor on the repos directory */
    repo_dir = dnf_context_get_repo_dir(priv->context);
    if (repo_dir == NULL) {
        g_warning("no repodir set");
        return;
    }
    file_repos = g_file_new_for_path(repo_dir);
    priv->monitor_repos = g_file_monitor_directory(file_repos,
                                                   G_FILE_MONITOR_NONE,
                                                   NULL,
                                                   &error);
    if (priv->monitor_repos != NULL) {
        g_signal_connect(priv->monitor_repos, "changed",
                         G_CALLBACK(dnf_repo_loader_directory_changed_cb), self);
    } else {
        g_warning("failed to setup monitor: %s",
                  error->message);
    }
}

/**
 * dnf_repo_loader_new:
 * @context: A #DnfContext instance
 *
 * Creates a new #DnfRepoLoader.
 *
 * Returns:(transfer full): a #DnfRepoLoader
 *
 * Since: 0.1.0
 **/
DnfRepoLoader *
dnf_repo_loader_new(DnfContext *context)
{
    DnfRepoLoaderPrivate *priv;
    auto self = DNF_REPO_LOADER(g_object_new(DNF_TYPE_REPO_LOADER, NULL));
    priv = GET_PRIVATE(self);
    priv->context = context;
    g_object_add_weak_pointer(G_OBJECT(priv->context),(void **) &priv->context);
    dnf_repo_loader_setup_watch(self);
    return DNF_REPO_LOADER(self);
}
