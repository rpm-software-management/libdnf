/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2009-2015 Richard Hughes <richard@hughsie.com>
 *
 * Most of this code was taken from Zif, libzif/zif-state.c
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
 * SECTION:dnf-state
 * @short_description: Progress reporting
 * @include: libdnf.h
 * @stability: Unstable
 *
 * Objects can use dnf_state_set_percentage() if the absolute percentage
 * is known. Percentages should always go up, not down.
 *
 * Modules usually set the number of steps that are expected using
 * dnf_state_set_number_steps() and then after each section is completed,
 * the dnf_state_done() function should be called. This will automatically
 * call dnf_state_set_percentage() with the correct values.
 *
 * #DnfState allows sub-modules to be "chained up" to the parent module
 * so that as the sub-module progresses, so does the parent.
 * The child can be reused for each section, and chains can be deep.
 *
 * To get a child object, you should use dnf_state_get_child() and then
 * use the result in any sub-process. You should ensure that the child
 * is not re-used without calling dnf_state_done().
 *
 * There are a few nice touches in this module, so that if a module only has
 * one progress step, the child progress is used for updates.
 *
 * <example>
 *   <title>Using a #DnfState.</title>
 *   <programlisting>
 * static void
 * _do_something(DnfState *state)
 * {
 *    DnfState *state_local;
 *
 *    // setup correct number of steps
 *    dnf_state_set_number_steps(state, 2);
 *
 *    // we can't cancel this function
 *    dnf_state_set_allow_cancel(state, FALSE);
 *
 *    // run a sub function
 *    state_local = dnf_state_get_child(state);
 *    _do_something_else1(state_local);
 *
 *    // this section done
 *    dnf_state_done(state);
 *
 *    // run another sub function
 *    state_local = dnf_state_get_child(state);
 *    _do_something_else2(state_local);
 *
 *    // this section done(all complete)
 *    dnf_state_done(state);
 * }
 *   </programlisting>
 * </example>
 *
 * See also: #DnfLock
 */


#include "dnf-state.h"
#include "dnf-utils.h"

#include "utils/bgettext/bgettext-lib.h"

typedef struct
{
    gboolean         allow_cancel;
    gboolean         allow_cancel_changed_state;
    gboolean         allow_cancel_child;
    gboolean         enable_profile;
    gboolean         report_progress;
    GCancellable    *cancellable;
    gchar            *action_hint;
    gchar            *id;
    gdouble          *step_profile;
    GTimer           *timer;
    guint64           speed;
    guint64          *speed_data;
    guint             current;
    guint             last_percentage;
    guint            *step_data;
    guint             steps;
    gulong            action_child_id;
    gulong            package_progress_child_id;
    gulong            notify_speed_child_id;
    gulong            allow_cancel_child_id;
    gulong            percentage_child_id;
    DnfStateAction    action;
    DnfStateAction    last_action;
    DnfStateAction    child_action;
    DnfState         *child;
    DnfState         *parent;
    GPtrArray        *lock_ids;
    DnfLock          *lock;
} DnfStatePrivate;

enum {
    SIGNAL_PERCENTAGE_CHANGED,
    SIGNAL_SUBPERCENTAGE_CHANGED,
    SIGNAL_ALLOW_CANCEL_CHANGED,
    SIGNAL_ACTION_CHANGED,
    SIGNAL_PACKAGE_PROGRESS_CHANGED,
    SIGNAL_LAST
};

enum {
    PROP_0,
    PROP_SPEED,
    PROP_LAST
};

static guint signals [SIGNAL_LAST] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE(DnfState, dnf_state, G_TYPE_OBJECT)
#define GET_PRIVATE(o) (static_cast<DnfStatePrivate *>(dnf_state_get_instance_private (o)))

#define DNF_STATE_SPEED_SMOOTHING_ITEMS        5

/**
 * dnf_state_finalize:
 **/
static void
dnf_state_finalize(GObject *object)
{
    DnfState *state = DNF_STATE(object);
    DnfStatePrivate *priv = GET_PRIVATE(state);

    /* no more locks */
    dnf_state_release_locks(state);

    dnf_state_reset(state);
    g_free(priv->id);
    g_free(priv->action_hint);
    g_free(priv->step_data);
    g_free(priv->step_profile);
    if (priv->cancellable != NULL)
        g_object_unref(priv->cancellable);
    g_timer_destroy(priv->timer);
    g_free(priv->speed_data);
    g_ptr_array_unref(priv->lock_ids);
    g_object_unref(priv->lock);

    G_OBJECT_CLASS(dnf_state_parent_class)->finalize(object);
}

/**
 * dnf_state_init:
 **/
