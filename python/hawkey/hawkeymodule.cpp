/*
 * Copyright (C) 2012-2014 Red Hat, Inc.
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

#include <libintl.h>

// hawkey
#include "dnf-advisory.h"
#include "dnf-advisorypkg.h"
#include "dnf-advisoryref.h"
#include "hy-goal.h"
#include "hy-package.h"
#include "hy-query.h"
#include "hy-subject.h"
#include "dnf-solution.h"
#include "hy-types.h"
#include "hy-util.h"
#include "dnf-version.h"

// pyhawkey
#include "advisory-py.hpp"
#include "advisorypkg-py.hpp"
#include "advisoryref-py.hpp"
#include "exception-py.hpp"
#include "goal-py.hpp"
#include "nevra-py.hpp"
#include "module-form-py.hpp"
#include "package-py.hpp"
#include "packagedelta-py.hpp"
#include "possibilities-py.hpp"
#include "query-py.hpp"
#include "reldep-py.hpp"
#include "repo-py.hpp"
#include "sack-py.hpp"
#include "selector-py.hpp"
#include "solution-py.hpp"
#include "subject-py.hpp"

#include "pycomp.hpp"

static PyObject *
detect_arch(PyObject *unused, PyObject *args)
{
    char *arch;

    if (ret2e(hy_detect_arch(&arch), "Failed detecting architecture."))
        return NULL;
    return PyString_FromString(arch);
}

static PyObject *
chksum_name(PyObject *unused, PyObject *args)
{
    int i;
    const char *name;

    if (!PyArg_ParseTuple(args, "i", &i))
        return NULL;
    name = hy_chksum_name(i);
    if (name == NULL) {
        PyErr_Format(PyExc_ValueError, "unrecognized chksum type: %d", i);
        return NULL;
    }

    return PyString_FromString(name);
}

static PyObject *
chksum_type(PyObject *unused, PyObject *str_o)
{
    PyObject *tmp_py_str = NULL;
    const char *str = pycomp_get_string(str_o, &tmp_py_str);

    if (str == NULL) {
        Py_XDECREF(tmp_py_str);
        return NULL;
    }
    int type = hy_chksum_type(str);
    Py_XDECREF(tmp_py_str);

    if (type == 0) {
        PyErr_Format(PyExc_ValueError, "unrecognized chksum type: %s", str);
        return NULL;
    }
    return PyLong_FromLong(type);
}

static PyObject *
split_nevra(PyObject *unused, PyObject *nevra_o)
{
    PyObject *tmp_py_str = NULL;
    const char *nevra = pycomp_get_string(nevra_o, &tmp_py_str);

    if (nevra == NULL) {
        Py_XDECREF(tmp_py_str);
        return NULL;
    }
    int epoch;
    char *name, *version, *release, *arch;

    int split_nevra_ret = hy_split_nevra(nevra, &name, &epoch, &version, &release, &arch);
    Py_XDECREF(tmp_py_str); // release memory after unicode string conversion

    if (ret2e(split_nevra_ret, "Failed parsing NEVRA."))
        return NULL;

    PyObject *ret = Py_BuildValue("slsss", name, epoch, version, release, arch);
    if (ret == NULL)
        return NULL;
    return ret;
}

static struct PyMethodDef hawkey_methods[] = {
    {"chksum_name",                (PyCFunction)chksum_name,
     METH_VARARGS,        NULL},
    {"chksum_type",                (PyCFunction)chksum_type,
     METH_O,                NULL},
    {"detect_arch",                (PyCFunction)detect_arch,
     METH_NOARGS,        NULL},
    {"split_nevra",                (PyCFunction)split_nevra,
     METH_O,                NULL},
    {NULL}                                /* sentinel */
};

