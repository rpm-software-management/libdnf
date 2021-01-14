/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2008-2015 Richard Hughes <richard@hughsie.com>
 *
 * Most of this code was taken from Dnf, libzif/zif-self-test.c
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


#include <glib-object.h>
#include <stdlib.h>
#include <fcntl.h>
#include <glib/gstdio.h>
#include "libdnf/libdnf.h"

/**
 * cd_test_get_filename:
 **/
static gchar *
dnf_test_get_filename(const gchar *filename)
{
    char full_tmp[PATH_MAX];
    gchar *full = NULL;
    gchar *path;
    gchar *tmp;

    path = g_build_filename(TESTDATADIR, filename, NULL);
    tmp = realpath(path, full_tmp);
    if (tmp == NULL)
        goto out;
    full = g_strdup(full_tmp);
out:
    g_free(path);
    return full;
}

static guint _dnf_lock_state_changed = 0;

static void
dnf_lock_state_changed_cb(DnfLock *lock, guint bitfield, gpointer user_data)
{
    g_debug("lock state now %i", bitfield);
    _dnf_lock_state_changed++;
}

static void
dnf_lock_func(void)
{
    DnfLock *lock;
    gboolean ret;
    guint lock_id1;
    guint lock_id2;
    GError *error = NULL;

    lock = dnf_lock_new();
    dnf_lock_set_lock_dir(lock, "/tmp");
    g_assert(lock != NULL);
    g_signal_connect(lock, "state-changed",
              G_CALLBACK(dnf_lock_state_changed_cb), NULL);

    /* nothing yet! */
    g_assert_cmpint(dnf_lock_get_state(lock), ==, 0);
    ret = dnf_lock_release(lock, 999, &error);
    g_assert_error(error, DNF_ERROR, DNF_ERROR_INTERNAL_ERROR);
    g_assert(!ret);
    g_clear_error(&error);

    /* take one */
    lock_id1 = dnf_lock_take(lock,
                  DNF_LOCK_TYPE_RPMDB,
                  DNF_LOCK_MODE_PROCESS,
                  &error);
    g_assert_no_error(error);
    g_assert(lock_id1 != 0);
    g_assert_cmpint(dnf_lock_get_state(lock), ==, 1 << DNF_LOCK_TYPE_RPMDB);
    g_assert_cmpint(_dnf_lock_state_changed, ==, 1);

    /* take a different one */
    lock_id2 = dnf_lock_take(lock,
                             DNF_LOCK_TYPE_REPO,
                             DNF_LOCK_MODE_PROCESS,
                             &error);
    g_assert_no_error(error);
    g_assert(lock_id2 != 0);
    g_assert(lock_id2 != lock_id1);
    g_assert_cmpint(dnf_lock_get_state(lock), ==, 1 << DNF_LOCK_TYPE_RPMDB | 1 << DNF_LOCK_TYPE_REPO);
    g_assert_cmpint(_dnf_lock_state_changed, ==, 2);

    /* take two */
    lock_id1 = dnf_lock_take(lock,
                             DNF_LOCK_TYPE_RPMDB,
                             DNF_LOCK_MODE_PROCESS,
                             &error);
    g_assert_no_error(error);
    g_assert(lock_id1 != 0);
    g_assert_cmpint(dnf_lock_get_state(lock), ==, 1 << DNF_LOCK_TYPE_RPMDB | 1 << DNF_LOCK_TYPE_REPO);

    /* release one */
    ret = dnf_lock_release(lock, lock_id1, &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* release different one */
    ret = dnf_lock_release(lock, lock_id2, &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* release two */
    ret = dnf_lock_release(lock, lock_id1, &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* no more! */
    ret = dnf_lock_release(lock, lock_id1, &error);
    g_assert_error(error, DNF_ERROR, DNF_ERROR_INTERNAL_ERROR);
    g_assert(!ret);
    g_clear_error(&error);
    g_assert_cmpint(dnf_lock_get_state(lock), ==, 0);
    g_assert_cmpint(_dnf_lock_state_changed, ==, 6);

    g_object_unref(lock);
}

static gpointer
dnf_self_test_lock_thread_one(gpointer data)
{
    GError *error = NULL;
    guint lock_id;
    DnfLock *lock = DNF_LOCK(data);

    g_usleep(G_USEC_PER_SEC / 100);
    lock_id = dnf_lock_take(lock,
                            DNF_LOCK_TYPE_REPO,
                            DNF_LOCK_MODE_PROCESS,
                            &error);
    g_assert_error(error, DNF_ERROR, DNF_ERROR_CANNOT_GET_LOCK);
    g_assert_cmpint(lock_id, ==, 0);
    g_error_free(error);
    return NULL;
}

static void
dnf_lock_threads_func(void)
{
    gboolean ret;
    GError *error = NULL;
    GThread *one;
    guint lock_id;
    DnfLock *lock;

    /* take in master thread */
    lock = dnf_lock_new();
    dnf_lock_set_lock_dir(lock, "/tmp");
    lock_id = dnf_lock_take(lock,
                 DNF_LOCK_TYPE_REPO,
                 DNF_LOCK_MODE_PROCESS,
                 &error);
    g_assert_no_error(error);
    g_assert_cmpint(lock_id, >, 0);

    /* attempt to take in slave thread(should fail) */
    one = g_thread_new("dnf-lock-one",
                dnf_self_test_lock_thread_one,
                lock);

    /* block, waiting for thread */
    g_usleep(G_USEC_PER_SEC);

    /* release lock */
    ret = dnf_lock_release(lock, lock_id, &error);
    g_assert_no_error(error);
    g_assert(ret);

    g_thread_unref(one);
    g_object_unref(lock);
}

static void
ch_test_repo_func(void)
{
    DnfRepo *repo;
    DnfContext *context;
    context = dnf_context_new();
    repo = dnf_repo_new(context);
    g_object_unref(repo);
    g_object_unref(context);
}

static guint _allow_cancel_updates = 0;
static guint _action_updates = 0;
static guint _package_progress_updates = 0;
static guint _last_percent = 0;
static guint _updates = 0;

static void
dnf_state_test_percentage_changed_cb(DnfState *state, guint value, gpointer data)
{
    _last_percent = value;
    _updates++;
}

static void
dnf_state_test_allow_cancel_changed_cb(DnfState *state, gboolean allow_cancel, gpointer data)
{
    _allow_cancel_updates++;
}

static void
dnf_state_test_action_changed_cb(DnfState *state, DnfStateAction action, gpointer data)
{
    _action_updates++;
}

static void
dnf_state_test_package_progress_changed_cb(DnfState *state,
                        const gchar *dnf_package_get_id,
                        DnfStateAction action,
                        guint percentage,
                        gpointer data)
{
    g_assert(data == NULL);
    _package_progress_updates++;
}

#define DNF_STATE_ACTION_DOWNLOAD    1
#define DNF_STATE_ACTION_DEP_RESOLVE    2
#define DNF_STATE_ACTION_LOADING_CACHE    3

static void
dnf_state_func(void)
{
    gboolean ret;
    DnfState *state;

    _updates = 0;

    state = dnf_state_new();
    g_object_add_weak_pointer(G_OBJECT(state),(gpointer *) &state);
    g_assert(state != NULL);
    g_signal_connect(state, "percentage-changed", G_CALLBACK(dnf_state_test_percentage_changed_cb), NULL);
    g_signal_connect(state, "allow-cancel-changed", G_CALLBACK(dnf_state_test_allow_cancel_changed_cb), NULL);
    g_signal_connect(state, "action-changed", G_CALLBACK(dnf_state_test_action_changed_cb), NULL);
    g_signal_connect(state, "package-progress-changed", G_CALLBACK(dnf_state_test_package_progress_changed_cb), NULL);

    g_assert(dnf_state_get_allow_cancel(state));
    g_assert_cmpint(dnf_state_get_action(state), ==, DNF_STATE_ACTION_UNKNOWN);

    dnf_state_set_allow_cancel(state, TRUE);
    g_assert(dnf_state_get_allow_cancel(state));

    dnf_state_set_allow_cancel(state, FALSE);
    g_assert(!dnf_state_get_allow_cancel(state));
    g_assert_cmpint(_allow_cancel_updates, ==, 1);

    /* stop never started */
    g_assert(!dnf_state_action_stop(state));

    /* repeated */
    g_assert(dnf_state_action_start(state, DNF_STATE_ACTION_DOWNLOAD, NULL));
    g_assert(!dnf_state_action_start(state, DNF_STATE_ACTION_DOWNLOAD, NULL));
    g_assert_cmpint(dnf_state_get_action(state), ==, DNF_STATE_ACTION_DOWNLOAD);
    g_assert(dnf_state_action_stop(state));
    g_assert_cmpint(dnf_state_get_action(state), ==, DNF_STATE_ACTION_UNKNOWN);
    g_assert_cmpint(_action_updates, ==, 2);

    ret = dnf_state_set_number_steps(state, 5);
    g_assert(ret);

    ret = dnf_state_done(state, NULL);
    g_assert(ret);

    g_assert_cmpint(_updates, ==, 1);

    g_assert_cmpint(_last_percent, ==, 20);

    ret = dnf_state_done(state, NULL);
    g_assert(ret);
    ret = dnf_state_done(state, NULL);
    g_assert(ret);
    ret = dnf_state_done(state, NULL);
    g_assert(ret);
    dnf_state_set_package_progress(state,
                    "hal;0.0.1;i386;fedora",
                    DNF_STATE_ACTION_DOWNLOAD,
                    50);
    g_assert(dnf_state_done(state, NULL));

    g_assert(!dnf_state_done(state, NULL));
    g_assert_cmpint(_updates, ==, 5);
    g_assert_cmpint(_package_progress_updates, ==, 1);
    g_assert_cmpint(_last_percent, ==, 100);

    /* ensure allow cancel as we're done */
    g_assert(dnf_state_get_allow_cancel(state));

    g_object_unref(state);
    g_assert(state == NULL);
}

static void
dnf_state_child_func(void)
{
    gboolean ret;
    DnfState *state;
    DnfState *child;

    /* reset */
    _updates = 0;
    _allow_cancel_updates = 0;
    _action_updates = 0;
    _package_progress_updates = 0;
    state = dnf_state_new();
    g_object_add_weak_pointer(G_OBJECT(state),(gpointer *) &state);
    dnf_state_set_allow_cancel(state, TRUE);
    dnf_state_set_number_steps(state, 2);
    g_signal_connect(state, "percentage-changed", G_CALLBACK(dnf_state_test_percentage_changed_cb), NULL);
    g_signal_connect(state, "allow-cancel-changed", G_CALLBACK(dnf_state_test_allow_cancel_changed_cb), NULL);
    g_signal_connect(state, "action-changed", G_CALLBACK(dnf_state_test_action_changed_cb), NULL);
    g_signal_connect(state, "package-progress-changed", G_CALLBACK(dnf_state_test_package_progress_changed_cb), NULL);

    // state: |-----------------------|-----------------------|
    // step1: |-----------------------|
    // child:                         |-------------|---------|

    /* PARENT UPDATE */
    g_debug("parent update #1");
    ret = dnf_state_done(state, NULL);

    g_assert(ret);
    g_assert((_updates == 1));
    g_assert((_last_percent == 50));

    /* set parent state */
    g_debug("setting: depsolving-conflicts");
    dnf_state_action_start(state,
                DNF_STATE_ACTION_DEP_RESOLVE,
                "hal;0.1.0-1;i386;fedora");

    /* now test with a child */
    child = dnf_state_get_child(state);
    dnf_state_set_number_steps(child, 2);

    /* check child inherits parents action */
    g_assert_cmpint(dnf_state_get_action(child), ==,
             DNF_STATE_ACTION_DEP_RESOLVE);

    /* set child non-cancellable */
    dnf_state_set_allow_cancel(child, FALSE);

    /* ensure both are disallow-cancel */
    g_assert(!dnf_state_get_allow_cancel(child));
    g_assert(!dnf_state_get_allow_cancel(state));

    /* CHILD UPDATE */
    g_debug("setting: loading-rpmdb");
    g_assert(dnf_state_action_start(child, DNF_STATE_ACTION_LOADING_CACHE, NULL));
    g_assert_cmpint(dnf_state_get_action(child), ==,
             DNF_STATE_ACTION_LOADING_CACHE);

    g_debug("child update #1");
    ret = dnf_state_done(child, NULL);
    g_assert(ret);
    dnf_state_set_package_progress(child,
                    "hal;0.0.1;i386;fedora",
                    DNF_STATE_ACTION_DOWNLOAD,
                    50);

    g_assert_cmpint(_updates, ==, 2);
    g_assert_cmpint(_last_percent, ==, 75);
    g_assert_cmpint(_package_progress_updates, ==, 1);

    /* child action */
    g_debug("setting: downloading");
    g_assert(dnf_state_action_start(child,
                      DNF_STATE_ACTION_DOWNLOAD,
                      NULL));
    g_assert_cmpint(dnf_state_get_action(child), ==,
             DNF_STATE_ACTION_DOWNLOAD);

    /* CHILD UPDATE */
    g_debug("child update #2");
    ret = dnf_state_done(child, NULL);

    g_assert(ret);
    g_assert_cmpint(dnf_state_get_action(state), ==,
             DNF_STATE_ACTION_DEP_RESOLVE);
    g_assert(dnf_state_action_stop(state));
    g_assert(!dnf_state_action_stop(state));
    g_assert_cmpint(dnf_state_get_action(state), ==,
             DNF_STATE_ACTION_UNKNOWN);
    g_assert_cmpint(_action_updates, ==, 6);

    g_assert_cmpint(_updates, ==, 3);
    g_assert_cmpint(_last_percent, ==, 100);

    /* ensure the child finishing cleared the allow cancel on the parent */
    ret = dnf_state_get_allow_cancel(state);
    g_assert(ret);

    /* PARENT UPDATE */
    g_debug("parent update #2");
    ret = dnf_state_done(state, NULL);
    g_assert(ret);

    /* ensure we ignored the duplicate */
    g_assert_cmpint(_updates, ==, 3);
    g_assert_cmpint(_last_percent, ==, 100);

    g_object_unref(state);
    g_assert(state == NULL);
}

static void
dnf_state_parent_one_step_proxy_func(void)
{
    DnfState *state;
    DnfState *child;

    /* reset */
    _updates = 0;
    state = dnf_state_new();
    g_object_add_weak_pointer(G_OBJECT(state),(gpointer *) &state);
    dnf_state_set_number_steps(state, 1);
    g_signal_connect(state, "percentage-changed", G_CALLBACK(dnf_state_test_percentage_changed_cb), NULL);
    g_signal_connect(state, "allow-cancel-changed", G_CALLBACK(dnf_state_test_allow_cancel_changed_cb), NULL);

    /* now test with a child */
    child = dnf_state_get_child(state);
    dnf_state_set_number_steps(child, 2);

    /* CHILD SET VALUE */
    dnf_state_set_percentage(child, 33);

    /* ensure 1 updates for state with one step and ensure using child value as parent */
    g_assert(_updates == 1);
    g_assert(_last_percent == 33);

    g_object_unref(state);
    g_assert(state == NULL);
}

static void
dnf_state_non_equal_steps_func(void)
{
    gboolean ret;
    GError *error = NULL;
    DnfState *state;
    DnfState *child;
    DnfState *child_child;

    /* test non-equal steps */
    state = dnf_state_new();
    g_object_add_weak_pointer(G_OBJECT(state),(gpointer *) &state);
    dnf_state_set_enable_profile(state, TRUE);
    ret = dnf_state_set_steps(state,
                              &error,
                              20, /* prepare */
                              60, /* download */
                              10, /* install */
                              -1);
    g_assert_error(error, DNF_ERROR, DNF_ERROR_INTERNAL_ERROR);
    g_assert(!ret);
    g_clear_error(&error);

    /* okay this time */
    ret = dnf_state_set_steps(state, &error, 20, 60, 20, -1);
    g_assert_no_error(error);
    g_assert(ret);

    /* verify nothing */
    g_assert_cmpint(dnf_state_get_percentage(state), ==, 0);

    /* child step should increment according to the custom steps */
    child = dnf_state_get_child(state);
    dnf_state_set_number_steps(child, 2);

    /* start child */
    g_usleep(9 * 10 * 1000);
    ret = dnf_state_done(child, &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* verify 10% */
    g_assert_cmpint(dnf_state_get_percentage(state), ==, 10);

    /* finish child */
    g_usleep(9 * 10 * 1000);
    ret = dnf_state_done(child, &error);
    g_assert_no_error(error);
    g_assert(ret);

    ret = dnf_state_done(state, &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* verify 20% */
    g_assert_cmpint(dnf_state_get_percentage(state), ==, 20);

    /* child step should increment according to the custom steps */
    child = dnf_state_get_child(state);
    ret = dnf_state_set_steps(child,
                              &error,
                              25,
                              75,
                              -1);
    g_assert(ret);

    /* start child */
    g_usleep(25 * 10 * 1000);
    ret = dnf_state_done(child, &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* verify bilinear interpolation is working */
    g_assert_cmpint(dnf_state_get_percentage(state), ==, 35);

    /*
     * 0        20                             80         100
     * |---------||----------------------------||---------|
     *            |       35                   |
     *            |-------||-------------------|(25%)
     *                     |              75.5 |
     *                     |---------------||--|(90%)
     */
    child_child = dnf_state_get_child(child);
    ret = dnf_state_set_steps(child_child,
                   &error,
                   90,
                   10,
                   -1);
    g_assert_no_error(error);
    g_assert(ret);

    ret = dnf_state_done(child_child, &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* verify bilinear interpolation(twice) is working for subpercentage */
    g_assert_cmpint(dnf_state_get_percentage(state), ==, 75);

    ret = dnf_state_done(child_child, &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* finish child */
    g_usleep(25 * 10 * 1000);
    ret = dnf_state_done(child, &error);
    g_assert_no_error(error);
    g_assert(ret);

    ret = dnf_state_done(state, &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* verify 80% */
    g_assert_cmpint(dnf_state_get_percentage(state), ==, 80);

    g_usleep(19 * 10 * 1000);

    ret = dnf_state_done(state, &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* verify 100% */
    g_assert_cmpint(dnf_state_get_percentage(state), ==, 100);

    g_object_unref(state);
    g_assert(state == NULL);
}

static void
dnf_state_no_progress_func(void)
{
    gboolean ret;
    GError *error = NULL;
    DnfState *state;
    DnfState *child;

    /* test a state where we don't care about progress */
    state = dnf_state_new();
    g_object_add_weak_pointer(G_OBJECT(state),(gpointer *) &state);
    dnf_state_set_report_progress(state, FALSE);

    dnf_state_set_number_steps(state, 3);
    g_assert_cmpint(dnf_state_get_percentage(state), ==, 0);

    ret = dnf_state_done(state, &error);
    g_assert_no_error(error);
    g_assert(ret);
    g_assert_cmpint(dnf_state_get_percentage(state), ==, 0);

    ret = dnf_state_done(state, &error);
    g_assert_no_error(error);
    g_assert(ret);

    child = dnf_state_get_child(state);
    g_assert(child != NULL);
    dnf_state_set_number_steps(child, 2);
    ret = dnf_state_done(child, &error);
    g_assert_no_error(error);
    g_assert(ret);
    ret = dnf_state_done(child, &error);
    g_assert_no_error(error);
    g_assert(ret);
    g_assert_cmpint(dnf_state_get_percentage(state), ==, 0);

    g_object_unref(state);
    g_assert(state == NULL);
}

static void
dnf_state_finish_func(void)
{
    gboolean ret;
    GError *error = NULL;
    DnfState *state;
    DnfState *child;

    /* check straight finish */
    state = dnf_state_new();
    g_object_add_weak_pointer(G_OBJECT(state),(gpointer *) &state);
    dnf_state_set_number_steps(state, 3);

    child = dnf_state_get_child(state);
    dnf_state_set_number_steps(child, 3);
    ret = dnf_state_finished(child, &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* parent step done after child finish */
    ret = dnf_state_done(state, &error);
    g_assert_no_error(error);
    g_assert(ret);

    g_object_unref(state);
    g_assert(state == NULL);
}

static void
dnf_state_speed_func(void)
{
    DnfState *state;

    /* speed averaging test */
    state = dnf_state_new();
    g_object_add_weak_pointer(G_OBJECT(state),(gpointer *) &state);
    g_assert_cmpint(dnf_state_get_speed(state), ==, 0);
    dnf_state_set_speed(state, 100);
    g_assert_cmpint(dnf_state_get_speed(state), ==, 100);
    dnf_state_set_speed(state, 200);
    g_assert_cmpint(dnf_state_get_speed(state), ==, 150);
    dnf_state_set_speed(state, 300);
    g_assert_cmpint(dnf_state_get_speed(state), ==, 200);
    dnf_state_set_speed(state, 400);
    g_assert_cmpint(dnf_state_get_speed(state), ==, 250);
    dnf_state_set_speed(state, 500);
    g_assert_cmpint(dnf_state_get_speed(state), ==, 300);
    dnf_state_set_speed(state, 600);
    g_assert_cmpint(dnf_state_get_speed(state), ==, 400);
    g_object_unref(state);
    g_assert(state == NULL);
}

static void
dnf_state_finished_func(void)
{
    DnfState *state_local;
    DnfState *state;
    gboolean ret;
    GError *error = NULL;
    guint i;

    state = dnf_state_new();
    g_object_add_weak_pointer(G_OBJECT(state),(gpointer *) &state);
    ret = dnf_state_set_steps(state,
                              &error,
                              90,
                              10,
                              -1);
    g_assert_no_error(error);
    g_assert(ret);

    dnf_state_set_allow_cancel(state, FALSE);
    dnf_state_action_start(state,
                DNF_STATE_ACTION_LOADING_CACHE, "/");

    state_local = dnf_state_get_child(state);
    dnf_state_set_report_progress(state_local, FALSE);

    for (i = 0; i < 10; i++) {
        /* check cancelled(okay to reuse as we called
         * dnf_state_set_report_progress before)*/
        ret = dnf_state_done(state_local, &error);
        g_assert_no_error(error);
        g_assert(ret);
    }

    /* turn checks back on */
    dnf_state_set_report_progress(state_local, TRUE);
    ret = dnf_state_finished(state_local, &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* this section done */
    ret = dnf_state_done(state, &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* this section done */
    ret = dnf_state_done(state, &error);
    g_assert_no_error(error);
    g_assert(ret);

    g_object_unref(state);
    g_assert(state == NULL);
}

static void
dnf_state_locking_func(void)
{
    gboolean ret;
    GError *error = NULL;
    DnfState *state;
    DnfLock *lock;

    /* set, as is singleton */
    lock = dnf_lock_new();
    dnf_lock_set_lock_dir(lock, "/tmp");

    state = dnf_state_new();

    /* lock once */
    ret = dnf_state_take_lock(state,
                              DNF_LOCK_TYPE_RPMDB,
                              DNF_LOCK_MODE_PROCESS,
                              &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* succeeded, even again */
    ret = dnf_state_take_lock(state,
                              DNF_LOCK_TYPE_RPMDB,
                              DNF_LOCK_MODE_PROCESS,
                              &error);
    g_assert_no_error(error);
    g_assert(ret);

    g_object_unref(state);
    g_object_unref(lock);
}

static void
dnf_state_small_step_func(void)
{
    DnfState *state;
    gboolean ret;
    GError *error = NULL;
    guint i;

    _updates = 0;

    state = dnf_state_new();
    g_signal_connect(state, "percentage-changed",
            G_CALLBACK(dnf_state_test_percentage_changed_cb), NULL);
    dnf_state_set_number_steps(state, 100000);

    /* process all steps, we should get 100 callbacks */
    for (i = 0; i < 100000; i++) {
        ret = dnf_state_done(state, &error);
        g_assert_no_error(error);
        g_assert(ret);
    }
    g_assert_cmpint(_updates, ==, 100);

    g_object_unref(state);
}

static void
dnf_repo_loader_func(void)
{
    GError *error = NULL;
    DnfRepo *repo;
    DnfState *state;
    gboolean ret;
    g_autofree gchar *repos_dir = NULL;
    g_autoptr(DnfContext) ctx = NULL;
    g_autoptr(DnfRepoLoader) repo_loader = NULL;
    guint metadata_expire;

    /* set up local context */
    ctx = dnf_context_new();
    repos_dir = dnf_test_get_filename("yum.repos.d");
    dnf_context_set_repo_dir(ctx, repos_dir);
    dnf_context_set_solv_dir(ctx, "/tmp");
    ret = dnf_context_setup(ctx, NULL, &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* use this as a throw-away */
    state = dnf_context_get_state(ctx);

    /* load repos that need keyfile fixes */
    repo_loader = dnf_repo_loader_new(ctx);
    repo = dnf_repo_loader_get_repo_by_id(repo_loader, "bumblebee", &error);
    g_assert_no_error(error);
    g_assert(repo != NULL);
    g_assert_cmpint(dnf_repo_get_kind(repo), ==, DNF_REPO_KIND_REMOTE);
    g_assert(dnf_repo_get_gpgcheck(repo));
    g_assert(!dnf_repo_get_gpgcheck_md(repo));

    /* load repos that should be metadata enabled automatically */
    repo = dnf_repo_loader_get_repo_by_id(repo_loader, "redhat", &error);
    g_assert_no_error(error);
    g_assert(repo != NULL);
    g_assert_cmpint(dnf_repo_get_enabled(repo), ==, DNF_REPO_ENABLED_METADATA);
    g_assert_cmpint(dnf_repo_get_kind(repo), ==, DNF_REPO_KIND_REMOTE);
    g_assert(!dnf_repo_get_gpgcheck(repo));
    g_assert(!dnf_repo_get_gpgcheck_md(repo));

    /* load local metadata repo */
    repo = dnf_repo_loader_get_repo_by_id(repo_loader, "local", &error);
    g_assert_no_error(error);
    g_assert(repo != NULL);
    g_assert_cmpint(dnf_repo_get_enabled(repo), ==, DNF_REPO_ENABLED_METADATA |
                               DNF_REPO_ENABLED_PACKAGES);
    g_assert_cmpint(dnf_repo_get_kind(repo), ==, DNF_REPO_KIND_LOCAL);
    g_assert(!dnf_repo_get_gpgcheck(repo));
    g_assert(!dnf_repo_get_gpgcheck_md(repo));

    /* try to clean local repo */
    ret = dnf_repo_clean(repo, &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* try to refresh local repo */
    dnf_state_reset(state);
    ret = dnf_repo_update(repo, DNF_REPO_UPDATE_FLAG_NONE, state, &error);
    /* check the metadata expire attribute */
    metadata_expire = dnf_repo_get_metadata_expire(repo);
    g_assert_cmpuint(metadata_expire, == , 60 * 60 * 24);
    g_assert_no_error(error);
    g_assert(ret);

    /* try to check local repo that will not exist */
    dnf_state_reset(state);
    ret = dnf_repo_check(repo, 1, state, &error);
    g_assert_error(error, DNF_ERROR, DNF_ERROR_REPO_NOT_AVAILABLE);
    g_assert(!ret);
    g_clear_error(&error);
}

static void
dnf_context_func(void)
{
    GError *error = NULL;
    DnfContext *ctx;
    gboolean ret;

    ctx = dnf_context_new();
    dnf_context_set_solv_dir(ctx, "/tmp/hawkey");
    dnf_context_set_repo_dir(ctx, "/tmp");
    ret = dnf_context_setup(ctx, NULL, &error);
    g_assert_no_error(error);
    g_assert(ret);

    g_assert_cmpstr(dnf_context_get_base_arch(ctx), !=, NULL);
    g_assert_cmpstr(dnf_context_get_os_info(ctx), !=, NULL);
    g_assert_cmpstr(dnf_context_get_arch_info(ctx), !=, NULL);
    g_assert_cmpstr(dnf_context_get_release_ver(ctx), !=, NULL);
    g_assert_cmpstr(dnf_context_get_cache_dir(ctx), ==, NULL);
    g_assert_cmpstr(dnf_context_get_repo_dir(ctx), ==, "/tmp");
    g_assert_cmpstr(dnf_context_get_solv_dir(ctx), ==, "/tmp/hawkey");
    g_assert(dnf_context_get_check_disk_space(ctx));
    g_assert(dnf_context_get_check_transaction(ctx));
    dnf_context_set_keep_cache(ctx, FALSE);
    g_assert(!dnf_context_get_keep_cache(ctx));

    dnf_context_set_cache_dir(ctx, "/var");
    dnf_context_set_repo_dir(ctx, "/etc");
    g_assert_cmpstr(dnf_context_get_cache_dir(ctx), ==, "/var");
    g_assert_cmpstr(dnf_context_get_repo_dir(ctx), ==, "/etc");

    g_object_unref(ctx);
}

static void
dnf_repo_loader_gpg_no_pubkey_func(void)
{
    gboolean ret;
    g_autoptr(GError) error = NULL;
    g_autofree gchar *repos_dir = NULL;
    g_autoptr(DnfContext) ctx = NULL;

    /* set up local context */
    ctx = dnf_context_new();
    repos_dir = dnf_test_get_filename("gpg-no-pubkey");
    dnf_context_set_repo_dir(ctx, repos_dir);
    dnf_context_set_solv_dir(ctx, "/tmp");
    ret = dnf_context_setup(ctx, NULL, &error);
    g_assert_error(error, DNF_ERROR, DNF_ERROR_FILE_INVALID);
    g_assert(!ret);
}

static void
dnf_repo_loader_gpg_no_asc_func(void)
{
    DnfRepoLoader *repo_loader;
    DnfRepo *repo;
    gboolean ret;
    g_autoptr(GError) error = NULL;
    g_autofree gchar *repos_dir = NULL;
    g_autoptr(DnfContext) ctx = NULL;
    g_autoptr(DnfState) state = NULL;
    g_autoptr(DnfLock) lock = NULL;

    lock = dnf_lock_new();
    dnf_lock_set_lock_dir(lock, "/tmp");

    /* set up local context */
    ctx = dnf_context_new();
    repos_dir = dnf_test_get_filename("gpg-no-asc/yum.repos.d");
    dnf_context_set_repo_dir(ctx, repos_dir);
    dnf_context_set_solv_dir(ctx, "/tmp");
    ret = dnf_context_setup(ctx, NULL, &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* get the repo with no repomd.xml.asc */
    repo_loader = dnf_repo_loader_new(ctx);
    repo = dnf_repo_loader_get_repo_by_id(repo_loader, "gpg-repo-no-asc", &error);
    g_assert_no_error(error);
    g_assert(repo != NULL);

    /* check, which should fail as no local repomd.xml.asc exists */
    state = dnf_state_new();
    ret = dnf_repo_check(repo, G_MAXUINT, state, &error);
    g_assert_error(error, DNF_ERROR, DNF_ERROR_REPO_NOT_AVAILABLE);
    g_assert(!ret);
    g_clear_error(&error);

    /* update, which should fail as no *remote* repomd.xml.asc exists */
    dnf_state_reset(state);
    ret = dnf_repo_update(repo,
                            DNF_REPO_UPDATE_FLAG_FORCE |
                            DNF_REPO_UPDATE_FLAG_SIMULATE,
                            state, &error);
    if (g_error_matches(error,
                        DNF_ERROR,
                        DNF_ERROR_CANNOT_WRITE_REPO_CONFIG)) {
        g_debug("skipping tests: %s", error->message);
        return;
    }
    g_assert_error(error, DNF_ERROR, DNF_ERROR_CANNOT_FETCH_SOURCE);
    g_assert(!ret);
    g_clear_error(&error);
}

static void
dnf_repo_loader_gpg_wrong_asc_func(void)
{
    DnfRepoLoader *repo_loader;
    DnfRepo *repo;
    gboolean ret;
    g_autoptr(GError) error = NULL;
    g_autofree gchar *repos_dir = NULL;
    g_autoptr(DnfContext) ctx = NULL;
    g_autoptr(DnfState) state = NULL;
    g_autoptr(DnfLock) lock = NULL;

    lock = dnf_lock_new();
    dnf_lock_set_lock_dir(lock, "/tmp");

    /* set up local context */
    ctx = dnf_context_new();
    repos_dir = dnf_test_get_filename("gpg-wrong-asc/yum.repos.d");
    dnf_context_set_repo_dir(ctx, repos_dir);
    dnf_context_set_solv_dir(ctx, "/tmp");
    ret = dnf_context_setup(ctx, NULL, &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* get the repo with the *wrong* remote repomd.xml.asc */
    repo_loader = dnf_repo_loader_new(ctx);
    repo = dnf_repo_loader_get_repo_by_id(repo_loader, "gpg-repo-wrong-asc", &error);
    g_assert_no_error(error);
    g_assert(repo != NULL);

    /* update, which should fail as the repomd.xml.asc key is wrong */
    state = dnf_state_new();
    ret = dnf_repo_update(repo,
                            DNF_REPO_UPDATE_FLAG_FORCE |
                            DNF_REPO_UPDATE_FLAG_SIMULATE,
                            state, &error);
    if (g_error_matches(error,
                        DNF_ERROR,
                        DNF_ERROR_CANNOT_WRITE_REPO_CONFIG)) {
        g_debug("skipping tests: %s", error->message);
        return;
    }
    g_assert_error(error, DNF_ERROR, DNF_ERROR_CANNOT_FETCH_SOURCE);
    g_assert(!ret);
    g_clear_error(&error);
}

static void
dnf_repo_loader_gpg_asc_func(void)
{
    DnfRepoLoader *repo_loader;
    DnfRepo *repo;
    gboolean ret;
    g_autoptr(GError) error = NULL;
    g_autofree gchar *repos_dir = NULL;
    g_autoptr(DnfContext) ctx = NULL;
    g_autoptr(DnfState) state = NULL;
    g_autoptr(DnfLock) lock = NULL;

    lock = dnf_lock_new();
    dnf_lock_set_lock_dir(lock, "/tmp");

    /* set up local context */
    ctx = dnf_context_new();
    repos_dir = dnf_test_get_filename("gpg-asc/yum.repos.d");
    dnf_context_set_repo_dir(ctx, repos_dir);
    dnf_context_set_solv_dir(ctx, "/tmp");
    ret = dnf_context_setup(ctx, NULL, &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* get the repo with no repomd.xml.asc */
    repo_loader = dnf_repo_loader_new(ctx);
    repo = dnf_repo_loader_get_repo_by_id(repo_loader, "gpg-repo-asc", &error);
    g_assert_no_error(error);
    g_assert(repo != NULL);

    /* check, which should fail as there's no gnupg homedir with the key */
    state = dnf_state_new();
    ret = dnf_repo_check(repo, G_MAXUINT, state, &error);
    g_assert_error(error, DNF_ERROR, DNF_ERROR_REPO_NOT_AVAILABLE);
    g_assert(!ret);
    g_clear_error(&error);

    /* update, which should pass as a valid remote repomd.xml.asc exists */
    dnf_state_reset(state);
    ret = dnf_repo_update(repo,
                 DNF_REPO_UPDATE_FLAG_FORCE |
                 DNF_REPO_UPDATE_FLAG_SIMULATE,
                 state, &error);
    if (g_error_matches(error,
                 DNF_ERROR,
                 DNF_ERROR_CANNOT_WRITE_REPO_CONFIG)) {
        g_debug("skipping tests: %s", error->message);
        return;
    }
    g_assert_no_error(error);
    g_assert(ret);
}

static void
dnf_repo_loader_cache_dir_check_func(void)
{
    DnfRepoLoader *repo_loader;
    DnfRepo *repo;
    gboolean ret;
    g_autoptr(GError) error = NULL;
    g_autoptr(DnfContext) ctx = NULL;
    g_autofree gchar *repos_dir = NULL;
    g_autofree gchar *cache_dir = NULL;
    const char *cache_location = NULL;
    g_autofree gchar *expected_cache_suffix = NULL;

    /* set up local context*/
    ctx = dnf_context_new();
    repos_dir = dnf_test_get_filename("cache-test/yum.repos.d");
    cache_dir = g_build_filename(TESTDATADIR, "cache-test/cache-dir", NULL);
    dnf_context_set_repo_dir(ctx, repos_dir);
    dnf_context_set_solv_dir(ctx, "/tmp");
    dnf_context_set_cache_dir(ctx, cache_dir);

    ret = dnf_context_setup(ctx, NULL, &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* get the testing repo */
    repo_loader = dnf_repo_loader_new(ctx);
    repo = dnf_repo_loader_get_repo_by_id(repo_loader, "fedora", &error);
    g_assert_no_error(error);
    g_assert(repo != NULL);

    /* check the repo location to verify it has the correct suffix */
    cache_location = dnf_repo_get_location(repo);
    expected_cache_suffix =  g_strjoin("-", dnf_context_get_release_ver(ctx),
                                       dnf_context_get_base_arch(ctx), NULL);
    g_assert(g_str_has_suffix(cache_location, expected_cache_suffix));
}


static void
touch_file(const char *filename)
{
    int touch_fd = open(filename, O_CREAT|O_WRONLY|O_NOCTTY, 0644);
    g_assert_cmpint(touch_fd, !=, -1);
    g_assert_cmpint(close(touch_fd), ==, 0);
    g_assert(g_file_test(filename, G_FILE_TEST_EXISTS));
}

static void
dnf_context_cache_clean_check_func(void)
{

    DnfRepoLoader *repo_loader;
    DnfRepo *repo;
    gboolean ret;
    g_autoptr(GError) error = NULL;
    g_autoptr(DnfContext) ctx = NULL;
    g_autofree gchar *repos_dir = NULL;
    g_autofree gchar *cache_dir = NULL;
    const gchar* repo_location;
    guint file_result;

    /* set up local context*/
    ctx = dnf_context_new();
    repos_dir = dnf_test_get_filename("cache-test/yum.repos.d");
    cache_dir = g_build_filename(TESTDATADIR, "cache-test/cache-dir", NULL);
    dnf_context_set_repo_dir(ctx, repos_dir);
    dnf_context_set_solv_dir(ctx, "/tmp");
    dnf_context_set_cache_dir(ctx, cache_dir);
    dnf_context_set_lock_dir(ctx, cache_dir);

    ret = dnf_context_setup(ctx, NULL, &error);
    g_assert_no_error(error);
    g_assert(ret);

    /* get the repo location */
    repo_loader = dnf_repo_loader_new(ctx);
    repo = dnf_repo_loader_get_repo_by_id(repo_loader, "fedora", &error);
    repo_location = dnf_repo_get_location(repo);

    /* Create test files for different flags */
    g_autofree gchar* package_directory = g_build_filename(repo_location, "packages", NULL);
    file_result = g_mkdir_with_parents(package_directory, 0777);
    g_assert(file_result == 0);

    /* File for Cleaning Metadata */
    g_autofree gchar* repo_data_folder = g_build_filename(repo_location, "repodata", NULL);
    file_result = g_mkdir_with_parents(repo_data_folder, 0777);
    g_assert(file_result == 0);

    g_autofree gchar* xml_string = g_build_filename(repo_location, "metalink.xml", NULL);
    touch_file(xml_string);

    /* File for Cleaning Expired Cache */
    g_autofree gchar* expire_cache_file = g_build_filename(repo_location, "repomd.xml", NULL);
    touch_file(expire_cache_file);

    /* File that is not for any flag case, used for testing functionality */
    g_autofree gchar* non_matching_file = g_build_filename(repo_location, "nomatch.xxx", NULL);
    touch_file(non_matching_file);

    /* Then we do the cleaning with dnf_clean_cache, to demonstate it works */
    DnfContextCleanFlags flags = DNF_CONTEXT_CLEAN_EXPIRE_CACHE;
    flags |= DNF_CONTEXT_CLEAN_PACKAGES;
    flags |= DNF_CONTEXT_CLEAN_METADATA;

    ret = dnf_context_clean_cache(ctx, flags, &error);
    g_assert(ret);

    /* Verify the functionality of the function */
    g_assert(!g_file_test(package_directory, G_FILE_TEST_EXISTS));
    g_assert(!g_file_test(repo_data_folder, G_FILE_TEST_EXISTS));
    g_assert(!g_file_test(xml_string, G_FILE_TEST_EXISTS));
    g_assert(!g_file_test(expire_cache_file, G_FILE_TEST_EXISTS));
    g_assert(g_file_test(non_matching_file, G_FILE_TEST_EXISTS));

    /* At this stage we clean up the files that we created for testing */
    dnf_remove_recursive(cache_dir, &error);
    g_assert_no_error(error);
}

int
main(int argc, char **argv)
{
    g_setenv("G_MESSAGES_DEBUG", "all", FALSE);
    /* avoid gvfs (http://bugzilla.gnome.org/show_bug.cgi?id=526454) */
    /* Also because we do valgrind testing and there are vast array of
     * "leaks" when we load gio vfs modules.
     */
    g_setenv ("GIO_USE_VFS", "local", TRUE);

    g_test_init(&argc, &argv, NULL);

    /* only critical and error are fatal */
    g_log_set_fatal_mask(NULL, G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL);
    g_log_set_always_fatal (G_LOG_FATAL_MASK);

    /* tests go here */
    g_test_add_func("/libdnf/repo_loader{gpg-asc}", dnf_repo_loader_gpg_asc_func);
    g_test_add_func("/libdnf/repo_loader{gpg-wrong-asc}", dnf_repo_loader_gpg_wrong_asc_func);
    g_test_add_func("/libdnf/repo_loader{gpg-no-asc}", dnf_repo_loader_gpg_no_asc_func);
    g_test_add_func("/libdnf/repo_loader", dnf_repo_loader_func);
    g_test_add_func("/libdnf/repo_loader{gpg-no-pubkey}", dnf_repo_loader_gpg_no_pubkey_func);
    g_test_add_func("/libdnf/repo_loader{cache-dir-check}", dnf_repo_loader_cache_dir_check_func);
    g_test_add_func("/libdnf/context", dnf_context_func);
    g_test_add_func("/libdnf/context{cache-clean-check}", dnf_context_cache_clean_check_func);
    g_test_add_func("/libdnf/lock", dnf_lock_func);
    g_test_add_func("/libdnf/lock[threads]", dnf_lock_threads_func);
    g_test_add_func("/libdnf/repo", ch_test_repo_func);
    g_test_add_func("/libdnf/state", dnf_state_func);
    g_test_add_func("/libdnf/state[child]", dnf_state_child_func);
    g_test_add_func("/libdnf/state[parent-1-step]", dnf_state_parent_one_step_proxy_func);
    g_test_add_func("/libdnf/state[no-equal]", dnf_state_non_equal_steps_func);
    g_test_add_func("/libdnf/state[no-progress]", dnf_state_no_progress_func);
    g_test_add_func("/libdnf/state[finish]", dnf_state_finish_func);
    g_test_add_func("/libdnf/state[speed]", dnf_state_speed_func);
    g_test_add_func("/libdnf/state[locking]", dnf_state_locking_func);
    g_test_add_func("/libdnf/state[finished]", dnf_state_finished_func);
    g_test_add_func("/libdnf/state[small-step]", dnf_state_small_step_func);

    return g_test_run();
}
