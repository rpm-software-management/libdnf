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
 * SECTION:hif-repos
 * @short_description: An object for loading several #HifRepo objects.
 * @include: libhif.h
 * @stability: Stable
 *
 * This object can create #HifRepo objects from either DVD media or from .repo
 * files in the repodir.
 *
 * See also: #HifRepo
 */

#include <strings.h>
#include "config.h"

#include <gio/gunixmounts.h>
#include <librepo/util.h>
#include <string.h>

#include "hif-package.h"
#include "hif-repos.h"
#include "hif-utils.h"

typedef struct
{
    GFileMonitor    *monitor_repos;
    HifContext      *context;    /* weak reference */
    GPtrArray       *repos;
    GVolumeMonitor  *volume_monitor;
    gboolean         loaded;
} HifReposPrivate;

enum {
    SIGNAL_CHANGED,
    SIGNAL_LAST
};

static guint signals[SIGNAL_LAST] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE(HifRepos, hif_repos, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (hif_repos_get_instance_private (o))

/**
 * hif_repos_finalize:
 **/
static void
hif_repos_finalize(GObject *object)
{
    HifRepos *repos = HIF_REPOS(object);
    HifReposPrivate *priv = GET_PRIVATE(repos);

    if (priv->monitor_repos != NULL)
        g_object_unref(priv->monitor_repos);
    g_object_unref(priv->volume_monitor);
    g_ptr_array_unref(priv->repos);

    G_OBJECT_CLASS(hif_repos_parent_class)->finalize(object);
}

/**
 * hif_repos_invalidate:
 */
static void
hif_repos_invalidate(HifRepos *repos)
{
    HifReposPrivate *priv = GET_PRIVATE(repos);
    priv->loaded = FALSE;
    hif_context_invalidate_full(priv->context, "repos.d invalidated",
                     HIF_CONTEXT_INVALIDATE_FLAG_ENROLLMENT);
}

/**
 * hif_repos_mount_changed_cb:
 */
static void
hif_repos_mount_changed_cb(GVolumeMonitor *vm, GMount *mount, HifRepos *repos)
{
    /* invalidate all repos if a CD is inserted / removed */
    g_debug("emit changed(mounts changed)");
    g_signal_emit(repos, signals[SIGNAL_CHANGED], 0);
    hif_repos_invalidate(repos);
}

/**
 * hif_repos_init:
 **/
static void
hif_repos_init(HifRepos *repos)
{
    HifReposPrivate *priv = GET_PRIVATE(repos);
    priv->repos = g_ptr_array_new_with_free_func((GDestroyNotify) g_object_unref);
    priv->volume_monitor = g_volume_monitor_get();
    g_signal_connect(priv->volume_monitor, "mount-added",
                     G_CALLBACK(hif_repos_mount_changed_cb), repos);
    g_signal_connect(priv->volume_monitor, "mount-removed",
                     G_CALLBACK(hif_repos_mount_changed_cb), repos);
}

/**
 * hif_repos_class_init:
 **/
static void
hif_repos_class_init(HifReposClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS(klass);

    /**
     * HifRepos::changed:
     **/
    signals[SIGNAL_CHANGED] =
        g_signal_new("changed",
                  G_TYPE_FROM_CLASS(object_class), G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET(HifReposClass, changed),
                  NULL, NULL, g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

    object_class->finalize = hif_repos_finalize;
}

/**
 * hif_repos_add_media:
 **/
static gboolean
hif_repos_add_media(HifRepos *repos,
             const gchar *mount_point,
             guint idx,
             GError **error)
{
    HifReposPrivate *priv = GET_PRIVATE(repos);
    HifRepo *repo;
    g_autofree gchar *packages = NULL;
    g_autofree gchar *treeinfo_fn;
    g_autoptr(GKeyFile) treeinfo;

    /* get common things */
    treeinfo_fn = g_build_filename(mount_point, ".treeinfo", NULL);
    treeinfo = g_key_file_new();
    if (!g_key_file_load_from_file(treeinfo, treeinfo_fn, 0, error))
        return FALSE;

    /* create read-only location */
    repo = hif_repo_new(priv->context);
    hif_repo_set_enabled(repo, HIF_REPO_ENABLED_PACKAGES);
    hif_repo_set_gpgcheck(repo, TRUE);
    hif_repo_set_kind(repo, HIF_REPO_KIND_MEDIA);
    hif_repo_set_cost(repo, 100);
    hif_repo_set_keyfile(repo, treeinfo);
    if (idx == 0) {
        hif_repo_set_id(repo, "media");
    } else {
        g_autofree gchar *tmp;
        tmp = g_strdup_printf("media-%i", idx);
        hif_repo_set_id(repo, tmp);
    }
    hif_repo_set_location(repo, mount_point);
    if (!hif_repo_setup(repo, error))
        return FALSE;

    g_debug("added repo %s", hif_repo_get_id(repo));
    g_ptr_array_add(priv->repos, repo);
    return TRUE;
}

/**
 * hif_repos_add_sack_from_mount_point:
 */
static gboolean
hif_repos_add_sack_from_mount_point(HifRepos *repos,
                     const gchar *root,
                     guint *idx,
                     GError **error)
{
    const gchar *id = ".treeinfo";
    gboolean exists;
    g_autofree gchar *treeinfo_fn;

    /* check if any installed media is an install disk */
    treeinfo_fn = g_build_filename(root, id, NULL);
    exists = g_file_test(treeinfo_fn, G_FILE_TEST_EXISTS);
    g_debug("checking %s for %s: %s", root, id, exists ? "yes" : "no");
    if (!exists)
        return TRUE;

    /* add the repodata/repomd.xml as a repo */
    if (!hif_repos_add_media(repos, root, *idx, error))
        return FALSE;
   (*idx)++;
    return TRUE;
}

/**
 * hif_repos_get_repos_removable:
 */
static gboolean
hif_repos_get_repos_removable(HifRepos *repos, GError **error)
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
        ret = hif_repos_add_sack_from_mount_point(repos,
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
 * hi_repos_repo_cost_fn:
 */
static gint
hi_repos_repo_cost_fn(gconstpointer a, gconstpointer b)
{
    HifRepo *repo_a = *((HifRepo **) a);
    HifRepo *repo_b = *((HifRepo **) b);
    if (hif_repo_get_cost(repo_a) < hif_repo_get_cost(repo_b))
        return -1;
    if (hif_repo_get_cost(repo_a) > hif_repo_get_cost(repo_b))
        return 1;
    return 0;
}

/**
 * hif_repos_load_multiline_key_file:
 **/
static GKeyFile *
hif_repos_load_multiline_key_file(const gchar *filename, GError **error)
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
 * hif_repos_repo_parse_id:
 **/
static gboolean
hif_repos_repo_parse_id(HifRepos *repos,
                          const gchar *id,
                          const gchar *filename,
                          GKeyFile *keyfile,
                          GError **error)
{
    HifReposPrivate *priv = GET_PRIVATE(repos);
    HifRepoEnabled enabled = 0;
    g_autoptr(HifRepo) repo;

    /* enabled isn't a required key */
    if (g_key_file_has_key(keyfile, id, "enabled", NULL)) {
        if (g_key_file_get_boolean(keyfile, id, "enabled", NULL))
            enabled |= HIF_REPO_ENABLED_PACKAGES;
    } else {
        enabled |= HIF_REPO_ENABLED_PACKAGES;
    }

    /* enabled_metadata isn't a required key */
    if (g_key_file_has_key(keyfile, id, "enabled_metadata", NULL)) {
        if (g_key_file_get_boolean(keyfile, id, "enabled_metadata", NULL))
            enabled |= HIF_REPO_ENABLED_METADATA;
    } else {
        g_autofree gchar *basename = NULL;
        basename = g_path_get_basename(filename);
        if (g_strcmp0(basename, "redhat.repo") == 0)
            enabled |= HIF_REPO_ENABLED_METADATA;
    }

    repo = hif_repo_new(priv->context);
    hif_repo_set_enabled(repo, enabled);
    hif_repo_set_kind(repo, HIF_REPO_KIND_REMOTE);
    hif_repo_set_keyfile(repo, keyfile);
    hif_repo_set_filename(repo, filename);
    hif_repo_set_id(repo, id);

    /* set up the repo ready for use */
    if (!hif_repo_setup(repo, error))
        return FALSE;

    g_debug("added repo %s\t%s", filename, id);
    g_ptr_array_add(priv->repos, g_object_ref(repo));
    return TRUE;
}

/**
 * hif_repos_repo_parse:
 **/
static gboolean
hif_repos_repo_parse(HifRepos *repos,
                       const gchar *filename,
                       GError **error)
{
    gboolean ret = TRUE;
    guint i;
    g_auto(GStrv) groups = NULL;
    g_autoptr(GKeyFile) keyfile;

    /* load non-standard keyfile */
    keyfile = hif_repos_load_multiline_key_file(filename, error);
    if (keyfile == NULL) {
        g_prefix_error(error, "Failed to load %s: ", filename);
        return FALSE;
    }

    /* save all the repos listed in the file */
    groups = g_key_file_get_groups(keyfile, NULL);
    for (i = 0; groups[i] != NULL; i++) {
        ret = hif_repos_repo_parse_id(repos,
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
 * hif_repos_refresh:
 */
static gboolean
hif_repos_refresh(HifRepos *repos, GError **error)
{
    HifReposPrivate *priv = GET_PRIVATE(repos);
    const gchar *file;
    const gchar *repo_path;
    g_autoptr(GDir) dir = NULL;

    /* no longer loaded */
    hif_repos_invalidate(repos);
    g_ptr_array_set_size(priv->repos, 0);

    /* re-populate redhat.repo */
    if (!hif_context_setup_enrollments(priv->context, error))
        return FALSE;

    /* open dir */
    repo_path = hif_context_get_repo_dir(priv->context);
    dir = g_dir_open(repo_path, 0, error);
    if (dir == NULL)
        return FALSE;

    /* find all the .repo files */
    while ((file = g_dir_read_name(dir)) != NULL) {
        g_autofree gchar *path_tmp = NULL;
        if (!g_str_has_suffix(file, ".repo"))
            continue;
        path_tmp = g_build_filename(repo_path, file, NULL);
        if (!hif_repos_repo_parse(repos, path_tmp, error))
            return FALSE;
    }

    /* add any DVD repos */
    if (!hif_repos_get_repos_removable(repos, error))
        return FALSE;

    /* all okay */
    priv->loaded = TRUE;

    /* sort these in order of cost */
    g_ptr_array_sort(priv->repos, hi_repos_repo_cost_fn);
    return TRUE;
}

/**
 * hif_repos_has_removable:
 */
gboolean
hif_repos_has_removable(HifRepos *repos)
{
    HifReposPrivate *priv = GET_PRIVATE(repos);
    HifRepo *repo;
    guint i;

    g_return_val_if_fail(HIF_IS_REPOS(repos), FALSE);

    /* are there any media repos */
    for (i = 0; i < priv->repos->len; i++) {
        repo = g_ptr_array_index(priv->repos, i);
        if (hif_repo_get_kind(repo) == HIF_REPO_KIND_MEDIA)
            return TRUE;
    }
    return FALSE;
}

/**
 * hif_repos_get_repos:
 *
 * Returns:(transfer container)(element-type HifRepo): Array of repos
 */
GPtrArray *
hif_repos_get_repos(HifRepos *repos, GError **error)
{
    HifReposPrivate *priv = GET_PRIVATE(repos);

    g_return_val_if_fail(HIF_IS_REPOS(repos), NULL);
    g_return_val_if_fail(error == NULL || *error == NULL, NULL);

    /* nothing set yet */
    if (!priv->loaded) {
        if (!hif_repos_refresh(repos, error))
            return NULL;
    }

    /* all okay */
    return g_ptr_array_ref(priv->repos);
}

/**
 * hif_repos_get_by_id:
 */
HifRepo *
hif_repos_get_by_id(HifRepos *repos, const gchar *id, GError **error)
{
    HifReposPrivate *priv = GET_PRIVATE(repos);
    HifRepo *tmp;
    guint i;

    g_return_val_if_fail(HIF_IS_REPOS(repos), NULL);
    g_return_val_if_fail(id != NULL, NULL);
    g_return_val_if_fail(error == NULL || *error == NULL, NULL);

    /* nothing set yet */
    if (!priv->loaded) {
        if (!hif_repos_refresh(repos, error))
            return NULL;
    }

    for (i = 0; i < priv->repos->len; i++) {
        tmp = g_ptr_array_index(priv->repos, i);
        if (g_strcmp0(hif_repo_get_id(tmp), id) == 0)
            return tmp;
    }

    /* we didn't find anything */
    g_set_error(error,
                HIF_ERROR,
                HIF_ERROR_REPO_NOT_FOUND,
                "failed to find %s", id);
    return NULL;
}

/**
 * hif_repos_directory_changed_cb:
 **/
static void
hif_repos_directory_changed_cb(GFileMonitor *monitor_,
                GFile *file, GFile *other_file,
                GFileMonitorEvent event_type,
                HifRepos *repos)
{
    g_debug("emit changed(ReposDir changed)");
    g_signal_emit(repos, signals[SIGNAL_CHANGED], 0);
    hif_repos_invalidate(repos);
}

/**
 * hif_repos_setup_watch:
 */
static void
hif_repos_setup_watch(HifRepos *repos)
{
    HifReposPrivate *priv = GET_PRIVATE(repos);
    const gchar *repo_dir;
    g_autoptr(GError) error = NULL;
    g_autoptr(GFile) file_repos = NULL;

    /* setup a file monitor on the repos directory */
    repo_dir = hif_context_get_repo_dir(priv->context);
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
                         G_CALLBACK(hif_repos_directory_changed_cb), repos);
    } else {
        g_warning("failed to setup monitor: %s",
                  error->message);
    }
}

/**
 * hif_repos_new:
 * @context: A #HifContext instance
 *
 * Creates a new #HifRepos.
 *
 * Returns:(transfer full): a #HifRepos
 *
 * Since: 0.1.0
 **/
HifRepos *
hif_repos_new(HifContext *context)
{
    HifReposPrivate *priv;
    HifRepos *repos;
    repos = g_object_new(HIF_TYPE_REPOS, NULL);
    priv = GET_PRIVATE(repos);
    priv->context = context;
    g_object_add_weak_pointer(G_OBJECT(priv->context),(void **) &priv->context);
    hif_repos_setup_watch(repos);
    return HIF_REPOS(repos);
}
