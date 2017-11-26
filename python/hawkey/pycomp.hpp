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

/* module for Python 2/3 C API compatibility */

#ifndef PYCOMP_H
#define PYCOMP_H

// Python 3 and newer types compatibility
#if PY_MAJOR_VERSION >= 3
    #define PyInt_Check PyLong_Check
    #define PyString_FromString PyUnicode_FromString
    #define PyString_FromFormat PyUnicode_FromFormat
    #define PyString_Check PyBytes_Check
    #define Py_TPFLAGS_HAVE_ITER 0
#endif

// uniform way to define Python 2 and Python 3 modules
#if PY_MAJOR_VERSION >= 3
    #define PYCOMP_MOD_ERROR_VAL NULL
    #define PYCOMP_MOD_SUCCESS_VAL(val) val
    #define PYCOMP_MOD_INIT(name) PyMODINIT_FUNC PyInit_##name(void)
    #define PYCOMP_MOD_DEF(ob, name, methods) \
          static struct PyModuleDef moduledef = { \
            PyModuleDef_HEAD_INIT, name, NULL, -1, methods, }; \
          ob = PyModule_Create(&moduledef);
#else
    #define PYCOMP_MOD_ERROR_VAL
    #define PYCOMP_MOD_SUCCESS_VAL(val)
    #define PYCOMP_MOD_INIT(name) extern "C" void init##name(void)
    #define PYCOMP_MOD_DEF(ob, name, methods) \
          ob = Py_InitModule(name, methods);
#endif


// Python 2.5 and older compatibility
#ifndef PyVarObject_HEAD_INIT
    #define PyVarObject_HEAD_INIT(type, size) \
        PyObject_HEAD_INIT(type) size,
#endif

const char *pycomp_get_string(PyObject *str_o, PyObject **tmp_py_str);
void pycomp_free_tmp_array(PyObject **tmp_py_strs, int count);
PYCOMP_MOD_INIT(_hawkey);

#endif // PYCOMP_H