static void
dnf_state_init(DnfState *state)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);
    priv->allow_cancel = TRUE;
    priv->allow_cancel_child = TRUE;
    priv->action = DNF_STATE_ACTION_UNKNOWN;
    priv->last_action = DNF_STATE_ACTION_UNKNOWN;
    priv->timer = g_timer_new();
    priv->lock_ids = g_ptr_array_new();
    priv->report_progress = TRUE;
    priv->lock = dnf_lock_new();
    priv->speed_data = g_new0(guint64, DNF_STATE_SPEED_SMOOTHING_ITEMS);
}

/**
 * dnf_state_get_property:
 **/
static void
dnf_state_get_property(GObject *object,
            guint prop_id,
            GValue *value,
            GParamSpec *pspec)
{
    DnfState *state = DNF_STATE(object);
    DnfStatePrivate *priv = GET_PRIVATE(state);

    switch(prop_id) {
    case PROP_SPEED:
        g_value_set_uint64(value, priv->speed);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

/**
 * dnf_state_set_property:
 **/
static void
dnf_state_set_property(GObject *object,
            guint prop_id,
            const GValue *value,
            GParamSpec *pspec)
{
    DnfState *state = DNF_STATE(object);
    DnfStatePrivate *priv = GET_PRIVATE(state);

    switch(prop_id) {
    case PROP_SPEED:
        priv->speed = g_value_get_uint64(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

/**
 * dnf_state_class_init:
 **/
static void
dnf_state_class_init(DnfStateClass *klass)
{
    GParamSpec *pspec;
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = dnf_state_finalize;
    object_class->get_property = dnf_state_get_property;
    object_class->set_property = dnf_state_set_property;

    /**
     * DnfState:speed:
     **/
    pspec = g_param_spec_uint64("speed", NULL, NULL,
                                0, G_MAXUINT64, 0,
                                G_PARAM_READABLE);
    g_object_class_install_property(object_class, PROP_SPEED, pspec);

    signals [SIGNAL_PERCENTAGE_CHANGED] =
        g_signal_new("percentage-changed",
                     G_TYPE_FROM_CLASS(object_class), G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(DnfStateClass, percentage_changed),
                     NULL, NULL, g_cclosure_marshal_VOID__UINT,
                     G_TYPE_NONE, 1, G_TYPE_UINT);

    signals [SIGNAL_ALLOW_CANCEL_CHANGED] =
        g_signal_new("allow-cancel-changed",
                     G_TYPE_FROM_CLASS(object_class), G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(DnfStateClass, allow_cancel_changed),
                     NULL, NULL, g_cclosure_marshal_VOID__BOOLEAN,
                     G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

    signals [SIGNAL_ACTION_CHANGED] =
        g_signal_new("action-changed",
                     G_TYPE_FROM_CLASS(object_class), G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(DnfStateClass, action_changed),
                     NULL, NULL, g_cclosure_marshal_generic,
                     G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_STRING);

    signals [SIGNAL_PACKAGE_PROGRESS_CHANGED] =
        g_signal_new("package-progress-changed",
                     G_TYPE_FROM_CLASS(object_class), G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(DnfStateClass, package_progress_changed),
                     NULL, NULL, g_cclosure_marshal_generic,
                     G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT);
}

/**
 * dnf_state_set_report_progress:
 * @state: A #DnfState
 * @report_progress: if we care about percentage status
 *
 * This disables progress tracking for DnfState. This is generally a bad
 * thing to do, except when you know you cannot guess the number of steps
 * in the state.
 *
 * Using this function also reduced the amount of time spent getting a
 * child state using dnf_state_get_child() as a refcounted version of
 * the parent is returned instead. *
 * Since: 0.1.0
 **/
void
dnf_state_set_report_progress(DnfState *state, gboolean report_progress)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);
    priv->report_progress = report_progress;
}

/**
 * dnf_state_set_enable_profile:
 * @state: A #DnfState
 * @enable_profile: if profiling should be enabled
 *
 * This enables profiling of DnfState. This may be useful in development,
 * but be warned; enabling profiling makes #DnfState very slow.
 *
 * Since: 0.1.0
 **/
void
dnf_state_set_enable_profile(DnfState *state, gboolean enable_profile)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);
    priv->enable_profile = enable_profile;
}

/**
 * dnf_state_take_lock:
 * @state: A #DnfState
 * @lock_type: A #DnfLockType, e.g. %DNF_LOCK_TYPE_RPMDB
 * @lock_mode: A #DnfLockMode, e.g. %DNF_LOCK_MODE_PROCESS
 * @error: A #GError
 *
 * Takes a lock of a specified type.
 * The lock is automatically free'd when the DnfState has been completed.
 *
 * You can call dnf_state_take_lock() multiple times with different or
 * even the same @lock_type value.
 *
 * Returns: %FALSE if the lock is fatal, %TRUE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_state_take_lock(DnfState *state,
                    DnfLockType lock_type,
                    DnfLockMode lock_mode,
                    GError **error)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);
    guint lock_id = 0;

    /* no custom handler */
    lock_id = dnf_lock_take(priv->lock,
                            lock_type,
                            lock_mode,
                            error);
    if (lock_id == 0)
        return FALSE;

    /* add the lock to an array so we can release on completion */
    g_debug("adding lock %i", lock_id);
    g_ptr_array_add(priv->lock_ids,
                    GUINT_TO_POINTER(lock_id));
    return TRUE;
}

/**
 * dnf_state_discrete_to_percent:
 **/
static gfloat
dnf_state_discrete_to_percent(guint discrete, guint steps)
{
    /* check we are in range */
    if (discrete > steps)
        return 100;
    if (steps == 0) {
        g_warning("steps is 0!");
        return 0;
    }
    return((gfloat) ((gdouble) discrete *(100.0 /(gdouble)(steps))));
}

/**
 * dnf_state_print_parent_chain:
 **/
static void
dnf_state_print_parent_chain(DnfState *state, guint level)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);
    if (priv->parent != NULL)
        dnf_state_print_parent_chain(priv->parent, level + 1);
    g_print("%i) %s(%i/%i)\n",
            level, priv->id, priv->current, priv->steps);
}

