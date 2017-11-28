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
#include <solv/poolid.h>
#include <solv/solver.h>
#include <solv/util.h>
#include <time.h>

#include <pygobject-3.0/pygobject.h>

#include "hy-query.h"
#include "hy-selector.h"
#include "hy-subject.h"
#include "dnf-reldep.h"
#include "dnf-reldep-list.h"

#if WITH_SWDB
#include "dnf-swdb.h"
#endif

#include "exception-py.h"
#include "hawkey-pysys.h"
#include "iutil-py.h"
#include "package-py.h"
#include "query-py.h"
#include "reldep-py.h"
#include "sack-py.h"
#include "pycomp.h"

typedef struct {
    PyObject_HEAD
    HyQuery query;
    PyObject *sack;
} _QueryObject;

static const int keyname_int_matches[] = {
    HY_PKG,
    HY_PKG_ADVISORY,
    HY_PKG_ADVISORY_BUG,
    HY_PKG_ADVISORY_CVE,
    HY_PKG_ADVISORY_SEVERITY,
    HY_PKG_ADVISORY_TYPE,
    HY_PKG_ARCH,
    HY_PKG_CONFLICTS,
    HY_PKG_DESCRIPTION,
    HY_PKG_DOWNGRADABLE,
    HY_PKG_DOWNGRADES,
    HY_PKG_EMPTY,
    HY_PKG_ENHANCES,
    HY_PKG_EPOCH,
    HY_PKG_EVR,
    HY_PKG_FILE,
    HY_PKG_LATEST,
    HY_PKG_LATEST_PER_ARCH,
    HY_PKG_LOCATION,
    HY_PKG_NAME,
    HY_PKG_NEVRA,
    HY_PKG_OBSOLETES,
    HY_PKG_PROVIDES,
    HY_PKG_RECOMMENDS,
    HY_PKG_RELEASE,
    HY_PKG_REPONAME,
    HY_PKG_REQUIRES,
    HY_PKG_SOURCERPM,
    HY_PKG_SUGGESTS,
    HY_PKG_SUMMARY,
    HY_PKG_SUPPLEMENTS,
    HY_PKG_UPGRADABLE,
    HY_PKG_UPGRADES,
    HY_PKG_URL,
    HY_PKG_VERSION
};

static const char * const keyname_char_matches[] = {
    "pkg",
    "advisory",
    "advisory_bug",
    "advisory_cve",
    "advisory_severity",
    "advisory_type",
    "arch",
    "conflicts",
    "description",
    "downgradable",
    "downgrades",
    "empty",
    "enhances",
    "epoch",
    "evr",
    "file",
    "latest",
    "latest_per_arch",
    "location",
    "name",
    "nevra",
    "obsoletes",
    "provides",
    "recommends",
    "release",
    "reponame",
    "requires",
    "sourcerpm",
    "suggests",
    "summary",
    "supplements",
    "upgradable",
    "upgrades",
    "url",
    "version",
    NULL
};

static const char * const query_cmp_map_char[] = {
    "eq",
    "gt",
    "lt",
    "neq",
    "not",
    "gte",
    "lte",
    "substr",
    "glob",
    NULL
};

static const int query_cmp_map_int[] = {
    HY_EQ,
    HY_GT,
    HY_LT,
    HY_NEQ,
    HY_NOT,
    HY_EQ | HY_GT,
    HY_EQ | HY_LT,
    HY_SUBSTR,
    HY_GLOB
};

HyQuery
queryFromPyObject(PyObject *o)
{
    if (!PyType_IsSubtype(o->ob_type, &query_Type)) {
        PyErr_SetString(PyExc_TypeError, "Expected a Query object.");
        return NULL;
    }
    return ((_QueryObject *)o)->query;
}

PyObject *
queryToPyObject(HyQuery query, PyObject *sack, PyTypeObject *custom_object_type)
{
    _QueryObject *self = (_QueryObject *)custom_object_type->tp_alloc(custom_object_type, 0);
    if (self) {
        self->query = query;
        self->sack = sack;
        Py_INCREF(sack);
    }
    return (PyObject *) self;
}

int
query_converter(PyObject *o, HyQuery *query_ptr)
{
    HyQuery query = queryFromPyObject(o);
    if (query == NULL)
        return 0;
    *query_ptr = query;
    return 1;
}

/* functions on the type */

static PyObject *
query_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _QueryObject *self = (_QueryObject *)type->tp_alloc(type, 0);
    if (self) {
        self->query = NULL;
        self->sack = NULL;
    }
    return (PyObject *)self;
}

