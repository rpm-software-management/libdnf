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
#include "dnf-types.h"

// pyhawkey
#include "exception-py.hpp"

PyObject *HyExc_Exception = NULL;
PyObject *HyExc_Value = NULL;
PyObject *HyExc_Query = NULL;
PyObject *HyExc_Arch = NULL;
PyObject *HyExc_Runtime = NULL;
PyObject *HyExc_Validation = NULL;

int
init_exceptions(void)
{
     HyExc_Exception = PyErr_NewException((char*) "_hawkey.Exception", NULL, NULL);
     if (!HyExc_Exception)
         return 0;
     Py_INCREF(HyExc_Exception);

     HyExc_Value = PyErr_NewException((char*) "_hawkey.ValueException", HyExc_Exception,
                                      NULL);
     if (!HyExc_Value)
         return 0;
     Py_INCREF(HyExc_Value);

     HyExc_Query = PyErr_NewException((char*) "_hawkey.QueryException", HyExc_Value,
                                      NULL);
     if (!HyExc_Query)
         return 0;
     Py_INCREF(HyExc_Query);

     HyExc_Arch = PyErr_NewException((char*) "_hawkey.ArchException", HyExc_Value,
                                      NULL);
     if (!HyExc_Arch)
         return 0;
     Py_INCREF(HyExc_Arch);

     HyExc_Runtime = PyErr_NewException((char*) "_hawkey.RuntimeException",
                                        HyExc_Exception, NULL);
     if (!HyExc_Runtime)
         return 0;
     Py_INCREF(HyExc_Runtime);

     HyExc_Validation = PyErr_NewException((char*) "_hawkey.ValidationException",
                                        HyExc_Exception, NULL);
     if (!HyExc_Validation)
         return 0;
     Py_INCREF(HyExc_Validation);

     return 1;
}

int
ret2e(int ret, const char *msg)
{
    PyObject *exctype = NULL;
    switch (ret) {
    case 0:
        return 0;
    case DNF_ERROR_FAILED:
        exctype = HyExc_Runtime;
        break;
    case DNF_ERROR_FILE_INVALID: {
        exctype = PyExc_IOError;
        break;
    }
    case DNF_ERROR_INTERNAL_ERROR:
    case DNF_ERROR_BAD_SELECTOR:
        exctype = HyExc_Value;
        break;
    default:
        // try to end it quickly
        assert(0);
        // or fallback to an internal-error exception
        PyErr_SetString(PyExc_AssertionError, msg);
        return 1;
    }

    assert(exctype);
    PyErr_SetString(exctype, msg);
    return 1;
}


PyObject *
op_error2exc(const GError *error)
{
    if (error == NULL)
        Py_RETURN_NONE;

    switch (error->code) {
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
    case DNF_ERROR_FILE_INVALID:
        PyErr_SetString(PyExc_IOError, error->message);
        return NULL;
    case DNF_ERROR_CANNOT_WRITE_CACHE:
        PyErr_SetString(PyExc_IOError, "Failed writing the cache.");
        return NULL;
    default:
        PyErr_SetString(HyExc_Exception, error->message);
        return NULL;
    }
}
