/*
 * Copyright (C) 2012-2013 Red Hat, Inc.
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

#include <Python.h>
#include <glib.h>
#include <structmember.h>
#include <stddef.h>
#include <solv/util.h>

#include "dnf-types.h"
#include "dnf-goal.h"

#include "exception-py.hpp"
#include "goal-py.hpp"
#include "iutil-py.hpp"
#include "package-py.hpp"
#include "selector-py.hpp"
#include "sack-py.hpp"
#include "solution-py.hpp"
#include "pycomp.hpp"

#define BLOCK_SIZE 15

typedef struct {
    PyObject_HEAD
    HyGoal goal;
    PyObject *sack;
} _GoalObject;

static int
args_pkg_sltr_check(DnfPackage *pkg, HySelector sltr)
{
    if (!(pkg || sltr)) {
        PyErr_SetString(PyExc_ValueError,
                        "Requires a Package or a Selector argument.");
        return 0;
    }
    if (pkg && sltr) {
        PyErr_SetString(PyExc_ValueError,
                        "Does not accept both Package and Selector arguments.");
        return 0;
    }
    return 1;
}

static int
args_pkg_sltr_parse(PyObject *args, PyObject *kwds,
                     DnfPackage **pkg, HySelector *sltr, int *flags, int flag_mask)
{
    const char *kwlist[] = {"package", "select", "clean_deps", "check_installed",
                      "optional", NULL};
    int clean_deps = 0, check_installed = 0, optional = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O&O&iii", (char**) kwlist,
                                     package_converter, pkg,
                                     selector_converter, sltr,
                                     &clean_deps, &check_installed,
                                     &optional))
        return 0;
    if (!args_pkg_sltr_check(*pkg, *sltr))
        return 0;
    if (clean_deps) {
        if (!(flag_mask & HY_CLEAN_DEPS)) {
            PyErr_SetString(PyExc_ValueError,
                            "Does not accept clean_deps keyword") ;
            return 0;
        }
        *flags |= HY_CLEAN_DEPS;
    }
    if (check_installed) {
        if  (!(flag_mask & HY_CHECK_INSTALLED)) {
            PyErr_SetString(PyExc_ValueError,
                            "Does not accept check_installed keyword") ;
            return 0;
        }
        *flags |= HY_CHECK_INSTALLED;
    }
    if (optional) {
        if  (!(flag_mask & HY_WEAK_SOLV)) {
            PyErr_SetString(PyExc_ValueError,
                            "Does not accept optional keyword");
            return 0;
        }
        *flags |= HY_WEAK_SOLV;
    }
    return 1;
}

static int
args_run_parse(PyObject *args, PyObject *kwds, int *flags, PyObject **callback_p)
{
    const char *kwlist[] = {"callback", "allow_uninstall", "force_best", "verify",
        "ignore_weak_deps", "ignore_weak", NULL};
    int ignore_weak = 0;
    int ignore_weak_deps = 0;
    int allow_uninstall = 0;
    int force_best = 0;
    int verify = 0;
    PyObject *callback = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Oiiiii", (char**) kwlist,
                                     &callback, &allow_uninstall, &force_best,
                                     &verify, &ignore_weak_deps, &ignore_weak))
        return 0;

    if (callback) {
        if (!callback_p) {
            PyErr_SetString(PyExc_ValueError,
                            "Does not accept a callback argument.");
            return 0;
        }
        if (!PyCallable_Check(callback)) {
            PyErr_SetString(PyExc_ValueError, "Must be a callable object.");
            return 0;
        }
        *callback_p = callback;
    } else if (callback_p) {
        PyErr_SetString(PyExc_ValueError, "Expected a callback argument.");
        return 0;
    }

    if (allow_uninstall)
        *flags |= DNF_ALLOW_UNINSTALL;
    if (force_best)
        *flags |= DNF_FORCE_BEST;
    if (verify)
        *flags |= DNF_VERIFY;
    if (ignore_weak_deps)
        *flags |= DNF_IGNORE_WEAK_DEPS;
    if (ignore_weak)
        *flags |= DNF_IGNORE_WEAK;
    return 1;
}

static PyObject *
op_ret2exc(int ret)
{
    if (!ret)
        Py_RETURN_NONE;

    switch (ret) {
    case DNF_ERROR_BAD_SELECTOR:
        PyErr_SetString(HyExc_Value,
                        "Ill-formed Selector used for the operation.");
        return NULL;
    case DNF_ERROR_INVALID_ARCHITECTURE:
        PyErr_SetString(HyExc_Arch, "Used arch is unknown.");
        return NULL;
    case DNF_ERROR_PACKAGE_NOT_FOUND:
        PyErr_SetString(HyExc_Validation, "The validation check has failed.");
        return NULL;
    default:
        PyErr_SetString(HyExc_Exception, "Goal operation failed.");
        return NULL;
    }
}

/* functions on the type */