static void
query_dealloc(_QueryObject *self)
{
    if (self->query)
        hy_query_free(self->query);
    Py_XDECREF(self->sack);
    Py_TYPE(self)->tp_free(self);
}

static int
query_init(_QueryObject * self, PyObject *args, PyObject *kwds)
{
    const char *kwlist[] = {"sack", "query", NULL};
    PyObject *sack = NULL;
    PyObject *query = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OO", (char**) kwlist, &sack, &query))
        return -1;

    if (query && (!sack || sack == Py_None) && queryObject_Check(query)) {
        _QueryObject *query_obj = (_QueryObject*)query;
        self->sack = query_obj->sack;
        self->query = hy_query_clone(query_obj->query);
    } else if (sack && (!query || query == Py_None) && sackObject_Check(sack)) {
        DnfSack *csack = sackFromPyObject(sack);
        assert(csack);
        self->sack = sack;
        self->query = hy_query_create(csack);
    } else {
        const char *msg = "Expected a _hawkey.Sack or a _hawkey.Query object.";
        PyErr_SetString(PyExc_TypeError, msg);
        return -1;
    }
    Py_INCREF(self->sack);
    return 0;
}

/* object attributes */

static PyObject *
get_evaluated(_QueryObject *self, void *unused)
{
    HyQuery q = self->query;
    return PyBool_FromLong((long)hy_query_is_applied(q));
}

static PyObject *
clear(_QueryObject *self, PyObject *unused)
{
    hy_query_clear(self->query);
    Py_RETURN_NONE;
}

static int
raise_bad_filter(void)
{
    PyErr_SetString(HyExc_Query, "Invalid filter key or match type.");
    return 0;
}

static int
filter_add(HyQuery query, key_t keyname, int cmp_type, PyObject *match)
{
    const char *cmatch;

    if (keyname == HY_PKG_DOWNGRADABLE ||
        keyname == HY_PKG_DOWNGRADES ||
        keyname == HY_PKG_EMPTY ||
        keyname == HY_PKG_LATEST_PER_ARCH ||
        keyname == HY_PKG_LATEST ||
        keyname == HY_PKG_UPGRADABLE ||
        keyname == HY_PKG_UPGRADES) {
        long val;

        if (!PyInt_Check(match) || cmp_type != HY_EQ) {
            PyErr_SetString(HyExc_Value, "Invalid boolean filter query.");
            return 0;
        }
        val = PyLong_AsLong(match);
        if (keyname == HY_PKG_EMPTY) {
            if (!val) {
                PyErr_SetString(HyExc_Value, "Invalid boolean filter query.");
                return 0;
            }
            hy_query_filter_empty(query);
        } else if (keyname == HY_PKG_LATEST_PER_ARCH)
            hy_query_filter_latest_per_arch(query, val);
        else if (keyname == HY_PKG_LATEST)
            hy_query_filter_latest(query, val);
        else if (keyname == HY_PKG_DOWNGRADABLE)
            hy_query_filter_downgradable(query, val);
        else if (keyname == HY_PKG_DOWNGRADES)
            hy_query_filter_downgrades(query, val);
        else if (keyname == HY_PKG_UPGRADABLE)
            hy_query_filter_upgradable(query, val);
        else
            hy_query_filter_upgrades(query, val);
        return 1;
    }
    if (PyUnicode_Check(match) || PyString_Check(match)) {
        PyObject *tmp_py_str = NULL;
        cmatch = pycomp_get_string(match, &tmp_py_str);
        int query_filter_ret = hy_query_filter(query, keyname, cmp_type, cmatch);
        Py_XDECREF(tmp_py_str);

        if (query_filter_ret)
            return raise_bad_filter();
        return 1;
    }
    if (PyInt_Check(match)) {
        long val = PyLong_AsLong(match);
        if (cmp_type == HY_GLOB) // Workaround: Python can send integer with HY_GLOB
            cmp_type = HY_EQ;
        if (val > INT_MAX || val < INT_MIN) {
            PyErr_SetString(HyExc_Value, "Numeric argument out of range.");
            return 0;
        }
        if (hy_query_filter_num(query, keyname, cmp_type, val))
            return raise_bad_filter();
        return 1;
    }
    if (queryObject_Check(match)) {
        HyQuery target = queryFromPyObject(match);
        DnfPackageSet *pset = hy_query_run_set(target);
        int ret = hy_query_filter_package_in(query, keyname,
                                             cmp_type, pset);

        g_object_unref(pset);
        if (ret)
            return raise_bad_filter();
        return 1;
    }
    if (reldepObject_Check(match)) {
        DnfReldep *reldep = reldepFromPyObject(match);
        if (cmp_type != HY_EQ ||
            hy_query_filter_reldep(query, keyname, reldep))
            return raise_bad_filter();
        return 1;
    }
    // match is a sequence now:
    switch (keyname) {
    case HY_PKG:
    case HY_PKG_OBSOLETES: {
        DnfPackageSet *pset = pyseq_to_packageset(match, hy_query_get_sack(query));

        if (pset == NULL)
            return 1;
        int ret = hy_query_filter_package_in(query, keyname,
                                             cmp_type, pset);
        g_object_unref(pset);
        if (ret)
            return raise_bad_filter();

        break;
    }
    case HY_PKG_PROVIDES:
    case HY_PKG_REQUIRES:
    case HY_PKG_ENHANCES:
    case HY_PKG_RECOMMENDS:
    case HY_PKG_SUGGESTS:
    case HY_PKG_SUPPLEMENTS: {
        DnfReldepList *reldeplist = pyseq_to_reldeplist(match, hy_query_get_sack(query), cmp_type);
        if (reldeplist == NULL)
            return 1;

        int ret = hy_query_filter_reldep_in(query, keyname, reldeplist);
        g_object_unref (reldeplist);
        if (ret)
            return raise_bad_filter();
        break;
    }
    default: {
        PyObject *seq = PySequence_Fast(match, "Expected a sequence.");
        if (seq == NULL)
            return 1;
        const unsigned count = PySequence_Size(seq);
        const char *matches[count + 1];
        matches[count] = NULL;
        PyObject *tmp_py_strs[count];
        for (unsigned int i = 0; i < count; ++i) {
            PyObject *item = PySequence_Fast_GET_ITEM(seq, i);
            tmp_py_strs[i] = NULL;
            if (PyUnicode_Check(item) || PyString_Check(item)) {
                matches[i] = pycomp_get_string(item, &tmp_py_strs[i]);
            } else {
                PyErr_SetString(PyExc_TypeError, "Invalid filter match value.");
                pycomp_free_tmp_array(tmp_py_strs, i);
                Py_DECREF(seq);
                return 0;
            }
        }
        int filter_in_ret = hy_query_filter_in(query, keyname, cmp_type, matches);
        Py_DECREF(seq);
        pycomp_free_tmp_array(tmp_py_strs, count - 1);

        if (filter_in_ret)
            return raise_bad_filter();
        break;
    }
    }
    return 1;
}

