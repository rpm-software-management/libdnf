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
#include "pycomp.hpp"

/**
 * unicode string in Python 2/3 to c string converter,
 * you need to call Py_XDECREF(tmp_py_str) after usage of returned string
 */
static char *
pycomp_get_string_from_unicode(PyObject *str_u, PyObject **tmp_py_str)
{
    *tmp_py_str = PyUnicode_AsEncodedString(str_u, "utf-8", "replace");
    if (*tmp_py_str) {
        return PyBytes_AsString(*tmp_py_str);
    } else {
        return NULL;
    }
}

PycompString::PycompString(PyObject * str)
: cString(nullptr), pyString(nullptr)
{
    if (PyUnicode_Check(str))
        cString = pycomp_get_string_from_unicode(str, &pyString);
#if PY_MAJOR_VERSION < 3
    else if (PyString_Check(str))
        cString = PyString_AsString(str);
    else
        PyErr_SetString(PyExc_TypeError, "Expected a string or a unicode object");
#else
    else if (PyBytes_Check(str))
        cString = PyBytes_AsString(str);
    else
        PyErr_SetString(PyExc_TypeError, "Expected a string or a unicode object");
#endif
}

PycompString::PycompString(PycompString && src) noexcept
: cString(src.cString), pyString(src.pyString)
{
    src.cString = nullptr;
    src.pyString = nullptr;
}

PycompString & PycompString::operator =(PycompString && src) noexcept
{
    if (this == &src)
        return *this;
    Py_XDECREF(pyString);
    cString = src.cString;
    pyString = src.pyString;
    src.cString = nullptr;
    src.pyString = nullptr;
    return *this;
}

PycompString::~PycompString()
{
    Py_XDECREF(pyString);
}

/**
 * bytes, basic string or unicode string in Python 2/3 to c string converter,
 * you need to call Py_XDECREF(tmp_py_str) after usage of returned string
 */
const char *
pycomp_get_string(PyObject *str, PyObject **tmp_py_str)
{
    char *res = NULL;
    if (PyUnicode_Check(str))
        res = pycomp_get_string_from_unicode(str, tmp_py_str);
#if PY_MAJOR_VERSION < 3
    else if (PyString_Check(str))
        res = PyString_AsString(str);
    else
        PyErr_SetString(PyExc_TypeError, "Expected a string or a unicode object");
#else
    else if (PyBytes_Check(str))
        res = PyBytes_AsString(str);
    else
        PyErr_SetString(PyExc_TypeError, "Expected a string or a unicode object");
#endif

    return res;
}

/* release PyObject array from memory */
void
pycomp_free_tmp_array(PyObject **tmp_py_strs, int count)
{
    for (int j = count; j >= 0; --j) {
        Py_XDECREF(tmp_py_strs[j]);
    }
}