/**
 * dnf_state_get_cancellable:
 * @state: A #DnfState
 *
 * Gets the #GCancellable for this operation
 *
 * Returns:(transfer none): The #GCancellable or %NULL
 *
 * Since: 0.1.0
 **/
GCancellable *
dnf_state_get_cancellable(DnfState *state)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);
    return priv->cancellable;
}

/**
 * dnf_state_set_cancellable:
 * @state: a #DnfState instance.
 * @cancellable: The #GCancellable which is used to cancel tasks, or %NULL
 *
 * Sets the #GCancellable object to use.
 *
 * Since: 0.1.0
 **/
void
dnf_state_set_cancellable(DnfState *state, GCancellable *cancellable)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);
    g_return_if_fail(priv->cancellable == NULL);
    if (priv->cancellable != NULL)
        g_clear_object(&priv->cancellable);
    if (cancellable != NULL)
        priv->cancellable = static_cast<GCancellable *>(g_object_ref(cancellable));
}

/**
 * dnf_state_get_allow_cancel:
 * @state: A #DnfState
 *
 * Gets if the sub-task(or one of it's sub-sub-tasks) is cancellable
 *
 * Returns: %TRUE if the translation has a chance of being cancelled
 *
 * Since: 0.1.0
 **/
gboolean
dnf_state_get_allow_cancel(DnfState *state)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);
    return priv->allow_cancel && priv->allow_cancel_child;
}

/**
 * dnf_state_set_allow_cancel:
 * @state: A #DnfState
 * @allow_cancel: If this sub-task can be cancelled
 *
 * Set is this sub task can be cancelled safely.
 *
 * Since: 0.1.0
 **/
void
dnf_state_set_allow_cancel(DnfState *state, gboolean allow_cancel)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);

    priv->allow_cancel_changed_state = TRUE;

    /* quick optimisation that saves lots of signals */
    if (priv->allow_cancel == allow_cancel)
        return;
    priv->allow_cancel = allow_cancel;

    /* just emit if both this and child is okay */
    g_signal_emit(state, signals [SIGNAL_ALLOW_CANCEL_CHANGED], 0,
                  priv->allow_cancel && priv->allow_cancel_child);
}

/**
 * dnf_state_get_speed:
 * @state: a #DnfState instance.
 *
 * Gets the transaction speed in bytes per second.
 *
 * Returns: speed, or 0 for unknown.
 *
 * Since: 0.1.0
 **/
guint64
dnf_state_get_speed(DnfState *state)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);
    return priv->speed;
}

/**
 * dnf_state_set_speed-private:
 **/
static void
dnf_state_set_speed_internal(DnfState *state, guint64 speed)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);
    if (priv->speed == speed)
        return;
    priv->speed = speed;
    g_object_notify(G_OBJECT(state), "speed");
}

/**
 * dnf_state_set_speed:
 * @state: a #DnfState instance.
 * @speed: The transaction speed.
 *
 * Sets the download or install transaction speed in bytes per second.
 *
 * Since: 0.1.0
 **/
void
dnf_state_set_speed(DnfState *state, guint64 speed)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);
    guint i;
    guint64 sum = 0;
    guint sum_cnt = 0;

    /* move the data down one entry */
    for (i=DNF_STATE_SPEED_SMOOTHING_ITEMS-1; i > 0; i--)
        priv->speed_data[i] = priv->speed_data[i-1];
    priv->speed_data[0] = speed;

    /* get the average */
    for (i = 0; i < DNF_STATE_SPEED_SMOOTHING_ITEMS; i++) {
        if (priv->speed_data[i] > 0) {
            sum += priv->speed_data[i];
            sum_cnt++;
        }
    }
    if (sum_cnt > 0)
        sum /= sum_cnt;

    dnf_state_set_speed_internal(state, sum);
}