static PyObject *
goal_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _GoalObject *self = (_GoalObject*)type->tp_alloc(type, 0);
    if (self) {
        self->goal = NULL;
        self->sack = NULL;
    }
    return (PyObject*)self;
}

static void
goal_dealloc(_GoalObject *self)
{
    if (self->goal)
        hy_goal_free(self->goal);

    Py_XDECREF(self->sack);
    Py_TYPE(self)->tp_free(self);
}

static int
goal_init(_GoalObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *sack;
    DnfSack *csack;

    if (!PyArg_ParseTuple(args, "O!", &sack_Type, &sack))
        return -1;
    csack = sackFromPyObject(sack);
    if (csack == NULL)
        return -1;
    self->sack = sack;
    Py_INCREF(self->sack); // sack has to kept around until we are
    self->goal = hy_goal_create(csack);
    return 0;
}

/* object methods */

static PyObject *
distupgrade_all(_GoalObject *self, PyObject *unused)
{
    int ret = hy_goal_distupgrade_all(self->goal);
    return op_ret2exc(ret);
}

static PyObject *
distupgrade(_GoalObject *self, PyObject *args, PyObject *kwds)
{
    DnfPackage *pkg = NULL;
    HySelector sltr = NULL;
    if (!args_pkg_sltr_parse(args, kwds, &pkg, &sltr, NULL, 0))
        return NULL;

    int ret = pkg ? hy_goal_distupgrade(self->goal, pkg) :
        hy_goal_distupgrade_selector(self->goal, sltr);
    return op_ret2exc(ret);
}

static PyObject *
downgrade_to(_GoalObject *self, PyObject *args, PyObject *kwds)
{
    DnfPackage *pkg = NULL;
    HySelector sltr = NULL;
    int flags = 0;
    int ret;

    if (!args_pkg_sltr_parse(args, kwds, &pkg, &sltr,
                              &flags, HY_CHECK_INSTALLED|HY_WEAK_SOLV))
        return NULL;

    if (pkg) {
        if (hy_goal_downgrade_to(self->goal, pkg))
            Py_RETURN_FALSE;
        Py_RETURN_TRUE;
    } else {
        if (flags & HY_WEAK_SOLV) {
            ret = hy_goal_downgrade_to_selector_optional(self->goal, sltr);
        } else {
            ret = hy_goal_downgrade_to_selector(self->goal, sltr);
        }
        return op_ret2exc(ret);
    }
}

static PyObject *
erase(_GoalObject *self, PyObject *args, PyObject *kwds)
{
    DnfPackage *pkg = NULL;
    HySelector sltr = NULL;
    int flags = 0;
    if (!args_pkg_sltr_parse(args, kwds, &pkg, &sltr, &flags, HY_CLEAN_DEPS))
        return NULL;

    int ret = pkg ? hy_goal_erase_flags(self->goal, pkg, flags) :
        hy_goal_erase_selector_flags(self->goal, sltr, flags);
    return op_ret2exc(ret);
}

static PyObject *
install(_GoalObject *self, PyObject *args, PyObject *kwds)
{
    DnfPackage *pkg = NULL;
    HySelector sltr = NULL;
    int flags = 0;
    g_autoptr(GError) error = NULL;
    if (!args_pkg_sltr_parse(args, kwds, &pkg, &sltr, &flags, HY_WEAK_SOLV))
        return NULL;

    if (flags & HY_WEAK_SOLV) {
        if (pkg) {
            hy_goal_install_optional(self->goal, pkg);
        } else {
            hy_goal_install_selector_optional(self->goal, sltr, &error);
        }
    } else {
        if (pkg) {
            hy_goal_install(self->goal, pkg);
        } else {
            hy_goal_install_selector(self->goal, sltr, &error);
        }
    }
    return op_error2exc(error);
}