PYCOMP_MOD_INIT(_hawkey)
{
    PyObject *m;
    PYCOMP_MOD_DEF(m, "_hawkey", hawkey_methods)

    if (!m)
        return PYCOMP_MOD_ERROR_VAL;
    /* exceptions */
    if (!init_exceptions())
        return PYCOMP_MOD_ERROR_VAL;
    PyModule_AddObject(m, "Exception", HyExc_Exception);
    PyModule_AddObject(m, "ValueException", HyExc_Value);
    PyModule_AddObject(m, "QueryException", HyExc_Query);
    PyModule_AddObject(m, "ArchException", HyExc_Arch);
    PyModule_AddObject(m, "RuntimeException", HyExc_Runtime);
    PyModule_AddObject(m, "ValidationException", HyExc_Validation);

    /* _hawkey.Sack */
    if (PyType_Ready(&sack_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&sack_Type);
    PyModule_AddObject(m, "Sack", (PyObject *)&sack_Type);
    /* _hawkey.Advisory */
    if (PyType_Ready(&advisory_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&advisory_Type);
    PyModule_AddObject(m, "Advisory", (PyObject *)&advisory_Type);
    /* _hawkey.AdvisoryPkg */
    if (PyType_Ready(&advisorypkg_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&advisorypkg_Type);
    PyModule_AddObject(m, "AdvisoryPkg", (PyObject *)&advisorypkg_Type);
    /* _hawkey.AdvisoryRef */
    if (PyType_Ready(&advisoryref_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&advisoryref_Type);
    PyModule_AddObject(m, "AdvisoryRef", (PyObject *)&advisoryref_Type);
    /* _hawkey.Goal */
    if (PyType_Ready(&goal_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&goal_Type);
    PyModule_AddObject(m, "Goal", (PyObject *)&goal_Type);
    /* _hawkey.Package */
    if (PyType_Ready(&package_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&package_Type);
    PyModule_AddObject(m, "Package", (PyObject *)&package_Type);
    /* _hawkey.PackageDelta */
    if (PyType_Ready(&packageDelta_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&packageDelta_Type);
    PyModule_AddObject(m, "PackageDelta", (PyObject *)&packageDelta_Type);
    /* _hawkey.Query */
    if (PyType_Ready(&query_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&query_Type);
    PyModule_AddObject(m, "Query", (PyObject *)&query_Type);
    /* _hawkey.Reldep */
    if (PyType_Ready(&reldep_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&reldep_Type);
    PyModule_AddObject(m, "Reldep", (PyObject *)&reldep_Type);
    /* _hawkey.Selector */
    if (PyType_Ready(&selector_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&selector_Type);
    PyModule_AddObject(m, "Selector", (PyObject *)&selector_Type);
    /* _hawkey.Repo */
    if (PyType_Ready(&repo_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&repo_Type);
    PyModule_AddObject(m, "Repo", (PyObject *)&repo_Type);
    /* _hawkey.NEVRA */
    if (PyType_Ready(&nevra_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&nevra_Type);
    PyModule_AddObject(m, "NEVRA", (PyObject *)&nevra_Type);
    /* _hawkey.ModuleForm */
    if (PyType_Ready(&module_form_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&module_form_Type);
    PyModule_AddObject(m, "ModuleForm", (PyObject *)&module_form_Type);
    /* _hawkey.Solution */
    if (PyType_Ready(&solution_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&solution_Type);
    PyModule_AddObject(m, "Solution", (PyObject *)&solution_Type);
    /* _hawkey.Subject */
    if (PyType_Ready(&subject_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&subject_Type);
    PyModule_AddObject(m, "Subject", (PyObject *)&subject_Type);
    /* _hawkey._Possibilities */
    possibilities_Type.tp_new = PyType_GenericNew;
    if (PyType_Ready(&possibilities_Type) < 0)
        return PYCOMP_MOD_ERROR_VAL;
    Py_INCREF(&possibilities_Type);

    PyModule_AddIntConstant(m, "FORM_NEVRA", HY_FORM_NEVRA);
    PyModule_AddIntConstant(m, "FORM_NEVR", HY_FORM_NEVR);
    PyModule_AddIntConstant(m, "FORM_NEV", HY_FORM_NEV);
    PyModule_AddIntConstant(m, "FORM_NA", HY_FORM_NA);
    PyModule_AddIntConstant(m, "FORM_NAME", HY_FORM_NAME);

    PyModule_AddIntConstant(m, "MODULE_FORM_NSVCAP", HY_MODULE_FORM_NSVCAP);
    PyModule_AddIntConstant(m, "MODULE_FORM_NSVCA", HY_MODULE_FORM_NSVCA);
    PyModule_AddIntConstant(m, "MODULE_FORM_NSVAP", HY_MODULE_FORM_NSVAP);
    PyModule_AddIntConstant(m, "MODULE_FORM_NSVA", HY_MODULE_FORM_NSVA);
    PyModule_AddIntConstant(m, "MODULE_FORM_NSAP", HY_MODULE_FORM_NSAP);
    PyModule_AddIntConstant(m, "MODULE_FORM_NSA", HY_MODULE_FORM_NSA);
    PyModule_AddIntConstant(m, "MODULE_FORM_NSVCP", HY_MODULE_FORM_NSVCP);
    PyModule_AddIntConstant(m, "MODULE_FORM_NSVP", HY_MODULE_FORM_NSVP);
    PyModule_AddIntConstant(m, "MODULE_FORM_NSVC", HY_MODULE_FORM_NSVC);
    PyModule_AddIntConstant(m, "MODULE_FORM_NSV", HY_MODULE_FORM_NSV);
    PyModule_AddIntConstant(m, "MODULE_FORM_NSP", HY_MODULE_FORM_NSP);
    PyModule_AddIntConstant(m, "MODULE_FORM_NS", HY_MODULE_FORM_NS);
    PyModule_AddIntConstant(m, "MODULE_FORM_NAP", HY_MODULE_FORM_NAP);
    PyModule_AddIntConstant(m, "MODULE_FORM_NA", HY_MODULE_FORM_NA);
    PyModule_AddIntConstant(m, "MODULE_FORM_NP", HY_MODULE_FORM_NP);
    PyModule_AddIntConstant(m, "MODULE_FORM_N", HY_MODULE_FORM_N);

    PyModule_AddIntConstant(m, "VERSION_MAJOR", LIBDNF_MAJOR_VERSION);
    PyModule_AddIntConstant(m, "VERSION_MINOR", LIBDNF_MINOR_VERSION);
    PyModule_AddIntConstant(m, "VERSION_PATCH", LIBDNF_MICRO_VERSION);

    PyModule_AddStringConstant(m, "SYSTEM_REPO_NAME", HY_SYSTEM_REPO_NAME);
    PyModule_AddStringConstant(m, "CMDLINE_REPO_NAME", HY_CMDLINE_REPO_NAME);

    PyModule_AddIntConstant(m, "PKG", HY_PKG);
    PyModule_AddIntConstant(m, "PKG_ADVISORY", HY_PKG_ADVISORY);
    PyModule_AddIntConstant(m, "PKG_ADVISORY_BUG", HY_PKG_ADVISORY_BUG);
    PyModule_AddIntConstant(m, "PKG_ADVISORY_CVE", HY_PKG_ADVISORY_CVE);
    PyModule_AddIntConstant(m, "PKG_ADVISORY_SEVERITY", HY_PKG_ADVISORY_SEVERITY);
    PyModule_AddIntConstant(m, "PKG_ADVISORY_TYPE", HY_PKG_ADVISORY_TYPE);
    PyModule_AddIntConstant(m, "PKG_ARCH", HY_PKG_ARCH);
    PyModule_AddIntConstant(m, "PKG_CONFLICTS", HY_PKG_CONFLICTS);
    PyModule_AddIntConstant(m, "PKG_DESCRIPTION", HY_PKG_DESCRIPTION);
    PyModule_AddIntConstant(m, "PKG_DOWNGRADABLE", HY_PKG_DOWNGRADABLE);
    PyModule_AddIntConstant(m, "PKG_DOWNGRADES", HY_PKG_DOWNGRADES);
    PyModule_AddIntConstant(m, "PKG_EMPTY", HY_PKG_EMPTY);
    PyModule_AddIntConstant(m, "PKG_ENHANCES", HY_PKG_ENHANCES);
    PyModule_AddIntConstant(m, "PKG_EPOCH", HY_PKG_EPOCH);
    PyModule_AddIntConstant(m, "PKG_EVR", HY_PKG_EVR);
    PyModule_AddIntConstant(m, "PKG_FILE", HY_PKG_FILE);
    PyModule_AddIntConstant(m, "PKG_LATEST_PER_ARCH", HY_PKG_LATEST_PER_ARCH);
    PyModule_AddIntConstant(m, "PKG_LATEST", HY_PKG_LATEST);
    PyModule_AddIntConstant(m, "PKG_LOCATION", HY_PKG_LOCATION);
    PyModule_AddIntConstant(m, "PKG_NAME", HY_PKG_NAME);
    PyModule_AddIntConstant(m, "PKG_NEVRA", HY_PKG_NEVRA);
    PyModule_AddIntConstant(m, "PKG_OBSOLETES", HY_PKG_OBSOLETES);
    PyModule_AddIntConstant(m, "PKG_PROVIDES", HY_PKG_PROVIDES);
    PyModule_AddIntConstant(m, "PKG_RECOMMENDS", HY_PKG_RECOMMENDS);
    PyModule_AddIntConstant(m, "PKG_RELEASE", HY_PKG_RELEASE);
    PyModule_AddIntConstant(m, "PKG_REPONAME", HY_PKG_REPONAME);
    PyModule_AddIntConstant(m, "PKG_REQUIRES", HY_PKG_REQUIRES);
    PyModule_AddIntConstant(m, "PKG_SOURCERPM", HY_PKG_SOURCERPM);
    PyModule_AddIntConstant(m, "PKG_SUGGESTS", HY_PKG_SUGGESTS);
    PyModule_AddIntConstant(m, "PKG_SUMMARY", HY_PKG_SUMMARY);
    PyModule_AddIntConstant(m, "PKG_SUPPLEMENTS", HY_PKG_SUPPLEMENTS);
    PyModule_AddIntConstant(m, "PKG_UPGRADABLE", HY_PKG_UPGRADABLE);
    PyModule_AddIntConstant(m, "PKG_UPGRADES", HY_PKG_UPGRADES);
    PyModule_AddIntConstant(m, "PKG_URL", HY_PKG_URL);
    PyModule_AddIntConstant(m, "PKG_VERSION", HY_PKG_VERSION);

    PyModule_AddIntConstant(m, "ERASE", DNF_ERASE);
    PyModule_AddIntConstant(m, "DISTUPGRADE", DNF_DISTUPGRADE);
    PyModule_AddIntConstant(m, "DISTUPGRADE_ALL", DNF_DISTUPGRADE_ALL);
    PyModule_AddIntConstant(m, "DOWNGRADE", DNF_DOWNGRADE);
    PyModule_AddIntConstant(m, "INSTALL", DNF_INSTALL);
    PyModule_AddIntConstant(m, "UPGRADE", DNF_UPGRADE);
    PyModule_AddIntConstant(m, "UPGRADE_ALL", DNF_UPGRADE_ALL);

    PyModule_AddIntConstant(m, "ALLOW_UNINSTALL", DNF_ALLOW_UNINSTALL);
    PyModule_AddIntConstant(m, "FORCE_BEST", DNF_FORCE_BEST);
    PyModule_AddIntConstant(m, "VERIFY", DNF_VERIFY);
    PyModule_AddIntConstant(m, "IGNORE_WEAK_DEPS", DNF_IGNORE_WEAK_DEPS);

    PyModule_AddIntConstant(m, "SOLUTION_ALLOW_INSTALL", DNF_SOLUTION_ACTION_ALLOW_INSTALL);
    PyModule_AddIntConstant(m, "SOLUTION_ALLOW_REINSTALL", DNF_SOLUTION_ACTION_ALLOW_REINSTALL);
    PyModule_AddIntConstant(m, "SOLUTION_ALLOW_UPGRADE", DNF_SOLUTION_ACTION_ALLOW_UPGRADE);
    PyModule_AddIntConstant(m, "SOLUTION_ALLOW_DOWNGRADE", DNF_SOLUTION_ACTION_ALLOW_DOWNGRADE);
    PyModule_AddIntConstant(m, "SOLUTION_ALLOW_CHANGE", DNF_SOLUTION_ACTION_ALLOW_CHANGE);
    PyModule_AddIntConstant(m, "SOLUTION_ALLOW_OBSOLETE", DNF_SOLUTION_ACTION_ALLOW_OBSOLETE);
    PyModule_AddIntConstant(m, "SOLUTION_ALLOW_REPLACEMENT", DNF_SOLUTION_ACTION_ALLOW_REPLACEMENT);
    PyModule_AddIntConstant(m, "SOLUTION_ALLOW_REMOVE", DNF_SOLUTION_ACTION_ALLOW_REMOVE);
    PyModule_AddIntConstant(m, "SOLUTION_DO_NOT_INSTALL", DNF_SOLUTION_ACTION_DO_NOT_INSTALL);
    PyModule_AddIntConstant(m, "SOLUTION_DO_NOT_REMOVE", DNF_SOLUTION_ACTION_DO_NOT_REMOVE);
    PyModule_AddIntConstant(m, "SOLUTION_DO_NOT_OBSOLETE", DNF_SOLUTION_ACTION_DO_NOT_OBSOLETE);
    PyModule_AddIntConstant(m, "SOLUTION_DO_NOT_UPGRADE", DNF_SOLUTION_ACTION_DO_NOT_UPGRADE);
    PyModule_AddIntConstant(m, "SOLUTION_BAD_SOLUTION", DNF_SOLUTION_ACTION_BAD_SOLUTION);

    PyModule_AddIntConstant(m, "CHKSUM_MD5", G_CHECKSUM_MD5);
    PyModule_AddIntConstant(m, "CHKSUM_SHA1", G_CHECKSUM_SHA1);
    PyModule_AddIntConstant(m, "CHKSUM_SHA256", G_CHECKSUM_SHA256);
    PyModule_AddIntConstant(m, "CHKSUM_SHA512", G_CHECKSUM_SHA512);

    PyModule_AddIntConstant(m, "ICASE", HY_ICASE);
    PyModule_AddIntConstant(m, "EQ", HY_EQ);
    PyModule_AddIntConstant(m, "LT", HY_LT);
    PyModule_AddIntConstant(m, "GT", HY_GT);
    PyModule_AddIntConstant(m, "NEQ", HY_NEQ);
    PyModule_AddIntConstant(m, "NOT", HY_NOT);
    PyModule_AddIntConstant(m, "SUBSTR", HY_SUBSTR);
    PyModule_AddIntConstant(m, "GLOB", HY_GLOB);

    PyModule_AddIntConstant(m, "REASON_DEP", HY_REASON_DEP);
    PyModule_AddIntConstant(m, "REASON_USER", HY_REASON_USER);
    PyModule_AddIntConstant(m, "REASON_CLEAN", HY_REASON_CLEAN);
    PyModule_AddIntConstant(m, "REASON_WEAKDEP", HY_REASON_WEAKDEP);

    PyModule_AddIntConstant(m, "ADVISORY_UNKNOWN", DNF_ADVISORY_KIND_UNKNOWN);
    PyModule_AddIntConstant(m, "ADVISORY_SECURITY", DNF_ADVISORY_KIND_SECURITY);
    PyModule_AddIntConstant(m, "ADVISORY_BUGFIX", DNF_ADVISORY_KIND_BUGFIX);
    PyModule_AddIntConstant(m, "ADVISORY_ENHANCEMENT", DNF_ADVISORY_KIND_ENHANCEMENT);
    PyModule_AddIntConstant(m, "ADVISORY_NEWPACKAGE", DNF_ADVISORY_KIND_NEWPACKAGE);

    PyModule_AddIntConstant(m, "REFERENCE_UNKNOWN", DNF_REFERENCE_KIND_UNKNOWN);
    PyModule_AddIntConstant(m, "REFERENCE_BUGZILLA", DNF_REFERENCE_KIND_BUGZILLA);
    PyModule_AddIntConstant(m, "REFERENCE_CVE", DNF_REFERENCE_KIND_CVE);
    PyModule_AddIntConstant(m, "REFERENCE_VENDOR", DNF_REFERENCE_KIND_VENDOR);

    bind_textdomain_codeset(GETTEXT_DOMAIN, "UTF-8");

    return PYCOMP_MOD_SUCCESS_VAL(m);
}