/**
 * dnf_state_release_locks:
 * @state: a #DnfState instance.
 *
 * Releases all locks used in the state.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_state_release_locks(DnfState *state)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);
    guint i;
    guint lock_id;

    /* release children first */
    if (priv->child != NULL)
        dnf_state_release_locks(priv->child);

    /* release each one */
    for (i = 0; i < priv->lock_ids->len; i++) {
        lock_id = GPOINTER_TO_UINT(g_ptr_array_index(priv->lock_ids, i));
        g_debug("releasing lock %i", lock_id);
        if (!dnf_lock_release(priv->lock, lock_id, NULL))
            return FALSE;
    }
    g_ptr_array_set_size(priv->lock_ids, 0);
    return TRUE;
}

/**
 * dnf_state_set_percentage:
 * @state: a #DnfState instance.
 * @percentage: Percentage value between 0% and 100%
 *
 * Set a percentage manually.
 * NOTE: this must be above what was previously set, or it will be rejected.
 *
 * Returns: %TRUE if the signal was propagated, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_state_set_percentage(DnfState *state, guint percentage)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);

    /* do we care */
    if (!priv->report_progress)
        return TRUE;

    /* is it the same */
    if (percentage == priv->last_percentage)
        return FALSE;

    /* is it invalid */
    if (percentage > 100) {
        dnf_state_print_parent_chain(state, 0);
        g_warning("percentage %i%% is invalid on %p!",
                  percentage, state);
        return FALSE;
    }

    /* is it less */
    if (percentage < priv->last_percentage) {
        if (priv->enable_profile) {
            dnf_state_print_parent_chain(state, 0);
            g_warning("percentage should not go down from %i to %i on %p!",
                      priv->last_percentage, percentage, state);
        }
        return FALSE;
    }

    /* we're done, so we're not preventing cancellation anymore */
    if (percentage == 100 && !priv->allow_cancel) {
        g_debug("done, so allow cancel 1 for %p", state);
        dnf_state_set_allow_cancel(state, TRUE);
    }

    /* automatically cancel any action */
    if (percentage == 100 && priv->action != DNF_STATE_ACTION_UNKNOWN)
        dnf_state_action_stop(state);

    /* speed no longer valid */
    if (percentage == 100)
        dnf_state_set_speed_internal(state, 0);

    /* release locks? */
    if (percentage == 100) {
        if (!dnf_state_release_locks(state))
            return FALSE;
    }

    /* save */
    priv->last_percentage = percentage;

    /* emit */
    g_signal_emit(state, signals [SIGNAL_PERCENTAGE_CHANGED], 0, percentage);

    /* success */
    return TRUE;
}

/**
 * dnf_state_get_percentage:
 * @state: a #DnfState instance.
 *
 * Get the percentage state.
 *
 * Returns: The percentage value, or %G_MAXUINT for error
 *
 * Since: 0.1.0
 **/
guint
dnf_state_get_percentage(DnfState *state)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);
    return priv->last_percentage;
}

/**
 * dnf_state_action_start:
 * @state: a #DnfState instance.
 * @action: An action, e.g. %DNF_STATE_ACTION_DECOMPRESSING
 * @action_hint: A hint on what the action is doing, e.g. "/var/cache/yum/i386/15/koji/primary.sqlite"
 *
 * Sets the action which is being performed. This is emitted up the chain
 * to any parent %DnfState objects, using the action-changed signal.
 *
 * If a %DnfState reaches 100% then it is automatically stopped with a
 * call to dnf_state_action_stop().
 *
 * It is allowed to call dnf_state_action_start() more than once for a
 * given %DnfState instance.
 *
 * Returns: %TRUE if the signal was propagated, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_state_action_start(DnfState *state, DnfStateAction action, const gchar *action_hint)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);

    /* ignore this */
    if (action == DNF_STATE_ACTION_UNKNOWN) {
        g_warning("cannot set action DNF_STATE_ACTION_UNKNOWN");
        return FALSE;
    }

    /* is different? */
    if (priv->action == action &&
        g_strcmp0(action_hint, priv->action_hint) == 0)
        return FALSE;

    /* remember for stop */
    priv->last_action = priv->action;

    /* save hint */
    g_free(priv->action_hint);
    priv->action_hint = g_strdup(action_hint);

    /* save */
    priv->action = action;

    /* just emit */
    g_signal_emit(state, signals [SIGNAL_ACTION_CHANGED], 0, action, action_hint);
    return TRUE;
}

/**
 * dnf_state_set_package_progress:
 * @state: a #DnfState instance.
 * @dnf_package_get_id: A package_id
 * @action: A #DnfStateAction
 * @percentage: A percentage
 *
 * Sets any package progress.
 *
 * Since: 0.1.0
 **/