static PyObject *
upgrade(_GoalObject *self, PyObject *args, PyObject *kwds)
{
    DnfPackage *pkg = NULL;
    HySelector sltr = NULL;

    if (!args_pkg_sltr_parse(args, kwds, &pkg, &sltr, NULL, 0))
        return NULL;
    if (pkg) {
        PyErr_SetString(PyExc_NotImplementedError,
                        "Selecting a package to be upgraded is not implemented.");
        return NULL;
    }
    int ret = hy_goal_upgrade_selector(self->goal, sltr);
    return op_ret2exc(ret);
}

static PyObject *
upgrade_to(_GoalObject *self, PyObject *args, PyObject *kwds)
{
    DnfPackage *pkg = NULL;
    HySelector sltr = NULL;
    int ret;
    int flags = 0;

    if (!args_pkg_sltr_parse(args, kwds, &pkg, &sltr,
                              &flags, HY_CHECK_INSTALLED))
        return NULL;
    if (sltr) {
        ret = hy_goal_upgrade_to_selector(self->goal, sltr);
        return op_ret2exc(ret);
    }
    ret = hy_goal_upgrade_to_flags(self->goal, pkg, flags);
    return op_ret2exc(ret);
}

static PyObject *
upgrade_all(_GoalObject *self, PyObject *unused)
{
    int ret = hy_goal_upgrade_all(self->goal);
    return op_ret2exc(ret);
}