static char *
filter_key_splitter(char** key)
{
    char *sbegin = *key;
	char *end;

    if (sbegin == NULL)
		return NULL;
    int index;

    for (index = 0; sbegin[index] != '\0'; ++index) {
        if ((sbegin[index] == '_') &&  (sbegin[index + 1] == '_')) {
            end = sbegin + index;
            *end++ = '\0';
            *key = ++end;
            return sbegin;
        }
    }
    *key = NULL;
    return sbegin;
}

gboolean
filter_internal(HyQuery query, HySelector sltr, PyObject *sack, PyObject *args, PyObject *kwds)
{
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    key_t keyname;
    int cmp_type;
    PyObject *tuple_item;
    const char *cmatch;
    int argument_number, presence_cmp_type;
    int cmp_type_flag = 0;

    if (args != NULL) {
        Py_ssize_t tuple_size = PyTuple_Size(args);
        for (int x = 0; x < tuple_size; ++x) {
            tuple_item = PyTuple_GetItem(args, x);
            if (PyInt_Check(tuple_item)) {
                long c_int = PyLong_AsLong(tuple_item);
                if (c_int == HY_ICASE) {
                    cmp_type_flag = HY_ICASE;
                } else {
                    PyErr_SetString(HyExc_Value, "Invalid flag. Only HY_ICASE allowed");
                    return FALSE;
                }
            }
        }
    }

    if (kwds != NULL) {
        while (PyDict_Next(kwds, &pos, &key, &value)) {
            keyname = -1;
            argument_number = 0;
            PyObject *tmp_py_str = NULL;
            cmatch = pycomp_get_string(key, &tmp_py_str);
            g_autofree char *parsed_string = strdup(cmatch);
            char *tmp_string = parsed_string;
            Py_XDECREF(tmp_py_str);
            cmp_type = 0;
            char *parcial_string;
            while ((parcial_string = filter_key_splitter(&tmp_string)) != NULL) {
                if (!argument_number) {
                    for (unsigned int i = 0; keyname_char_matches[i] != NULL; ++i) {
                        if (strcmp(keyname_char_matches[i], parcial_string) == 0) {
                            keyname = keyname_int_matches[i];
                            argument_number = 1;
                            break;
                        }
                    }
                    if (!argument_number) {
                        PyErr_SetString(HyExc_Value, g_strdup_printf(
                            "Unrecognized key name: %s", parcial_string));
                        return FALSE;
                    }
                } else {
                    presence_cmp_type = FALSE;
                    for (unsigned int i = 0; query_cmp_map_char[i] != NULL; ++i) {
                        if (strcmp(query_cmp_map_char[i], parcial_string) == 0) {
                            cmp_type |= query_cmp_map_int[i];
                            presence_cmp_type = TRUE;
                            break;
                        }
                    }
                    if (!presence_cmp_type) {
                        PyErr_SetString(HyExc_Value, g_strdup_printf(
                            "Unrecognized filter type: %s", parcial_string));
                        return FALSE;
                    }
                }
            }
            if (cmp_type == 0) {
                cmp_type = HY_EQ;
            }
            if (keyname != -1) {
                if (query != NULL) {
                    if (filter_add(query, keyname, cmp_type|cmp_type_flag, value) == 0) {
                        return FALSE;
                    }
                } else {
                    if (keyname == HY_PKG) {
                        const DnfPackageSet *pset;
                        if (queryObject_Check(value)) {
                            HyQuery target = queryFromPyObject(value);
                            pset = hy_query_run_set(target);
                        } else if (PyList_Check(value)) {
                            DnfSack *c_sack = sackFromPyObject(sack);
                            assert(c_sack);
                            pset = pyseq_to_packageset(value, c_sack);
                        }  else {
                            (ret2e(DNF_ERROR_BAD_SELECTOR, "Invalid value type: Only List and Query supported"));
                            return FALSE;
                        }

                        if (ret2e(hy_selector_pkg_set(sltr, keyname, cmp_type, pset),
                            "Invalid Selector spec." )) {
                            return FALSE;
                        }
                    } else {
                        const char *c_sltr_match;
                        PyObject *tmp_py_str_sltr = NULL;
                        c_sltr_match = pycomp_get_string(value, &tmp_py_str_sltr);
                        if (ret2e(hy_selector_set(sltr, keyname, cmp_type, c_sltr_match),
                            "Invalid Selector spec." )) {
                            Py_XDECREF(tmp_py_str_sltr);
                            return FALSE;
                        }

                    }
                }
            }
        }
    }
    return TRUE;
}