void
dnf_state_set_package_progress(DnfState *state,
                const gchar *dnf_package_get_id,
                DnfStateAction action,
                guint percentage)
{
    g_return_if_fail(dnf_package_get_id != NULL);
    g_return_if_fail(action != DNF_STATE_ACTION_UNKNOWN);
    g_return_if_fail(percentage <= 100);

    /* just emit */
    g_signal_emit(state, signals [SIGNAL_PACKAGE_PROGRESS_CHANGED], 0,
               dnf_package_get_id, action, percentage);
}

/**
 * dnf_state_action_stop:
 * @state: A #DnfState
 *
 * Returns the DnfState to it's previous value.
 * It is not expected you will ever need to use this function.
 *
 * Returns: %TRUE if the signal was propagated, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_state_action_stop(DnfState *state)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);

    /* nothing ever set */
    if (priv->action == DNF_STATE_ACTION_UNKNOWN) {
        g_debug("cannot unset action DNF_STATE_ACTION_UNKNOWN");
        return FALSE;
    }

    /* pop and reset */
    priv->action = priv->last_action;
    priv->last_action = DNF_STATE_ACTION_UNKNOWN;
    if (priv->action_hint != NULL) {
        g_free(priv->action_hint);
        priv->action_hint = NULL;
    }

    /* just emit */
    g_signal_emit(state, signals [SIGNAL_ACTION_CHANGED], 0, priv->action, NULL);
    return TRUE;
}

/**
 * dnf_state_get_action_hint:
 * @state: a #DnfState instance.
 *
 * Gets the action hint, which may be useful to the users.
 *
 * Returns: An a ction hint, e.g. "/var/cache/yum/i386/15/koji/primary.sqlite"
 *
 * Since: 0.1.0
 **/
const gchar *
dnf_state_get_action_hint(DnfState *state)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);
    return priv->action_hint;
}

/**
 * dnf_state_get_action:
 * @state: a #DnfState instance.
 *
 * Gets the last set action value.
 *
 * Returns: An action, e.g. %DNF_STATE_ACTION_DECOMPRESSING
 *
 * Since: 0.1.0
 **/
DnfStateAction
dnf_state_get_action(DnfState *state)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);
    return priv->action;
}

/**
 * dnf_state_child_percentage_changed_cb:
 **/
static void
dnf_state_child_percentage_changed_cb(DnfState *child, guint percentage, DnfState *state)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);
    gfloat offset;
    gfloat range;
    gfloat extra;
    guint parent_percentage;

    /* propagate up the stack if DnfState has only one step */
    if (priv->steps == 1) {
        dnf_state_set_percentage(state, percentage);
        return;
    }

    /* did we call done on a state that did not have a size set? */
    if (priv->steps == 0)
        return;

    /* already at >= 100% */
    if (priv->current >= priv->steps) {
        g_warning("already at %i/%i steps on %p", priv->current, priv->steps, state);
        return;
    }

    /* we have to deal with non-linear steps */
    if (priv->step_data != NULL) {
        /* we don't store zero */
        if (priv->current == 0) {
            parent_percentage = percentage * priv->step_data[priv->current] / 100;
        } else {
            /* bilinearly interpolate for speed */
            parent_percentage =(((100 - percentage) * priv->step_data[priv->current-1]) +
                        (percentage * priv->step_data[priv->current])) / 100;
        }
        goto out;
    }

    /* get the offset */
    offset = dnf_state_discrete_to_percent(priv->current, priv->steps);

    /* get the range between the parent step and the next parent step */
    range = dnf_state_discrete_to_percent(priv->current+1, priv->steps) - offset;
    if (range < 0.01) {
        g_warning("range=%f(from %i to %i), should be impossible", range, priv->current+1, priv->steps);
        return;
    }

    /* restore the pre-child action */
    if (percentage == 100)
        priv->last_action = priv->child_action;

    /* get the extra contributed by the child */
    extra =((gfloat) percentage / 100.0f) * range;

    /* emit from the parent */
    parent_percentage =(guint)(offset + extra);
out:
    dnf_state_set_percentage(state, parent_percentage);
}

/**
 * dnf_state_child_allow_cancel_changed_cb:
 **/
static void
dnf_state_child_allow_cancel_changed_cb(DnfState *child, gboolean allow_cancel, DnfState *state)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);

    /* save */
    priv->allow_cancel_child = allow_cancel;

    /* just emit if both this and child is okay */
    g_signal_emit(state, signals [SIGNAL_ALLOW_CANCEL_CHANGED], 0,
                  priv->allow_cancel && priv->allow_cancel_child);
}

/**
 * dnf_state_child_action_changed_cb:
 **/
static void
dnf_state_child_action_changed_cb(DnfState *child,
                   DnfStateAction action,
                   const gchar *action_hint,
                   DnfState *state)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);
    /* save */
    priv->action = action;

    /* just emit */
    g_signal_emit(state, signals [SIGNAL_ACTION_CHANGED], 0, action, action_hint);
}

/**
 * dnf_state_child_package_progress_changed_cb:
 **/
