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
 * SECTION:hif-state
 * @short_description: Progress reporting
 * @include: libhif.h
 * @stability: Unstable
 *
 * Objects can use hif_state_set_percentage() if the absolute percentage
 * is known. Percentages should always go up, not down.
 *
 * Modules usually set the number of steps that are expected using
 * hif_state_set_number_steps() and then after each section is completed,
 * the hif_state_done() function should be called. This will automatically
 * call hif_state_set_percentage() with the correct values.
 *
 * #HifState allows sub-modules to be "chained up" to the parent module
 * so that as the sub-module progresses, so does the parent.
 * The child can be reused for each section, and chains can be deep.
 *
 * To get a child object, you should use hif_state_get_child() and then
 * use the result in any sub-process. You should ensure that the child
 * is not re-used without calling hif_state_done().
 *
 * There are a few nice touches in this module, so that if a module only has
 * one progress step, the child progress is used for updates.
 *
 * <example>
 *   <title>Using a #HifState.</title>
 *   <programlisting>
 * static void
 * _do_something(HifState *state)
 * {
 *    HifState *state_local;
 *
 *    // setup correct number of steps
 *    hif_state_set_number_steps(state, 2);
 *
 *    // we can't cancel this function
 *    hif_state_set_allow_cancel(state, FALSE);
 *
 *    // run a sub function
 *    state_local = hif_state_get_child(state);
 *    _do_something_else1(state_local);
 *
 *    // this section done
 *    hif_state_done(state);
 *
 *    // run another sub function
 *    state_local = hif_state_get_child(state);
 *    _do_something_else2(state_local);
 *
 *    // this section done(all complete)
 *    hif_state_done(state);
 * }
 *   </programlisting>
 * </example>
 *
 * See also: #HifLock
 */

#include "config.h"

#include "hif-cleanup.h"
#include "hif-state.h"
#include "hif-utils.h"

typedef struct _HifStatePrivate    HifStatePrivate;
struct _HifStatePrivate
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
    HifStateAction    action;
    HifStateAction    last_action;
    HifStateAction    child_action;
    HifState         *child;
    HifState         *parent;
    GPtrArray        *lock_ids;
    HifLock          *lock;
};

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

G_DEFINE_TYPE(HifState, hif_state, G_TYPE_OBJECT)
#define GET_PRIVATE(o)(G_TYPE_INSTANCE_GET_PRIVATE((o), HIF_TYPE_STATE, HifStatePrivate))

#define HIF_STATE_SPEED_SMOOTHING_ITEMS        5

/**
 * hif_state_finalize:
 **/
static void
hif_state_finalize(GObject *object)
{
    HifState *state = HIF_STATE(object);
    HifStatePrivate *priv = GET_PRIVATE(state);

    /* no more locks */
    hif_state_release_locks(state);

    hif_state_reset(state);
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

    G_OBJECT_CLASS(hif_state_parent_class)->finalize(object);
}

/**
 * hif_state_init:
 **/
static void
hif_state_init(HifState *state)
{
    HifStatePrivate *priv = GET_PRIVATE(state);
    priv->allow_cancel = TRUE;
    priv->allow_cancel_child = TRUE;
    priv->action = HIF_STATE_ACTION_UNKNOWN;
    priv->last_action = HIF_STATE_ACTION_UNKNOWN;
    priv->timer = g_timer_new();
    priv->lock_ids = g_ptr_array_new();
    priv->report_progress = TRUE;
    priv->lock = hif_lock_new();
    priv->speed_data = g_new0(guint64, HIF_STATE_SPEED_SMOOTHING_ITEMS);
}

/**
 * hif_state_get_property:
 **/