static PyObject *
filter(_QueryObject *self, PyObject *args, PyObject *kwds)
{
    HyQuery query = hy_query_clone(self->query);
    gboolean ret = filter_internal(query, NULL, self->sack, args, kwds);
    if (!ret)
        return NULL;
    PyObject *final_query = queryToPyObject(query, self->sack, Py_TYPE(self));
    Py_INCREF(final_query);
    return final_query;
}

static _QueryObject *
filterm(_QueryObject *self, PyObject *args, PyObject *kwds)
{
    gboolean ret = filter_internal(self->query, NULL, self->sack, args, kwds);
    if (!ret)
        return NULL;
    Py_INCREF(self);
    return self;
}

static PyObject *
add_available_filter(_QueryObject *self, PyObject *unused)
{
    HyQuery query = hy_query_clone(self->query);
    hy_query_filter(query, HY_PKG_REPONAME, HY_NEQ, HY_SYSTEM_REPO_NAME);
    PyObject *final_query = queryToPyObject(query, self->sack, Py_TYPE(self));
    Py_INCREF(final_query);
    return final_query;
}

static PyObject *
add_downgrades_filter(_QueryObject *self, PyObject *unused)
{
    HyQuery query = hy_query_clone(self->query);
    hy_query_filter_downgrades(query, 1);
    PyObject *final_query = queryToPyObject(query, self->sack, Py_TYPE(self));
    Py_INCREF(final_query);
    return final_query;
}

static PyObject *
duplicated_filter(_QueryObject *self, PyObject *unused)
{
    HyQuery self_query_copy = hy_query_clone(self->query);
    hy_filter_duplicated(self_query_copy);
    PyObject *final_query = queryToPyObject(self_query_copy, self->sack, Py_TYPE(self));
    Py_INCREF(final_query);
    return final_query;
}