static void
dnf_state_child_package_progress_changed_cb(DnfState *child,
                         const gchar *dnf_package_get_id,
                         DnfStateAction action,
                         guint progress,
                         DnfState *state)
{
    /* just emit */
    g_signal_emit(state, signals [SIGNAL_PACKAGE_PROGRESS_CHANGED], 0,
                  dnf_package_get_id, action, progress);
}

/**
 * dnf_state_reset:
 * @state: a #DnfState instance.
 *
 * Resets the #DnfState object to unset
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_state_reset(DnfState *state)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);

    g_return_val_if_fail(DNF_IS_STATE(state), FALSE);

    /* do we care */
    if (!priv->report_progress)
        return TRUE;

    /* reset values */
    priv->steps = 0;
    priv->current = 0;
    priv->last_percentage = 0;

    /* only use the timer if profiling; it's expensive */
    if (priv->enable_profile)
        g_timer_start(priv->timer);

    /* disconnect client */
    if (priv->percentage_child_id != 0) {
        g_signal_handler_disconnect(priv->child, priv->percentage_child_id);
        priv->percentage_child_id = 0;
    }
    if (priv->allow_cancel_child_id != 0) {
        g_signal_handler_disconnect(priv->child, priv->allow_cancel_child_id);
        priv->allow_cancel_child_id = 0;
    }
    if (priv->action_child_id != 0) {
        g_signal_handler_disconnect(priv->child, priv->action_child_id);
        priv->action_child_id = 0;
    }
    if (priv->package_progress_child_id != 0) {
        g_signal_handler_disconnect(priv->child, priv->package_progress_child_id);
        priv->package_progress_child_id = 0;
    }
    if (priv->notify_speed_child_id != 0) {
        g_signal_handler_disconnect(priv->child, priv->notify_speed_child_id);
        priv->notify_speed_child_id = 0;
    }

    /* unref child */
    if (priv->child != NULL) {
        g_object_unref(priv->child);
        priv->child = NULL;
    }

    /* no more locks */
    dnf_state_release_locks(state);

    /* no more step data */
    g_free(priv->step_data);
    g_free(priv->step_profile);
    priv->step_data = NULL;
    priv->step_profile = NULL;
    return TRUE;
}

/**
 * dnf_state_child_notify_speed_cb:
 **/
static void
dnf_state_child_notify_speed_cb(DnfState *child,
                 GParamSpec *pspec,
                 DnfState *state)
{
    dnf_state_set_speed_internal(state,
                      dnf_state_get_speed(child));
}

/**
 * dnf_state_get_child:
 * @state: a #DnfState instance.
 *
 * Monitor a child state and proxy back up to the parent state.
 * You should not g_object_unref() this object, it is owned by the parent.
 *
 * Returns:(transfer none): A new %DnfState or %NULL for failure
 *
 * Since: 0.1.0
 **/
DnfState *
dnf_state_get_child(DnfState *state)
{
    DnfState *child = NULL;
    DnfStatePrivate *child_priv;
    DnfStatePrivate *priv = GET_PRIVATE(state);

    g_return_val_if_fail(DNF_IS_STATE(state), NULL);

    /* do we care */
    if (!priv->report_progress)
        return state;

    /* already set child */
    if (priv->child != NULL) {
        g_signal_handler_disconnect(priv->child,
                                    priv->percentage_child_id);
        g_signal_handler_disconnect(priv->child,
                                    priv->allow_cancel_child_id);
        g_signal_handler_disconnect(priv->child,
                                    priv->action_child_id);
        g_signal_handler_disconnect(priv->child,
                                    priv->package_progress_child_id);
        g_signal_handler_disconnect(priv->child,
                                    priv->notify_speed_child_id);
        g_object_unref(priv->child);
    }

    /* connect up signals */
    child = dnf_state_new();
    child_priv = GET_PRIVATE(child);
    child_priv->parent = state; /* do not ref! */
    priv->child = child;
    priv->percentage_child_id =
        g_signal_connect(child, "percentage-changed",
                  G_CALLBACK(dnf_state_child_percentage_changed_cb),
                  state);
    priv->allow_cancel_child_id =
        g_signal_connect(child, "allow-cancel-changed",
                  G_CALLBACK(dnf_state_child_allow_cancel_changed_cb),
                  state);
    priv->action_child_id =
        g_signal_connect(child, "action-changed",
                  G_CALLBACK(dnf_state_child_action_changed_cb),
                  state);
    priv->package_progress_child_id =
        g_signal_connect(child, "package-progress-changed",
                  G_CALLBACK(dnf_state_child_package_progress_changed_cb),
                  state);
    priv->notify_speed_child_id =
        g_signal_connect(child, "notify::speed",
                  G_CALLBACK(dnf_state_child_notify_speed_cb),
                  state);

    /* reset child */
    child_priv->current = 0;
    child_priv->last_percentage = 0;

    /* save so we can recover after child has done */
    child_priv->action = priv->action;
    priv->child_action = priv->action;

    /* set cancellable, creating if required */
    if (priv->cancellable == NULL)
        priv->cancellable = g_cancellable_new();
    dnf_state_set_cancellable(child, priv->cancellable);

    /* set the profile state */
    dnf_state_set_enable_profile(child, priv->enable_profile);
    return child;
}