static PyObject *
userinstalled(_GoalObject *self, PyObject *pkg)
{
    DnfPackage *cpkg = packageFromPyObject(pkg);
    int ret;

    if (cpkg == NULL)
        return NULL;
    ret = hy_goal_userinstalled(self->goal, cpkg);
    if (!ret)
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject *
has_actions(_GoalObject *self, PyObject *action)
{
    return PyBool_FromLong(hy_goal_has_actions(
        self->goal, static_cast<DnfGoalActions>(PyLong_AsLong(action))));
}

static PyObject *
req_has_distupgrade_all(_GoalObject *self, PyObject *unused)
{
    return PyBool_FromLong(hy_goal_has_actions(self->goal, DNF_DISTUPGRADE_ALL));
}

static PyObject *
req_has_erase(_GoalObject *self, PyObject *unused)
{
    return PyBool_FromLong(hy_goal_has_actions(self->goal, DNF_ERASE));
}

static PyObject *
req_has_upgrade_all(_GoalObject *self, PyObject *unused)
{
    return PyBool_FromLong(hy_goal_has_actions(self->goal, DNF_UPGRADE_ALL));
}

static PyObject *
req_length(_GoalObject *self, PyObject *unused)
{
    return PyLong_FromLong(hy_goal_req_length(self->goal));
}

static PyObject *
add_protected(_GoalObject *self, PyObject *seq)
{
    HyGoal goal = self->goal;
    DnfPackageSet *pset = pyseq_to_packageset(seq, hy_goal_get_sack(goal));
    if (pset == NULL)
        return NULL;
    dnf_goal_add_protected(goal, pset);
    g_object_unref(pset);
    Py_RETURN_NONE;
}

static PyObject *
run(_GoalObject *self, PyObject *args, PyObject *kwds)
{
    int flags = 0;
    if (!args_run_parse(args, kwds, &flags, NULL))
        return NULL;

    int ret = hy_goal_run_flags(self->goal, static_cast<DnfGoalActions>(flags));
    if (!ret)
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

struct _PySolutionCallback {
    PyObject *callback_tuple;
    PyObject *callback;
    int errors;
};

static int
py_solver_callback(HyGoal goal, void *data)
{
    struct _PySolutionCallback *cb_s = (struct _PySolutionCallback*)data;

    PyObject *ret = PyObject_CallObject(cb_s->callback, cb_s->callback_tuple);
    if (ret)
        Py_DECREF(ret);
    else
        cb_s->errors++;

    return 0; /* solution_callback() result is ignored in libsolv */
}

static PyObject *
run_all(_GoalObject *self, PyObject *args, PyObject *kwds)
{
    int flags = 0;
    PyObject *callback = NULL;
    if (!args_run_parse(args, kwds, &flags, &callback))
        return NULL;

    PyObject *callback_tuple = Py_BuildValue("(O)", self);
    if (!callback_tuple)
        return NULL;

    struct _PySolutionCallback cb_s = {callback_tuple, callback, 0};
    int ret = hy_goal_run_all_flags(self->goal, py_solver_callback, &cb_s,
                                    static_cast<DnfGoalActions>(flags));
    Py_DECREF(callback_tuple);
    if (cb_s.errors > 0)
        return NULL;
    if (!ret)
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject *
count_problems(_GoalObject *self, PyObject *unused)
{
    return PyLong_FromLong(hy_goal_count_problems(self->goal));
}

static PyObject *
describe_problem(_GoalObject *self, PyObject *index_obj)
{
    char *cstr;
    PyObject *str;

    if (!PyInt_Check(index_obj)) {
        PyErr_SetString(PyExc_TypeError, "An integer value expected.");
        return NULL;
    }
    cstr = hy_goal_describe_problem(self->goal, PyLong_AsLong(index_obj));
    if (cstr == NULL) {
        PyErr_SetString(PyExc_ValueError, "Index out of range.");
        return NULL;
    }
    str = PyString_FromString(cstr);
    g_free(cstr);
    return str;
}

/**
 * Reports problems described in strings.
 *
 * Returns Python list with Python list of strings for each problem or empty Python list if no
 * problem or NULL in case of error.
 */
static PyObject *
problem_rules(_GoalObject *self, PyObject *unused)
{
    PyObject *list_output = PyList_New(0);
    PyObject *list;
    if (list_output == NULL)
        return NULL;
    int count_problems = hy_goal_count_problems(self->goal);
    for (int i = 0; i < count_problems; i++) {
        g_auto(GStrv) plist = hy_goal_describe_problem_rules(self->goal, i);
        if (plist == NULL) {
            PyErr_SetString(PyExc_ValueError, "Index out of range.");
            continue;
        }
        list = strlist_to_pylist((const char **)plist);
        int rc = PyList_Append(list_output, list);
        Py_DECREF(list);
        if (rc == -1)
            return NULL;
    }
    return list_output;
}

/**
 * Reports packages that has a conflict
 *
 * Returns Python list with package objects that have a conflict.
 */
static PyObject *
problem_conflicts(_GoalObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *list;
    DnfPackageSet *pset;
    const char *kwlist[] = {"available", NULL};
    int available = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|i", (char**) kwlist, &available))
        return 0;

    DnfPackageState pkg_type = DNF_PACKAGE_STATE_ALL;
    if (available)
        pkg_type = DNF_PACKAGE_STATE_AVAILABLE;

    pset = hy_goal_conflict_all_pkgs(self->goal, pkg_type);
    list = packageset_to_pylist(pset, self->sack);
    return list;
}

/**
 * Reports packages that has a conflict
 *
 * Returns Python list with package objects that have a conflict.
 */
static PyObject *
problem_broken_dependency(_GoalObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *list;
    DnfPackageSet *pset;
    const char *kwlist[] = {"available", NULL};
    int available = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|i", (char**) kwlist, &available))
        return 0;

    DnfPackageState pkg_type = DNF_PACKAGE_STATE_ALL;

    if (available)
        pkg_type = DNF_PACKAGE_STATE_AVAILABLE;

    pset = hy_goal_broken_dependency_all_pkgs(self->goal, pkg_type);
    list = packageset_to_pylist(pset, self->sack);
    return list;
}

static PyObject *
log_decisions(_GoalObject *self, PyObject *unused)
{
    if (hy_goal_log_decisions(self->goal))
        PyErr_SetString(PyExc_ValueError, "log_decisions() failed.");
    Py_RETURN_NONE;
}

static PyObject *
write_debugdata(_GoalObject *self, PyObject *dir_str)
{
    g_autoptr(GError) error = NULL;
    PyObject *tmp_py_str = NULL;
    const char *dir = pycomp_get_string(dir_str, &tmp_py_str);

    if (dir == NULL) {
        Py_XDECREF(tmp_py_str);
        return NULL;
    }

    gboolean ret = hy_goal_write_debugdata(self->goal, dir, &error);
    Py_XDECREF(tmp_py_str);
    if (!ret) {
        op_error2exc(error);
        return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
list_generic(_GoalObject *self, GPtrArray *(*func)(HyGoal, GError **))
{
    g_autoptr(GError) error = NULL;
    GPtrArray *plist = func(self->goal, &error);
    PyObject *list;

    if (!plist) {
        switch (error->code) {
        case DNF_ERROR_INTERNAL_ERROR:
            PyErr_SetString(HyExc_Value, "Goal has not been run yet.");
            break;
        case DNF_ERROR_NO_SOLUTION:
            PyErr_SetString(HyExc_Runtime, "Goal could not find a solution.");
            break;
        default:
            assert(0);
        }
        return NULL;
    }
    list = packagelist_to_pylist(plist, self->sack);
    g_ptr_array_unref(plist);
    return list;
}

static PyObject *
list_erasures(_GoalObject *self, PyObject *unused)
{
    return list_generic(self, hy_goal_list_erasures);
}

static PyObject *
list_installs(_GoalObject *self, PyObject *unused)
{
    return list_generic(self, hy_goal_list_installs);
}

static PyObject *
list_obsoleted(_GoalObject *self, PyObject *unused)
{
    return list_generic(self, hy_goal_list_obsoleted);
}

static PyObject *
list_reinstalls(_GoalObject *self, PyObject *unused)
{
    return list_generic(self, hy_goal_list_reinstalls);
}

static PyObject *
list_unneeded(_GoalObject *self, PyObject *unused)
{
    return list_generic(self, hy_goal_list_unneeded);
}

static PyObject *
list_downgrades(_GoalObject *self, PyObject *unused)
{
    return list_generic(self, hy_goal_list_downgrades);
}

static PyObject *
list_upgrades(_GoalObject *self, PyObject *unused)
{
    return list_generic(self, hy_goal_list_upgrades);
}

static PyObject *
obsoleted_by_package(_GoalObject *self, PyObject *pkg)
{
    DnfPackage *cpkg = packageFromPyObject(pkg);

    if (cpkg == NULL)
        return NULL;
    GPtrArray *plist = hy_goal_list_obsoleted_by_package(self->goal, cpkg);
    PyObject *list = packagelist_to_pylist(plist, self->sack);
    g_ptr_array_unref(plist);
    return list;
}

static PyObject *
get_reason(_GoalObject *self, PyObject *pkg)
{
    DnfPackage *cpkg = packageFromPyObject(pkg);

    if (cpkg == NULL)
        return NULL;
    int reason = hy_goal_get_reason(self->goal, cpkg);
    return PyLong_FromLong(reason);
}

static PyObject *
get_solution(_GoalObject *self, PyObject *index_obj)
{
    PyObject *list;

    if (!PyInt_Check(index_obj)) {
        PyErr_SetString(PyExc_TypeError, "An integer value expected.");
        return NULL;
    }
    g_autoptr(GPtrArray) slist = hy_goal_get_solution(self->goal,
                                                      PyLong_AsLong(index_obj));
    if (slist == NULL) {
        PyErr_SetString(PyExc_ValueError, "Index out of range.");
        return NULL;
    }
    // make PYLIST from GPtrArray
    list = solutionlist_to_pylist(slist);
    return list;
}

static PyObject *
goalToPyObject(HyGoal goal, PyObject *sack)
{
    _GoalObject *self = (_GoalObject *)goal_Type.tp_alloc(&goal_Type, 0);
    if (self) {
        self->goal = goal;
        self->sack = sack;
        Py_INCREF(sack);
    }
    return (PyObject *)self;
}

static PyObject *
deepcopy(_GoalObject *self, PyObject *args, PyObject *kwds)
{
    HyGoal goal = hy_goal_clone(self->goal);
    return goalToPyObject(goal, self->sack);
}

static struct PyMethodDef goal_methods[] = {
    {"__deepcopy__", (PyCFunction)deepcopy, METH_KEYWORDS|METH_VARARGS,
     NULL},
    {"add_protected", (PyCFunction)add_protected, METH_O,
     NULL},
    {"distupgrade_all",        (PyCFunction)distupgrade_all,        METH_NOARGS,        NULL},
    {"distupgrade",                (PyCFunction)distupgrade,
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"downgrade_to",        (PyCFunction)downgrade_to,
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"erase",                (PyCFunction)erase,
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"install",                (PyCFunction)install,
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"upgrade",                (PyCFunction)upgrade,
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"upgrade_to",        (PyCFunction)upgrade_to,
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"upgrade_all",        (PyCFunction)upgrade_all,        METH_NOARGS,        NULL},
    {"userinstalled",        (PyCFunction)userinstalled,        METH_O,                NULL},
    {"_has_actions",        (PyCFunction)has_actions,        METH_O, NULL},
    // deprecated in 0.5.9, will be removed in 1.0.0
    // use goal.actions | hawkey.DISTUPGRADE_ALL instead
    {"req_has_distupgrade_all",        (PyCFunction)req_has_distupgrade_all,
     METH_NOARGS,        NULL},
    // deprecated in 0.5.9, will be removed in 1.0.0
    // use goal.actions | hawkey.ERASE instead
    {"req_has_erase",        (PyCFunction)req_has_erase,        METH_NOARGS,        NULL},
    // deprecated in 0.5.9, will be removed in 1.0.0
    // use goal.actions | hawkey.UPGRADE_ALL instead
    {"req_has_upgrade_all", (PyCFunction)req_has_upgrade_all,
     METH_NOARGS,        NULL},
    {"req_length",        (PyCFunction)req_length,        METH_NOARGS,        NULL},
    {"run",                (PyCFunction)run,
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"run_all",        (PyCFunction)run_all,
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"count_problems",        (PyCFunction)count_problems,        METH_NOARGS,        NULL},
    {"problem_conflicts",(PyCFunction)problem_conflicts,        METH_VARARGS | METH_KEYWORDS,                NULL},
    {"problem_broken_dependency",(PyCFunction)problem_broken_dependency,        METH_VARARGS | METH_KEYWORDS,                NULL},
    {"describe_problem",(PyCFunction)describe_problem,        METH_O,                NULL},
    {"problem_rules", (PyCFunction)problem_rules,        METH_NOARGS,                NULL},
    {"log_decisions",   (PyCFunction)log_decisions,        METH_NOARGS,        NULL},
    {"write_debugdata", (PyCFunction)write_debugdata,        METH_O,                NULL},
    {"list_erasures",        (PyCFunction)list_erasures,        METH_NOARGS,        NULL},
    {"list_installs",        (PyCFunction)list_installs,        METH_NOARGS,        NULL},
    {"list_obsoleted",        (PyCFunction)list_obsoleted,        METH_NOARGS,        NULL},
    {"list_reinstalls",        (PyCFunction)list_reinstalls,        METH_NOARGS,        NULL},
    {"list_unneeded",        (PyCFunction)list_unneeded,        METH_NOARGS,        NULL},
    {"list_downgrades",        (PyCFunction)list_downgrades,        METH_NOARGS,        NULL},
    {"list_upgrades",        (PyCFunction)list_upgrades,        METH_NOARGS,        NULL},
    {"obsoleted_by_package",(PyCFunction)obsoleted_by_package,
     METH_O, NULL},
    {"get_reason",        (PyCFunction)get_reason,        METH_O,                NULL},
    {"get_solution",      (PyCFunction)get_solution,      METH_O,                NULL},
    {NULL}                      /* sentinel */
};

static struct PyMemberDef goal_members[] = {
    {(char*)"sack", T_OBJECT, offsetof(_GoalObject, sack), READONLY, NULL},
    {NULL}
};

PyTypeObject goal_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_hawkey.Goal",                /*tp_name*/
    sizeof(_GoalObject),        /*tp_basicsize*/
    0,                                /*tp_itemsize*/
    (destructor) goal_dealloc,  /*tp_dealloc*/
    0,                                /*tp_print*/
    0,                                /*tp_getattr*/
    0,                                /*tp_setattr*/
    0,                                /*tp_compare*/
    0,                                /*tp_repr*/
    0,                                /*tp_as_number*/
    0,                                /*tp_as_sequence*/
    0,                                /*tp_as_mapping*/
    0,                                /*tp_hash */
    0,                                /*tp_call*/
    0,                                /*tp_str*/
    PyObject_GenericGetAttr,        /*tp_getattro*/
    0,                                /*tp_setattro*/
    0,                                /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "Goal object",                /* tp_doc */
    0,                                /* tp_traverse */
    0,                                /* tp_clear */
    0,                                /* tp_richcompare */
    0,                                /* tp_weaklistoffset */
    PyObject_SelfIter,                /* tp_iter */
    0,                                 /* tp_iternext */
    goal_methods,                /* tp_methods */
    goal_members,                /* tp_members */
    0,                                /* tp_getset */
    0,                                /* tp_base */
    0,                                /* tp_dict */
    0,                                /* tp_descr_get */
    0,                                /* tp_descr_set */
    0,                                /* tp_dictoffset */
    (initproc)goal_init,        /* tp_init */
    0,                                /* tp_alloc */
    goal_new,                        /* tp_new */
    0,                                /* tp_free */
    0,                                /* tp_is_gc */
};