static PyObject *
add_filter_extras(_QueryObject *self, PyObject *unused)
{
    HyQuery self_query_copy = hy_query_clone(self->query);
    hy_add_filter_extras(self_query_copy);
    PyObject *final_query = queryToPyObject(self_query_copy, self->sack, Py_TYPE(self));
    Py_INCREF(final_query);
    return final_query;
}

static PyObject *
add_installed_filter(_QueryObject *self, PyObject *unused)
{
    HyQuery query = hy_query_clone(self->query);
    hy_query_filter(query, HY_PKG_REPONAME, HY_EQ, HY_SYSTEM_REPO_NAME);
    PyObject *final_query = queryToPyObject(query, self->sack, Py_TYPE(self));
    Py_INCREF(final_query);
    return final_query;
}

static PyObject *
add_filter_latest(_QueryObject *self, PyObject *args)
{
    int value = 1;

    if (!PyArg_ParseTuple(args, "|i", &value))
        return NULL;

    HyQuery query = hy_query_clone(self->query);
    hy_query_filter_latest_per_arch(query, value);
    PyObject *final_query = queryToPyObject(query, self->sack, Py_TYPE(self));
    Py_INCREF(final_query);
    return final_query;
}

static PyObject *
add_upgrades_filter(_QueryObject *self, PyObject *unused)
{
    HyQuery query = hy_query_clone(self->query);
    hy_query_filter_upgrades(query, 1);
    PyObject *final_query = queryToPyObject(query, self->sack, Py_TYPE(self));
    Py_INCREF(final_query);
    return final_query;
}


static PyObject *
run(_QueryObject *self, PyObject *unused)
{
    DnfPackageSet *pset;
    PyObject *list;

    pset = hy_query_run_set(self->query);
    list = packageset_to_pylist(pset, self->sack);
    g_object_unref(pset);
    return list;
}

static PyObject *
apply(PyObject *self, PyObject *unused)
{
    hy_query_apply(((_QueryObject *) self)->query);
    Py_INCREF(self);
    return self;
}

static PyObject *
q_union(PyObject *self, PyObject *args)
{
    PyObject *other;
    if (!PyArg_ParseTuple(args, "O!", &query_Type, &other))
            return NULL;

    HyQuery self_query_copy = hy_query_clone(((_QueryObject *) self)->query);
    HyQuery other_q = ((_QueryObject *) other)->query;
    hy_query_union(self_query_copy, other_q);
    PyObject *final_query = queryToPyObject(self_query_copy, ((_QueryObject *) self)->sack,
                                            Py_TYPE(self));
    Py_INCREF(final_query);
    return final_query;
}

static PyObject *
q_intersection(PyObject *self, PyObject *args)
{
    PyObject *other;
    if (!PyArg_ParseTuple(args, "O!", &query_Type, &other))
            return NULL;

    HyQuery self_query_copy = hy_query_clone(((_QueryObject *) self)->query);
    HyQuery other_q = ((_QueryObject *) other)->query;
    hy_query_intersection(self_query_copy, other_q);
    PyObject *final_query = queryToPyObject(self_query_copy, ((_QueryObject *) self)->sack,
                                            Py_TYPE(self));
    Py_INCREF(final_query);
    return final_query;
}

static PyObject *
q_difference(PyObject *self, PyObject *args)
{
    PyObject *other;
    if (!PyArg_ParseTuple(args, "O!", &query_Type, &other))
            return NULL;

    HyQuery self_query_copy = hy_query_clone(((_QueryObject *) self)->query);
    HyQuery other_q = ((_QueryObject *) other)->query;
    hy_query_difference(self_query_copy, other_q);
    PyObject *final_query = queryToPyObject(self_query_copy, ((_QueryObject *) self)->sack,
                                            Py_TYPE(self));
    Py_INCREF(final_query);
    return final_query;
}

static PyObject *
filter_unneeded(PyObject *self, PyObject *args, PyObject *kwds)
{
#if WITH_SWDB
    HyQuery self_query_copy = hy_query_clone(((_QueryObject *) self)->query);
    const char *kwlist[] = {"swdb", "debug_solver", NULL};
    PyObject *swdb;
    PyObject *debug_solver = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|O!", (char**) kwlist, &swdb, &PyBool_Type,
        &debug_solver)) {
        return NULL;
    }
    DnfSwdb *c_swdb = (DnfSwdb *) ((PyGObject *) swdb)->obj;
    gboolean c_debug_solver = debug_solver != NULL && PyObject_IsTrue(debug_solver);

    int ret = hy_filter_unneeded(self_query_copy, c_swdb, c_debug_solver);
    if (ret == -1) {
        PyErr_SetString(PyExc_SystemError, "Unable to provide query with unneded filter");
        return NULL;
    }
    PyObject *final_query = queryToPyObject(self_query_copy, ((_QueryObject *) self)->sack,
                                            Py_TYPE(self));
    Py_INCREF(final_query);
    return final_query;