static void
hif_state_get_property(GObject *object,
            guint prop_id,
            GValue *value,
            GParamSpec *pspec)
{
    HifState *state = HIF_STATE(object);
    HifStatePrivate *priv = GET_PRIVATE(state);

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
 * hif_state_set_property:
 **/
static void
hif_state_set_property(GObject *object,
            guint prop_id,
            const GValue *value,
            GParamSpec *pspec)
{
    HifState *state = HIF_STATE(object);
    HifStatePrivate *priv = GET_PRIVATE(state);

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
 * hif_state_class_init:
 **/
static void
hif_state_class_init(HifStateClass *klass)
{
    GParamSpec *pspec;
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = hif_state_finalize;
    object_class->get_property = hif_state_get_property;
    object_class->set_property = hif_state_set_property;

    /**
     * HifState:speed:
     **/
    pspec = g_param_spec_uint64("speed", NULL, NULL,
                                0, G_MAXUINT64, 0,
                                G_PARAM_READABLE);
    g_object_class_install_property(object_class, PROP_SPEED, pspec);

    signals [SIGNAL_PERCENTAGE_CHANGED] =
        g_signal_new("percentage-changed",
                     G_TYPE_FROM_CLASS(object_class), G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(HifStateClass, percentage_changed),
                     NULL, NULL, g_cclosure_marshal_VOID__UINT,
                     G_TYPE_NONE, 1, G_TYPE_UINT);

    signals [SIGNAL_ALLOW_CANCEL_CHANGED] =
        g_signal_new("allow-cancel-changed",
                     G_TYPE_FROM_CLASS(object_class), G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(HifStateClass, allow_cancel_changed),
                     NULL, NULL, g_cclosure_marshal_VOID__BOOLEAN,
                     G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

    signals [SIGNAL_ACTION_CHANGED] =
        g_signal_new("action-changed",
                     G_TYPE_FROM_CLASS(object_class), G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(HifStateClass, action_changed),
                     NULL, NULL, g_cclosure_marshal_generic,
                     G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_STRING);

    signals [SIGNAL_PACKAGE_PROGRESS_CHANGED] =
        g_signal_new("package-progress-changed",
                     G_TYPE_FROM_CLASS(object_class), G_SIGNAL_RUN_LAST,
                     G_STRUCT_OFFSET(HifStateClass, package_progress_changed),
                     NULL, NULL, g_cclosure_marshal_generic,
                     G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_UINT, G_TYPE_UINT);
    g_type_class_add_private(klass, sizeof(HifStatePrivate));
}

/**
 * hif_state_set_report_progress:
 * @state: A #HifState
 * @report_progress: if we care about percentage status
 *
 * This disables progress tracking for HifState. This is generally a bad
 * thing to do, except when you know you cannot guess the number of steps
 * in the state.
 *
 * Using this function also reduced the amount of time spent getting a
 * child state using hif_state_get_child() as a refcounted version of
 * the parent is returned instead. *
 * Since: 0.1.0
 **/
void
hif_state_set_report_progress(HifState *state, gboolean report_progress)
{
    HifStatePrivate *priv = GET_PRIVATE(state);
    priv->report_progress = report_progress;
}

/**
 * hif_state_set_enable_profile:
 * @state: A #HifState
 * @enable_profile: if profiling should be enabled
 *
 * This enables profiling of HifState. This may be useful in development,
 * but be warned; enabling profiling makes #HifState very slow.
 *
 * Since: 0.1.0
 **/
void
hif_state_set_enable_profile(HifState *state, gboolean enable_profile)
{
    HifStatePrivate *priv = GET_PRIVATE(state);
    priv->enable_profile = enable_profile;
}

/**
 * hif_state_take_lock:
 * @state: A #HifState
 * @lock_type: A #HifLockType, e.g. %HIF_LOCK_TYPE_RPMDB
 * @lock_mode: A #HifLockMode, e.g. %HIF_LOCK_MODE_PROCESS
 * @error: A #GError
 *
 * Takes a lock of a specified type.
 * The lock is automatically free'd when the HifState has been completed.
 *
 * You can call hif_state_take_lock() multiple times with different or
 * even the same @lock_type value.
 *
 * Returns: %FALSE if the lock is fatal, %TRUE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_state_take_lock(HifState *state,
                    HifLockType lock_type,
                    HifLockMode lock_mode,
                    GError **error)
{
    HifStatePrivate *priv = GET_PRIVATE(state);
    guint lock_id = 0;

    /* no custom handler */
    lock_id = hif_lock_take(priv->lock,
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
 * hif_state_discrete_to_percent:
 **/
static gfloat
hif_state_discrete_to_percent(guint discrete, guint steps)
{
    /* check we are in range */
    if (discrete > steps)
        return 100;
    if (steps == 0) {
        g_warning("steps is 0!");
        return 0;
    }
    return((gfloat) discrete *(100.0f /(gfloat)(steps)));
}

/**
 * hif_state_print_parent_chain:
 **/
static void
hif_state_print_parent_chain(HifState *state, guint level)
{
    HifStatePrivate *priv = GET_PRIVATE(state);
    if (priv->parent != NULL)
        hif_state_print_parent_chain(priv->parent, level + 1);
    g_print("%i) %s(%i/%i)\n",
            level, priv->id, priv->current, priv->steps);
}

/**
 * hif_state_get_cancellable:
 * @state: A #HifState
 *
 * Gets the #GCancellable for this operation
 *
 * Returns:(transfer none): The #GCancellable or %NULL
 *
 * Since: 0.1.0
 **/
GCancellable *
hif_state_get_cancellable(HifState *state)
{
    HifStatePrivate *priv = GET_PRIVATE(state);
    return priv->cancellable;
}

/**
 * hif_state_set_cancellable:
 * @state: a #HifState instance.
 * @cancellable: The #GCancellable which is used to cancel tasks, or %NULL
 *
 * Sets the #GCancellable object to use.
 *
 * Since: 0.1.0
 **/
void
hif_state_set_cancellable(HifState *state, GCancellable *cancellable)
{
    HifStatePrivate *priv = GET_PRIVATE(state);
    g_return_if_fail(priv->cancellable == NULL);
    if (priv->cancellable != NULL)
        g_clear_object(&priv->cancellable);
    if (cancellable != NULL)
        priv->cancellable = g_object_ref(cancellable);
}

/**
 * hif_state_get_allow_cancel:
 * @state: A #HifState
 *
 * Gets if the sub-task(or one of it's sub-sub-tasks) is cancellable
 *
 * Returns: %TRUE if the translation has a chance of being cancelled
 *
 * Since: 0.1.0
 **/
gboolean
hif_state_get_allow_cancel(HifState *state)
{
    HifStatePrivate *priv = GET_PRIVATE(state);
    return priv->allow_cancel && priv->allow_cancel_child;
}

/**
 * hif_state_set_allow_cancel:
 * @state: A #HifState
 * @allow_cancel: If this sub-task can be cancelled
 *
 * Set is this sub task can be cancelled safely.
 *
 * Since: 0.1.0
 **/
void
hif_state_set_allow_cancel(HifState *state, gboolean allow_cancel)
{
    HifStatePrivate *priv = GET_PRIVATE(state);

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
 * hif_state_get_speed:
 * @state: a #HifState instance.
 *
 * Gets the transaction speed in bytes per second.
 *
 * Returns: speed, or 0 for unknown.
 *
 * Since: 0.1.0
 **/
guint64
hif_state_get_speed(HifState *state)
{
    HifStatePrivate *priv = GET_PRIVATE(state);
    return priv->speed;
}

/**
 * hif_state_set_speed_internal:
 **/
static void
hif_state_set_speed_internal(HifState *state, guint64 speed)
{
    HifStatePrivate *priv = GET_PRIVATE(state);
    if (priv->speed == speed)
        return;
    priv->speed = speed;
    g_object_notify(G_OBJECT(state), "speed");
}

/**
 * hif_state_set_speed:
 * @state: a #HifState instance.
 * @speed: The transaction speed.
 *
 * Sets the download or install transaction speed in bytes per second.
 *
 * Since: 0.1.0
 **/
void
hif_state_set_speed(HifState *state, guint64 speed)
{
    HifStatePrivate *priv = GET_PRIVATE(state);
    guint i;
    guint64 sum = 0;
    guint sum_cnt = 0;

    /* move the data down one entry */
    for (i=HIF_STATE_SPEED_SMOOTHING_ITEMS-1; i > 0; i--)
        priv->speed_data[i] = priv->speed_data[i-1];
    priv->speed_data[0] = speed;

    /* get the average */
    for (i = 0; i < HIF_STATE_SPEED_SMOOTHING_ITEMS; i++) {
        if (priv->speed_data[i] > 0) {
            sum += priv->speed_data[i];
            sum_cnt++;
        }
    }
    if (sum_cnt > 0)
        sum /= sum_cnt;

    hif_state_set_speed_internal(state, sum);
}

/**
 * hif_state_release_locks:
 * @state: a #HifState instance.
 *
 * Releases all locks used in the state.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_state_release_locks(HifState *state)
{
    HifStatePrivate *priv = GET_PRIVATE(state);
    guint i;
    guint lock_id;

    /* release children first */
    if (priv->child != NULL)
        hif_state_release_locks(priv->child);

    /* release each one */
    for (i = 0; i < priv->lock_ids->len; i++) {
        lock_id = GPOINTER_TO_UINT(g_ptr_array_index(priv->lock_ids, i));
        g_debug("releasing lock %i", lock_id);
        if (!hif_lock_release(priv->lock, lock_id, NULL))
            return FALSE;
    }
    g_ptr_array_set_size(priv->lock_ids, 0);
    return TRUE;
}

/**
 * hif_state_set_percentage:
 * @state: a #HifState instance.
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
hif_state_set_percentage(HifState *state, guint percentage)
{
    HifStatePrivate *priv = GET_PRIVATE(state);

    /* do we care */
    if (!priv->report_progress)
        return TRUE;

    /* is it the same */
    if (percentage == priv->last_percentage)
        return FALSE;

    /* is it invalid */
    if (percentage > 100) {
        hif_state_print_parent_chain(state, 0);
        g_warning("percentage %i%% is invalid on %p!",
                  percentage, state);
        return FALSE;
    }

    /* is it less */
    if (percentage < priv->last_percentage) {
        if (priv->enable_profile) {
            hif_state_print_parent_chain(state, 0);
            g_warning("percentage should not go down from %i to %i on %p!",
                      priv->last_percentage, percentage, state);
        }
        return FALSE;
    }

    /* we're done, so we're not preventing cancellation anymore */
    if (percentage == 100 && !priv->allow_cancel) {
        g_debug("done, so allow cancel 1 for %p", state);
        hif_state_set_allow_cancel(state, TRUE);
    }

    /* automatically cancel any action */
    if (percentage == 100 && priv->action != HIF_STATE_ACTION_UNKNOWN)
        hif_state_action_stop(state);

    /* speed no longer valid */
    if (percentage == 100)
        hif_state_set_speed_internal(state, 0);

    /* release locks? */
    if (percentage == 100) {
        if (!hif_state_release_locks(state))
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
 * hif_state_get_percentage:
 * @state: a #HifState instance.
 *
 * Get the percentage state.
 *
 * Returns: The percentage value, or %G_MAXUINT for error
 *
 * Since: 0.1.0
 **/
guint
hif_state_get_percentage(HifState *state)
{
    HifStatePrivate *priv = GET_PRIVATE(state);
    return priv->last_percentage;
}

/**
 * hif_state_action_start:
 * @state: a #HifState instance.
 * @action: An action, e.g. %HIF_STATE_ACTION_DECOMPRESSING
 * @action_hint: A hint on what the action is doing, e.g. "/var/cache/yum/i386/15/koji/primary.sqlite"
 *
 * Sets the action which is being performed. This is emitted up the chain
 * to any parent %HifState objects, using the action-changed signal.
 *
 * If a %HifState reaches 100% then it is automatically stopped with a
 * call to hif_state_action_stop().
 *
 * It is allowed to call hif_state_action_start() more than once for a
 * given %HifState instance.
 *
 * Returns: %TRUE if the signal was propagated, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_state_action_start(HifState *state, HifStateAction action, const gchar *action_hint)
{
    HifStatePrivate *priv = GET_PRIVATE(state);

    /* ignore this */
    if (action == HIF_STATE_ACTION_UNKNOWN) {
        g_warning("cannot set action HIF_STATE_ACTION_UNKNOWN");
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
 * hif_state_set_package_progress:
 * @state: a #HifState instance.
 * @package_id: A package_id
 * @action: A #HifStateAction
 * @percentage: A percentage
 *
 * Sets any package progress.
 *
 * Since: 0.1.0
 **/
void
hif_state_set_package_progress(HifState *state,
                const gchar *package_id,
                HifStateAction action,
                guint percentage)
{
    g_return_if_fail(package_id != NULL);
    g_return_if_fail(action != HIF_STATE_ACTION_UNKNOWN);
    g_return_if_fail(percentage <= 100);

    /* just emit */
    g_signal_emit(state, signals [SIGNAL_PACKAGE_PROGRESS_CHANGED], 0,
               package_id, action, percentage);
}

/**
 * hif_state_action_stop:
 * @state: A #HifState
 *
 * Returns the HifState to it's previous value.
 * It is not expected you will ever need to use this funtion.
 *
 * Returns: %TRUE if the signal was propagated, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_state_action_stop(HifState *state)
{
    HifStatePrivate *priv = GET_PRIVATE(state);

    /* nothing ever set */
    if (priv->action == HIF_STATE_ACTION_UNKNOWN) {
        g_debug("cannot unset action HIF_STATE_ACTION_UNKNOWN");
        return FALSE;
    }

    /* pop and reset */
    priv->action = priv->last_action;
    priv->last_action = HIF_STATE_ACTION_UNKNOWN;
    if (priv->action_hint != NULL) {
        g_free(priv->action_hint);
        priv->action_hint = NULL;
    }

    /* just emit */
    g_signal_emit(state, signals [SIGNAL_ACTION_CHANGED], 0, priv->action, NULL);
    return TRUE;
}

/**
 * hif_state_get_action_hint:
 * @state: a #HifState instance.
 *
 * Gets the action hint, which may be useful to the users.
 *
 * Returns: An a ction hint, e.g. "/var/cache/yum/i386/15/koji/primary.sqlite"
 *
 * Since: 0.1.0
 **/
const gchar *
hif_state_get_action_hint(HifState *state)
{
    HifStatePrivate *priv = GET_PRIVATE(state);
    return priv->action_hint;
}

/**
 * hif_state_get_action:
 * @state: a #HifState instance.
 *
 * Gets the last set action value.
 *
 * Returns: An action, e.g. %HIF_STATE_ACTION_DECOMPRESSING
 *
 * Since: 0.1.0
 **/
HifStateAction
hif_state_get_action(HifState *state)
{
    HifStatePrivate *priv = GET_PRIVATE(state);
    return priv->action;
}

/**
 * hif_state_child_percentage_changed_cb:
 **/
static void
hif_state_child_percentage_changed_cb(HifState *child, guint percentage, HifState *state)
{
    HifStatePrivate *priv = GET_PRIVATE(state);
    gfloat offset;
    gfloat range;
    gfloat extra;
    guint parent_percentage;

    /* propagate up the stack if HifState has only one step */
    if (priv->steps == 1) {
        hif_state_set_percentage(state, percentage);
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
    offset = hif_state_discrete_to_percent(priv->current, priv->steps);

    /* get the range between the parent step and the next parent step */
    range = hif_state_discrete_to_percent(priv->current+1, priv->steps) - offset;
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
    hif_state_set_percentage(state, parent_percentage);
}

/**
 * hif_state_child_allow_cancel_changed_cb:
 **/
static void
hif_state_child_allow_cancel_changed_cb(HifState *child, gboolean allow_cancel, HifState *state)
{
    HifStatePrivate *priv = GET_PRIVATE(state);

    /* save */
    priv->allow_cancel_child = allow_cancel;

    /* just emit if both this and child is okay */
    g_signal_emit(state, signals [SIGNAL_ALLOW_CANCEL_CHANGED], 0,
                  priv->allow_cancel && priv->allow_cancel_child);
}

/**
 * hif_state_child_action_changed_cb:
 **/
static void
hif_state_child_action_changed_cb(HifState *child,
                   HifStateAction action,
                   const gchar *action_hint,
                   HifState *state)
{
    HifStatePrivate *priv = GET_PRIVATE(state);
    /* save */
    priv->action = action;

    /* just emit */
    g_signal_emit(state, signals [SIGNAL_ACTION_CHANGED], 0, action, action_hint);
}

/**
 * hif_state_child_package_progress_changed_cb:
 **/
static void
hif_state_child_package_progress_changed_cb(HifState *child,
                         const gchar *package_id,
                         HifStateAction action,
                         guint progress,
                         HifState *state)
{
    /* just emit */
    g_signal_emit(state, signals [SIGNAL_PACKAGE_PROGRESS_CHANGED], 0,
                  package_id, action, progress);
}

/**
 * hif_state_reset:
 * @state: a #HifState instance.
 *
 * Resets the #HifState object to unset
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_state_reset(HifState *state)
{
    HifStatePrivate *priv = GET_PRIVATE(state);

    g_return_val_if_fail(HIF_IS_STATE(state), FALSE);

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
    hif_state_release_locks(state);

    /* no more step data */
    g_free(priv->step_data);
    g_free(priv->step_profile);
    priv->step_data = NULL;
    priv->step_profile = NULL;
    return TRUE;
}

/**
 * hif_state_child_notify_speed_cb:
 **/
static void
hif_state_child_notify_speed_cb(HifState *child,
                 GParamSpec *pspec,
                 HifState *state)
{
    hif_state_set_speed_internal(state,
                      hif_state_get_speed(child));
}

/**
 * hif_state_get_child:
 * @state: a #HifState instance.
 *
 * Monitor a child state and proxy back up to the parent state.
 * You should not g_object_unref() this object, it is owned by the parent.
 *
 * Returns:(transfer none): A new %HifState or %NULL for failure
 *
 * Since: 0.1.0
 **/
HifState *
hif_state_get_child(HifState *state)
{
    HifState *child = NULL;
    HifStatePrivate *child_priv;
    HifStatePrivate *priv = GET_PRIVATE(state);

    g_return_val_if_fail(HIF_IS_STATE(state), NULL);

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
    child = hif_state_new();
    child_priv = GET_PRIVATE(child);
    child_priv->parent = state; /* do not ref! */
    priv->child = child;
    priv->percentage_child_id =
        g_signal_connect(child, "percentage-changed",
                  G_CALLBACK(hif_state_child_percentage_changed_cb),
                  state);
    priv->allow_cancel_child_id =
        g_signal_connect(child, "allow-cancel-changed",
                  G_CALLBACK(hif_state_child_allow_cancel_changed_cb),
                  state);
    priv->action_child_id =
        g_signal_connect(child, "action-changed",
                  G_CALLBACK(hif_state_child_action_changed_cb),
                  state);
    priv->package_progress_child_id =
        g_signal_connect(child, "package-progress-changed",
                  G_CALLBACK(hif_state_child_package_progress_changed_cb),
                  state);
    priv->notify_speed_child_id =
        g_signal_connect(child, "notify::speed",
                  G_CALLBACK(hif_state_child_notify_speed_cb),
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
    hif_state_set_cancellable(child, priv->cancellable);

    /* set the profile state */
    hif_state_set_enable_profile(child, priv->enable_profile);
    return child;
}

/**
 * hif_state_set_number_steps_real:
 * @state: a #HifState instance.
 * @steps: The number of sub-tasks in this transaction, can be 0
 * @strloc: Code position identifier.
 *
 * Sets the number of sub-tasks, i.e. how many times the hif_state_done()
 * function will be called in the loop.
 *
 * The function will immediately return with TRUE when the number of steps is 0
 * or if hif_state_set_report_progress(FALSE) was previously called.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_state_set_number_steps_real(HifState *state, guint steps, const gchar *strloc)
{
    HifStatePrivate *priv = GET_PRIVATE(state);

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
        hif_state_print_parent_chain(state, 0);
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
 * hif_state_set_steps_real:
 * @state: a #HifState instance.
 * @error: A #GError, or %NULL
 * @strloc: the code location
 * @value: A step weighting variable argument array
 * @...: -1 terminated step values
 *
 * This sets the step weighting, which you will want to do if one action
 * will take a bigger chunk of time than another.
 *
 * All the values must add up to 100, and the list must end with -1.
 * Do not use this funtion directly, instead use the hif_state_set_steps() macro.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_state_set_steps_real(HifState *state, GError **error, const gchar *strloc, gint value, ...)
{
    HifStatePrivate *priv = GET_PRIVATE(state);
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
                    HIF_ERROR,
                    HIF_ERROR_INTERNAL_ERROR,
                    "percentage not 100: %i",
                    total);
        return FALSE;
    }

    /* set step number */
    if (!hif_state_set_number_steps_real(state, i+1, strloc)) {
        g_set_error(error,
                    HIF_ERROR,
                    HIF_ERROR_INTERNAL_ERROR,
                    "failed to set number steps: %i",
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
 * hif_state_show_profile:
 **/
static void
hif_state_show_profile(HifState *state)
{
    HifStatePrivate *priv = GET_PRIVATE(state);
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
 * hif_state_check:
 * @state: a #HifState instance.
 * @error: A #GError or %NULL
 *
 * Do any checks to see if the task has been cancelled.
 *
 * Returns: %TRUE for success, %FALSE otherwise
 *
 * Since: 0.1.0
 **/
gboolean
hif_state_check(HifState *state, GError **error)
{
    HifStatePrivate *priv = GET_PRIVATE(state);

    g_return_val_if_fail(state != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    /* are we cancelled */
    if (g_cancellable_is_cancelled(priv->cancellable)) {
        g_set_error_literal(error,
                            HIF_ERROR,
                            HIF_ERROR_CANCELLED,
                            "cancelled by user action");
        return FALSE;
    }
    return TRUE;
}

/**
 * hif_state_done_real:
 * @state: a #HifState instance.
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
hif_state_done_real(HifState *state, GError **error, const gchar *strloc)
{
    HifStatePrivate *priv = GET_PRIVATE(state);
    gdouble elapsed;
    gfloat percentage;

    g_return_val_if_fail(state != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    /* check */
    if (!hif_state_check(state, error))
        return FALSE;

    /* do we care */
    if (!priv->report_progress)
        return TRUE;

    /* did we call done on a state that did not have a size set? */
    if (priv->steps == 0) {
        g_set_error(error, HIF_ERROR, HIF_ERROR_INTERNAL_ERROR,
                    "done on a state %p that did not have a size set! [%s]",
                    state, strloc);
        hif_state_print_parent_chain(state, 0);
        return FALSE;
    }

    /* check the interval was too big in allow_cancel false mode */
    if (priv->enable_profile) {
        elapsed = g_timer_elapsed(priv->timer, NULL);
        if (!priv->allow_cancel_changed_state && priv->current > 0) {
            if (elapsed > 0.1f) {
                g_warning("%.1fms between hif_state_done() and no hif_state_set_allow_cancel()", elapsed * 1000);
                hif_state_print_parent_chain(state, 0);
            }
        }

        /* save the duration in the array */
        if (priv->step_profile != NULL)
            priv->step_profile[priv->current] = elapsed;
        g_timer_start(priv->timer);
    }

    /* is already at 100%? */
    if (priv->current >= priv->steps) {
        g_set_error(error, HIF_ERROR, HIF_ERROR_INTERNAL_ERROR,
                    "already at 100%% state [%s]", strloc);
        hif_state_print_parent_chain(state, 0);
        return FALSE;
    }

    /* is child not at 100%? */
    if (priv->child != NULL) {
        HifStatePrivate *child_priv = GET_PRIVATE(priv->child);
        if (child_priv->current != child_priv->steps) {
            g_print("child is at %i/%i steps and parent done [%s]\n",
                    child_priv->current, child_priv->steps, strloc);
            hif_state_print_parent_chain(priv->child, 0);
            /* do not abort, as we want to clean this up */
        }
    }

    /* we just checked for cancel, so it's not true to say we're blocking */
    hif_state_set_allow_cancel(state, TRUE);

    /* another */
    priv->current++;

    /* find new percentage */
    if (priv->step_data == NULL) {
        percentage = hif_state_discrete_to_percent(priv->current,
                                                   priv->steps);
    } else {
        /* this is cumalative, for speedy access */
        percentage = priv->step_data[priv->current - 1];
    }
    hif_state_set_percentage(state,(guint) percentage);

    /* show any profiling stats */
    if (priv->enable_profile &&
        priv->current == priv->steps &&
        priv->step_profile != NULL) {
        hif_state_show_profile(state);
    }

    /* reset child if it exists */
    if (priv->child != NULL)
        hif_state_reset(priv->child);
    return TRUE;
}

/**
 * hif_state_finished_real:
 * @state: A #HifState
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
hif_state_finished_real(HifState *state, GError **error, const gchar *strloc)
{
    HifStatePrivate *priv = GET_PRIVATE(state);

    g_return_val_if_fail(state != NULL, FALSE);
    g_return_val_if_fail(error == NULL || *error == NULL, FALSE);

    /* check */
    if (!hif_state_check(state, error))
        return FALSE;

    /* is already at 100%? */
    if (priv->current == priv->steps)
        return TRUE;

    /* all done */
    priv->current = priv->steps;

    /* set new percentage */
    hif_state_set_percentage(state, 100);
    return TRUE;
}

/**
 * hif_state_new:
 *
 * Creates a new #HifState.
 *
 * Returns:(transfer full): a #HifState
 *
 * Since: 0.1.0
 **/
HifState *
hif_state_new(void)
{
    HifState *state;
    state = g_object_new(HIF_TYPE_STATE, NULL);
    return HIF_STATE(state);
}