/**
 * dnf_state_set_number_steps_real:
 * @state: a #DnfState instance.
 * @steps: The number of sub-tasks in this transaction, can be 0
 * @strloc: Code position identifier.
 *
 * Sets the number of sub-tasks, i.e. how many times the dnf_state_done()
 * function will be called in the loop.
 *
 * The function will immediately return with TRUE when the number of steps is 0
 * or if dnf_state_set_report_progress(FALSE) was previously called.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_state_set_number_steps_real(DnfState *state, guint steps, const gchar *strloc)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);

    g_return_val_if_fail(state != NULL, FALSE);

    /* nothing to do for 0 steps */
    if (steps == 0)
        return TRUE;

    /* do we care */
    if (!priv->report_progress)
        return TRUE;

    /* did we call done on a state that did not have a size set? */
    if (priv->steps != 0) {
        g_warning("steps already set to %i, can't set %i! [%s]",
                  priv->steps, steps, strloc);
        dnf_state_print_parent_chain(state, 0);
        return FALSE;
    }

    /* set id */
    g_free(priv->id);
    priv->id = g_strdup_printf("%s", strloc);

    /* only use the timer if profiling; it's expensive */
    if (priv->enable_profile)
        g_timer_start(priv->timer);

    /* set steps */
    priv->steps = steps;

    /* success */
    return TRUE;
}

/**
 * dnf_state_set_steps_real:
 * @state: a #DnfState instance.
 * @error: A #GError, or %NULL
 * @strloc: the code location
 * @value: A step weighting variable argument array
 * @...: -1 terminated step values
 *
 * This sets the step weighting, which you will want to do if one action
 * will take a bigger chunk of time than another.
 *
 * All the values must add up to 100, and the list must end with -1.
 * Do not use this function directly, instead use the dnf_state_set_steps() macro.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_state_set_steps_real(DnfState *state, GError **error, const gchar *strloc, gint value, ...)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);
    va_list args;
    guint i;
    gint value_temp;
    guint total;

    g_return_val_if_fail(state != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    /* do we care */
    if (!priv->report_progress)
        return TRUE;

    /* we must set at least one thing */
    total = value;

    /* process the valist */
    va_start(args, value);
    for (i = 0;; i++) {
        value_temp = va_arg(args, gint);
        if (value_temp == -1)
            break;
        total +=(guint) value_temp;
    }
    va_end(args);

    /* does not sum to 100% */
    if (total != 100) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    _("percentage not 100: %i"),
                    total);
        return FALSE;
    }

    /* set step number */
    if (!dnf_state_set_number_steps_real(state, i+1, strloc)) {
        g_set_error(error,
                    DNF_ERROR,
                    DNF_ERROR_INTERNAL_ERROR,
                    _("failed to set number steps: %i"),
                    i+1);
        return FALSE;
    }

    /* save this data */
    total = value;
    priv->step_data = g_new0(guint, i+2);
    priv->step_profile = g_new0(gdouble, i+2);
    priv->step_data[0] = total;
    va_start(args, value);
    for (i = 0;; i++) {
        value_temp = va_arg(args, gint);
        if (value_temp == -1)
            break;

        /* we pre-add the data to make access simpler */
        total +=(guint) value_temp;
        priv->step_data[i+1] = total;
    }
    va_end(args);

    /* success */
    return TRUE;
}

/**
 * dnf_state_show_profile:
 **/
static void
dnf_state_show_profile(DnfState *state)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);
    gdouble division;
    gdouble total_time = 0.0f;
    GString *result;
    guint i;
    guint uncumalitive = 0;

    /* get the total time */
    for (i = 0; i < priv->steps; i++)
        total_time += priv->step_profile[i];
    if (total_time < 0.01)
        return;

    /* get the total time so we can work out the divisor */
    result = g_string_new("Raw timing data was { ");
    for (i = 0; i < priv->steps; i++) {
        g_string_append_printf(result, "%.3f, ",
                               priv->step_profile[i]);
    }
    if (priv->steps > 0)
        g_string_set_size(result, result->len - 2);
    g_string_append(result, " }\n");

    /* what we set */
    g_string_append(result, "steps were set as [ ");
    for (i = 0; i < priv->steps; i++) {
        g_string_append_printf(result, "%i, ",
                               priv->step_data[i] - uncumalitive);
        uncumalitive = priv->step_data[i];
    }

    /* what we _should_ have set */
    g_string_append_printf(result, "-1 ] but should have been: [ ");
    division = total_time / 100.0f;
    for (i = 0; i < priv->steps; i++) {
        g_string_append_printf(result, "%.0f, ",
                               priv->step_profile[i] / division);
    }
    g_string_append(result, "-1 ]");
    g_printerr("\n\n%s at %s\n\n", result->str, priv->id);
    g_string_free(result, TRUE);
}