#else
    return NULL;
#endif
}

static PyObject *
q_add(_QueryObject *self, PyObject *list)
{
    if (!PyList_Check(list)) {
        PyErr_SetString(PyExc_TypeError, "Only a list can be concatenated to a Query");
        return NULL;
    }
    PyObject *unused = NULL;
    PyObject *query_list = run(self, unused);

    int list_count = PyList_Size(list);
    for (int index = 0; index < list_count; ++index)
        PyList_Append(query_list, PyList_GetItem(list, index));
    return query_list;
}

static PyObject *
q_contains(PyObject *self, PyObject *pypkg)
{
    HyQuery q = ((_QueryObject *) self)->query;
    DnfPackage *pkg = packageFromPyObject(pypkg);

    if (pkg) {
        Id id = dnf_package_get_id(pkg);
        hy_query_apply(q);
        if (MAPTST(hy_query_get_result(q), id))
            Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static int
query_len(PyObject *self)
{
    HyQuery q = ((_QueryObject *) self)->query;
    hy_query_apply(q);

    const unsigned char *res = hy_query_get_result(q)->map;
    const unsigned char *end = res + hy_query_get_result(q)->size;
    int length = 0;

    while (res < end)
        length += __builtin_popcount(*res++);

    return length;
}

static PyObject *
q_length(PyObject *self, PyObject *unused)
{
    return PyLong_FromLong(query_len(self));
}

static PyObject *
query_get_item(PyObject *self, int index)
{
    HyQuery query = ((_QueryObject *) self)->query;
    Id id = query_get_index_item(query, index);
    if (!id) {
        PyErr_SetString(PyExc_IndexError, "list index out of range");
        return NULL;
    }
    PyObject *package = new_package(((_QueryObject *) self)->sack, id);
    Py_INCREF(package);
    return package;
}

static PyObject *
query_get_item_by_pyindex(PyObject *self, PyObject *index)
{
    return query_get_item(self, (int) PyLong_AsLong(index));
}

static PyObject *
query_iter(PyObject *self)
{
    PyObject *list;
    DnfPackageSet *pset;

    pset = hy_query_run_set(((_QueryObject *) self)->query);
    list = packageset_to_pylist(pset, ((_QueryObject *) self)->sack);
    g_object_unref(pset);
    PyObject *iter = PyObject_GetIter(list);
    Py_DECREF(list);
    Py_INCREF(iter);
    return iter;
}

static PyObject *
query_to_name_dict(_QueryObject *self, PyObject *unused)
{
    HyQuery query = ((_QueryObject *) self)->query;
    Pool *pool = dnf_sack_get_pool(hy_query_get_sack(query));

    Queue samename;
    queue_init(&samename);

    hy_query_to_name_ordered_queue(query, &samename);

    Solvable *considered;
    Id name = 0;
    PyObject *list = PyList_New(0);
    PyObject *ret_dict = PyDict_New();

    for (int i = 0; i < samename.count; ++i) {
        Id package_id = samename.elements[i];
        considered = pool->solvables + package_id;
        if (name == 0) {
            name = considered->name;
        } else if (name != considered->name) {
            PyDict_SetItemString(ret_dict, pool_id2str(pool, name), list);
            Py_DECREF(list);
            list = PyList_New(0);
            name = considered->name;
        }
        PyObject *package = new_package(self->sack, package_id);
        if (package == NULL) {
            goto fail;
        }

        int rc = PyList_Append(list, package);
        Py_DECREF(package);
        if (rc == -1) {
            goto fail;
        }
    }
    queue_free(&samename);
    if (name) {
        PyDict_SetItemString(ret_dict, pool_id2str(pool, name), list);
        Py_DECREF(list);
    }
    return ret_dict;

    fail:
        queue_free(&samename);
        Py_DECREF(list);
        Py_DECREF(ret_dict);
        PyErr_SetString(PyExc_SystemError, "Unable to create name_dict");
        return NULL;
}

static PyObject *
query_to_name_arch_dict(_QueryObject *self, PyObject *unused)
{
    HyQuery query = ((_QueryObject *) self)->query;
    Pool *pool = dnf_sack_get_pool(hy_query_get_sack(query));

    Queue samename;
    queue_init(&samename);

    hy_query_to_name_arch_ordered_queue(query, &samename);

    Solvable *considered;
    Id name = 0;
    Id arch = 0;
    PyObject *key = PyTuple_New(2);
    PyObject *list = PyList_New(0);
    PyObject *ret_dict = PyDict_New();

    for (int i = 0; i < samename.count; ++i) {
        Id package_id = samename.elements[i];
        considered = pool->solvables + package_id;
        if (name == 0) {
            name = considered->name;
            arch = considered->arch;
        } else if ((name != considered->name) || (arch != considered->arch)) {
            if (PyTuple_SetItem(key, 0, PyString_FromString(pool_id2str(pool, name))))
                goto fail;
            if (PyTuple_SetItem(key, 1, PyString_FromString(pool_id2str(pool, arch))))
                goto fail;
            PyDict_SetItem(ret_dict, key, list);
            Py_DECREF(key);
            Py_DECREF(list);
            key = PyTuple_New(2);
            list = PyList_New(0);
            name = considered->name;
            arch = considered->arch;
        }
        PyObject *package = new_package(self->sack, package_id);
        if (package == NULL) {
            goto fail;
        }

        int rc = PyList_Append(list, package);
        Py_DECREF(package);
        if (rc == -1) {
            goto fail;
        }
    }
    queue_free(&samename);
    if (name) {
        if (PyTuple_SetItem(key, 0, PyString_FromString(pool_id2str(pool, name))))
            goto fail;
        if (PyTuple_SetItem(key, 1, PyString_FromString(pool_id2str(pool, arch))))
            goto fail;
        PyDict_SetItem(ret_dict, key, list);
    }
    Py_DECREF(key);
    Py_DECREF(list);

    return ret_dict;

    fail:
        queue_free(&samename);
        Py_DECREF(key);
        Py_DECREF(list);
        Py_DECREF(ret_dict);
        PyErr_SetString(PyExc_SystemError, "Unable to create name_arch_dict");
        return NULL;
}

static PyObject *
add_nevra_or_other_filter(_QueryObject *self, PyObject *args)
{
    HyQuery self_query_copy = hy_query_clone(((_QueryObject *) self)->query);

    int arguments_count = PyTuple_Size(args);
    if (arguments_count == 1) {
        const char *name;
        if (!PyArg_ParseTuple(args, "s", &name))
            return NULL;

        HyNevra out_nevra = hy_nevra_create();
        if (hy_nevra_possibility((char *) name, HY_FORM_NEVRA, out_nevra) == 0) {
            hy_add_filter_nevra_object(self_query_copy, out_nevra, FALSE);
        } else {
            hy_query_filter_empty(self_query_copy);
        }
    } else if (arguments_count == 3) {
        const char *name;
        const char *evr;
        const char *arch;

        if (!PyArg_ParseTuple(args, "sss", &name, &evr, &arch))
            return NULL;
        hy_query_filter(self_query_copy, HY_PKG_NAME, HY_EQ, name);
        hy_query_filter(self_query_copy, HY_PKG_EVR, HY_EQ, evr);
        hy_query_filter(self_query_copy, HY_PKG_ARCH, HY_EQ, arch);
    } else {
        PyErr_SetString(PyExc_TypeError,
                        "nevra() takes 1 (NEVRA), or 3 (name, evr, arch) str params");
        return NULL;
    }
    PyObject *final_query = queryToPyObject(self_query_copy, self->sack, Py_TYPE(self));
    Py_INCREF(final_query);
    return final_query;
}

static PyObject *
add_filter_recent(_QueryObject *self, PyObject *args)
{
    long recent;
    if (!PyArg_ParseTuple(args, "l", &recent))
        return NULL;

    hy_query_apply(self->query);
    HyQuery self_query_copy = hy_query_clone(self->query);
    time_t now = time(NULL);
    time_t recent_limit = now - (recent*86400);
    hy_filter_recent(self_query_copy, (recent_limit < 0) ? 0 : recent_limit);
    PyObject *final_query = queryToPyObject(self_query_copy, self->sack, Py_TYPE(self));
    Py_INCREF(final_query);
    return final_query;
}

static PyGetSetDef query_getsetters[] = {
    {(char*)"evaluated",  (getter)get_evaluated, NULL, NULL, NULL},
    {NULL}                        /* sentinel */
};

PySequenceMethods query_sequence = {
    (lenfunc)query_len,               /* sq_length */
    (binaryfunc)q_add,                /* sq_concat */
    0,                                /* sq_repeat */
    (ssizeargfunc) query_get_item,    /* sq_item */
};

static struct PyMethodDef query_methods[] = {
    {"clear", (PyCFunction)clear, METH_NOARGS,
     NULL},
    {"filter", (PyCFunction)filter, METH_KEYWORDS|METH_VARARGS,
     NULL},
    {"filterm", (PyCFunction)filterm, METH_KEYWORDS|METH_VARARGS,
     NULL},
    {"run", (PyCFunction)run, METH_NOARGS,
     NULL},
    {"apply", (PyCFunction)apply, METH_NOARGS,
     NULL},
    {"available", (PyCFunction)add_available_filter, METH_NOARGS, NULL},
    {"downgrades", (PyCFunction)add_downgrades_filter, METH_NOARGS, NULL},
    {"duplicated", (PyCFunction)duplicated_filter, METH_NOARGS, NULL},
    {"extras", (PyCFunction)add_filter_extras, METH_NOARGS, NULL},
    {"installed", (PyCFunction)add_installed_filter, METH_NOARGS, NULL},
    {"latest", (PyCFunction)add_filter_latest, METH_VARARGS, NULL},
    {"union", (PyCFunction)q_union, METH_VARARGS, NULL},
    {"upgrades", (PyCFunction)add_upgrades_filter, METH_NOARGS, NULL},
    {"intersection", (PyCFunction)q_intersection, METH_VARARGS, NULL},
    {"difference", (PyCFunction)q_difference, METH_VARARGS, NULL},
    {"count", (PyCFunction)q_length, METH_NOARGS,
        NULL},
    {"_na_dict", (PyCFunction)query_to_name_arch_dict, METH_NOARGS, NULL},
    {"_name_dict", (PyCFunction)query_to_name_dict, METH_NOARGS, NULL},
    {"_nevra", (PyCFunction)add_nevra_or_other_filter, METH_VARARGS, NULL},
    {"_recent", (PyCFunction)add_filter_recent, METH_VARARGS, NULL},
    {"_unneeded", (PyCFunction)filter_unneeded, METH_KEYWORDS|METH_VARARGS,
     NULL},
    {"__add__", (PyCFunction)q_add, METH_O, NULL},
    {"__contains__", (PyCFunction)q_contains, METH_O,
     NULL},
    {"__getitem__", (PyCFunction)query_get_item_by_pyindex, METH_O,
     NULL},
    {"__iter__", (PyCFunction)query_iter, METH_NOARGS, NULL},
    {"__len__", (PyCFunction)q_length, METH_NOARGS,
     NULL},
    {NULL}                      /* sentinel */
};

PyTypeObject query_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_hawkey.Query",                /*tp_name*/
    sizeof(_QueryObject),        /*tp_basicsize*/
    0,                                /*tp_itemsize*/
    (destructor) query_dealloc, /*tp_dealloc*/
    0,                                /*tp_print*/
    0,                                /*tp_getattr*/
    0,                                /*tp_setattr*/
    0,                                /*tp_compare*/
    0,                                /*tp_repr*/
    0,                                /*tp_as_number*/
    &query_sequence,                  /*tp_as_sequence*/
    0,                                /*tp_as_mapping*/
    0,                                /*tp_hash */
    0,                                /*tp_call*/
    0,                                /*tp_str*/
    0,                                /*tp_getattro*/
    0,                                /*tp_setattro*/
    0,                                /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "Query object",                /* tp_doc */
    0,                                /* tp_traverse */
    0,                                /* tp_clear */
    0,                                /* tp_richcompare */
    0,                                /* tp_weaklistoffset */
    query_iter,                       /* tp_iter */
    0,                                /* tp_iternext */
    query_methods,                /* tp_methods */
    0,                                /* tp_members */
    query_getsetters,                /* tp_getset */
    0,                                /* tp_base */
    0,                                /* tp_dict */
    0,                                /* tp_descr_get */
    0,                                /* tp_descr_set */
    0,                                /* tp_dictoffset */
    (initproc)query_init,        /* tp_init */
    0,                                /* tp_alloc */
    query_new,                        /* tp_new */
    0,                                /* tp_free */
    0,                                /* tp_is_gc */
};
