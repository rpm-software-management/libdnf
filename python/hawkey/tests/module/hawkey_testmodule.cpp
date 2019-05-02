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

// hawkey
#include "dnf-sack-private.hpp"

#include "python/hawkey/repo-py.hpp"
#include "python/hawkey/sack-py.hpp"
#include "tests/hawkey/testshared.h"

#include "python/hawkey/pycomp.hpp"


PYCOMP_MOD_INIT(_hawkey_test);

static PyObject *
py_load_repo(PyObject *unused, PyObject *args)
{
    PyObject *sack = NULL;
    char *name = NULL, *path = NULL;
    int installed;

    if (!PyArg_ParseTuple(args, "Ossi", &sack, &name, &path, &installed))
	return NULL;

    DnfSack *csack = sackFromPyObject(sack);
    if (csack == NULL) {
	PyErr_SetString(PyExc_TypeError, "Expected a DnfSack *object.");
	return NULL;
    }
    if (load_repo(dnf_sack_get_pool(csack), name, path, installed)) {
	PyErr_SetString(PyExc_IOError, "Can not load a testing repo.");
	return NULL;
    }
    Py_RETURN_NONE;
}

static PyObject *
py_glob_for_repofiles(PyObject *unused, PyObject *args)
{
    const char *repo_name, *path;
    DnfSack *sack;

    if (!PyArg_ParseTuple(args, "O&ss",
			  sack_converter, &sack, &repo_name, &path))
	return NULL;
    HyRepo repo = glob_for_repofiles(dnf_sack_get_pool(sack), repo_name, path);
    return repoToPyObject(repo);
}

static struct PyMethodDef testmodule_methods[] = {
    {"load_repo",		(PyCFunction)py_load_repo,
     METH_VARARGS, NULL},
    {"glob_for_repofiles",	(PyCFunction)py_glob_for_repofiles,
     METH_VARARGS, NULL},
    {NULL}				/* sentinel */
};

PYCOMP_MOD_INIT(_hawkey_test)
{
    PyObject *m;
    PYCOMP_MOD_DEF(m, "_hawkey_test", testmodule_methods);
    if (!m)
	return PYCOMP_MOD_ERROR_VAL;
    PyModule_AddIntConstant(m, "EXPECT_SYSTEM_NSOLVABLES",
			    TEST_EXPECT_SYSTEM_NSOLVABLES);
    PyModule_AddIntConstant(m, "EXPECT_MAIN_NSOLVABLES",
			    TEST_EXPECT_MAIN_NSOLVABLES);
    PyModule_AddIntConstant(m, "EXPECT_UPDATES_NSOLVABLES",
			    TEST_EXPECT_UPDATES_NSOLVABLES);
    PyModule_AddIntConstant(m, "EXPECT_YUM_NSOLVABLES",
			    TEST_EXPECT_YUM_NSOLVABLES);
    PyModule_AddStringConstant(m, "FIXED_ARCH", TEST_FIXED_ARCH);
    PyModule_AddStringConstant(m, "UNITTEST_DIR", UNITTEST_DIR);
    PyModule_AddStringConstant(m, "YUM_DIR_SUFFIX", YUM_DIR_SUFFIX);

    return PYCOMP_MOD_SUCCESS_VAL(m);
}