/**
 * dnf_state_check:
 * @state: a #DnfState instance.
 * @error: A #GError or %NULL
 *
 * Do any checks to see if the task has been cancelled.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_state_check(DnfState *state, GError **error)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);

    g_return_val_if_fail(state != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    /* are we cancelled */
    if (g_cancellable_is_cancelled(priv->cancellable)) {
        g_set_error_literal(error,
                            DNF_ERROR,
                            DNF_ERROR_CANCELLED,
                            _("cancelled by user action"));
        return FALSE;
    }
    return TRUE;
}

/**
 * dnf_state_done_real:
 * @state: a #DnfState instance.
 * @error: A #GError or %NULL
 * @strloc: Code position identifier.
 *
 * Called when the current sub-task has finished.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_state_done_real(DnfState *state, GError **error, const gchar *strloc)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);
    gdouble elapsed;
    gfloat percentage;

    g_return_val_if_fail(state != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    /* check */
    if (!dnf_state_check(state, error))
        return FALSE;

    /* do we care */
    if (!priv->report_progress)
        return TRUE;

    /* did we call done on a state that did not have a size set? */
    if (priv->steps == 0) {
        g_set_error(error, DNF_ERROR, DNF_ERROR_INTERNAL_ERROR,
                    _("done on a state %1$p that did not have a size set! [%2$s]"),
                    state, strloc);
        dnf_state_print_parent_chain(state, 0);
        return FALSE;
    }

    /* check the interval was too big in allow_cancel false mode */
    if (priv->enable_profile) {
        elapsed = g_timer_elapsed(priv->timer, NULL);
        if (!priv->allow_cancel_changed_state && priv->current > 0) {
            if (elapsed > 0.1f) {
                g_warning("%.1fms between dnf_state_done() and no dnf_state_set_allow_cancel()", elapsed * 1000);
                dnf_state_print_parent_chain(state, 0);
            }
        }

        /* save the duration in the array */
        if (priv->step_profile != NULL)
            priv->step_profile[priv->current] = elapsed;
        g_timer_start(priv->timer);
    }

    /* is already at 100%? */
    if (priv->current >= priv->steps) {
        g_set_error(error, DNF_ERROR, DNF_ERROR_INTERNAL_ERROR,
                    _("already at 100%% state [%s]"), strloc);
        dnf_state_print_parent_chain(state, 0);
        return FALSE;
    }

    /* is child not at 100%? */
    if (priv->child != NULL) {
        DnfStatePrivate *child_priv = GET_PRIVATE(priv->child);
        if (child_priv->current != child_priv->steps) {
            g_print("child is at %i/%i steps and parent done [%s]\n",
                    child_priv->current, child_priv->steps, strloc);
            dnf_state_print_parent_chain(priv->child, 0);
            /* do not abort, as we want to clean this up */
        }
    }

    /* we just checked for cancel, so it's not true to say we're blocking */
    dnf_state_set_allow_cancel(state, TRUE);

    /* another */
    priv->current++;

    /* find new percentage */
    if (priv->step_data == NULL) {
        percentage = dnf_state_discrete_to_percent(priv->current,
                                                   priv->steps);
    } else {
        /* this is cumalative, for speedy access */
        percentage = priv->step_data[priv->current - 1];
    }
    dnf_state_set_percentage(state,(guint) percentage);

    /* show any profiling stats */
    if (priv->enable_profile &&
        priv->current == priv->steps &&
        priv->step_profile != NULL) {
        dnf_state_show_profile(state);
    }

    /* reset child if it exists */
    if (priv->child != NULL)
        dnf_state_reset(priv->child);
    return TRUE;
}

/**
 * dnf_state_finished_real:
 * @state: A #DnfState
 * @error: A #GError or %NULL
 * @strloc: Code position identifier.
 *
 * Called when the current sub-task has finished.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
dnf_state_finished_real(DnfState *state, GError **error, const gchar *strloc)
{
    DnfStatePrivate *priv = GET_PRIVATE(state);

    g_return_val_if_fail(state != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    /* check */
    if (!dnf_state_check(state, error))
        return FALSE;

    /* is already at 100%? */
    if (priv->current == priv->steps)
        return TRUE;

    /* all done */
    priv->current = priv->steps;

    /* set new percentage */
    dnf_state_set_percentage(state, 100);
    return TRUE;
}

/**
 * dnf_state_new:
 *
 * Creates a new #DnfState.
 *
 * Returns:(transfer full): a #DnfState
 *
 * Since: 0.1.0
 **/
DnfState *
dnf_state_new(void)
{
    return DNF_STATE(g_object_new(DNF_TYPE_STATE, NULL));
}
