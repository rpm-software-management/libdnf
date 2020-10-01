/*
 * Copyright (C) 2012-2019 Red Hat, Inc.
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

#include "Python.h"


// hawkey
#include "libdnf/repo/Repo.hpp"
#include "dnf-types.h"
#include "hy-packageset.h"
#include "hy-repo.h"
#include "hy-util.h"
#include "dnf-version.h"
#include "dnf-sack-private.hpp"
#include "libdnf/module/ModulePackageContainer.hpp"

// pyhawkey
#include "exception-py.hpp"
#include "hawkey-pysys.hpp"
#include "iutil-py.hpp"
#include "package-py.hpp"
#include "repo-py.hpp"
#include "sack-py.hpp"

#include "pycomp.hpp"
#include "sack/packageset.hpp"

#include <algorithm>
#include <functional>

typedef struct {
    PyObject_HEAD
    DnfSack *sack;
    PyObject *custom_package_class;
    PyObject *custom_package_val;
    PyObject * ModulePackageContainerPy;

    // g_log handler IDs
    // Multiple sacks can be created during a run of an application and each
    // sack opens a log file and registers two g_log handlers. To avoid dangling
    // handlers with invalid FILE pointers (we close them when destroying the
    // sack), we need to keep track of the handlers so that we can also remove
    // them.
    //
    // g_log is clever about adding log handlers. It does store all handlers
    // registered for a given domain, but only the one that was registered last
    // is used. If you remove the last registered one, the next in line will be
    // used. That means stacking sacks is ok, the handler from the last
    // undeleted sack will be the one that is used.
    guint default_log_handler_id;
    guint libdnf_log_handler_id;

    FILE *log_out;
} _SackObject;

typedef struct {
    PyObject_HEAD
    libdnf::ModulePackageContainer * ptr;
    void * ty;
    int own;
    PyObject * next;
} ModulePackageContainerPyObject;

PyObject *
new_package(PyObject *sack, Id id)
{
    _SackObject *self;

    if (!sackObject_Check(sack)) {
        PyErr_SetString(PyExc_TypeError, "Expected a _hawkey.Sack object.");
        return NULL;
    }
    self = (_SackObject*)sack;
    UniquePtrPyObject arglist;
    if (self->custom_package_class || self->custom_package_val) {
        arglist.reset(Py_BuildValue("(Oi)O", sack, id, self->custom_package_val));
    } else {
        arglist.reset(Py_BuildValue("((Oi))", sack, id));
    }
    if (!arglist)
        return NULL;
    if (self->custom_package_class)
        return PyObject_CallObject(self->custom_package_class, arglist.get());
    return PyObject_CallObject((PyObject*)&package_Type, arglist.get());
}

DnfSack *
sackFromPyObject(PyObject *o)
{
    if (!sackObject_Check(o)) {
        PyErr_SetString(PyExc_TypeError, "Expected a _hawkey.Sack object.");
        return NULL;
    }
    return ((_SackObject *)o)->sack;
}

int
sack_converter(PyObject *o, DnfSack **sack_ptr)
{
    DnfSack *sack = sackFromPyObject(o);
    if (sack == NULL)
        return 0;
    *sack_ptr = sack;
    return 1;
}

/* helpers */
static PyObject *
repo_enabled(_SackObject *self, PyObject *reponame, int enabled)
{
    PycompString cname(reponame);
    if (!cname.getCString())
        return NULL;
    dnf_sack_repo_enabled(self->sack, cname.getCString(), enabled);
    Py_RETURN_NONE;
}

/* functions on the type */

static void
sack_dealloc(_SackObject *o)
{
    Py_XDECREF(o->custom_package_class);
    Py_XDECREF(o->custom_package_val);
    if (o->sack) {
        if (auto moduleContainer = o->ModulePackageContainerPy) {
            dnf_sack_set_module_container(o->sack, NULL);
            Py_DECREF(moduleContainer);
        }
        g_object_unref(o->sack);
    }

    if (o->log_out) {
        g_log_remove_handler(nullptr, o->default_log_handler_id);
        g_log_remove_handler("libdnf", o->libdnf_log_handler_id);
        fclose(o->log_out);
    }

    Py_TYPE(o)->tp_free(o);
}

static PyObject *
sack_new(PyTypeObject *type, PyObject *args, PyObject *kwds) try
{
    _SackObject *self = (_SackObject *)type->tp_alloc(type, 0);

    if (self) {
        self->sack = NULL;
        self->custom_package_class = NULL;
        self->custom_package_val = NULL;
        self->ModulePackageContainerPy = NULL;
    }
    return (PyObject *)self;
} CATCH_TO_PYTHON

const char *
log_level_name(int level)
{
    switch (level) {
    case G_LOG_FLAG_FATAL:
        return "FATAL";
    case G_LOG_LEVEL_ERROR:
        return "ERROR";
    case G_LOG_LEVEL_CRITICAL:
        return "CRITICAL";
    case G_LOG_LEVEL_WARNING:
        return "WARN";
    case G_LOG_LEVEL_DEBUG:
        return "DEBUG";
    case G_LOG_LEVEL_INFO:
        return "INFO";
    default:
        return "(level?)";
    }
}

static void
log_handler(const gchar *log_domain, GLogLevelFlags log_level, const gchar *message, gpointer user_data)
{
    time_t t = time(NULL);
    struct tm tm;
    char timestr[32];

    FILE *log_out = (FILE*) user_data;
    localtime_r(&t, &tm);
    strftime(timestr, sizeof(timestr), "%Y-%m-%dT%H:%M:%S%z ", &tm);
    gchar *msg = g_strjoin("", timestr, log_level_name(log_level), " ", message, "\n", NULL);
    fwrite(msg, strlen(msg), 1, log_out);
    fflush(log_out);
    g_free(msg);
}

static void
log_handler_noop(const gchar *, GLogLevelFlags, const gchar *, gpointer)
{
}

static gboolean
sack_set_logfile(_SackObject *self, const gchar *path, bool debug)
{
    self->log_out = fopen(path, "a");

    if (!self->log_out)
        return FALSE;

    // The default log handler prints messages that weren't handled by any
    // other logger to stderr/stdout, we do not want that
    g_log_set_default_handler(log_handler_noop, nullptr);

    GLogLevelFlags log_mask = debug ? G_LOG_LEVEL_MASK : static_cast<GLogLevelFlags>(
        G_LOG_LEVEL_INFO |
        G_LOG_LEVEL_MESSAGE |
        G_LOG_LEVEL_WARNING |
        G_LOG_LEVEL_CRITICAL |
        G_LOG_LEVEL_ERROR);

    // set the handler for the default domain as well as "libdnf"
    self->default_log_handler_id = g_log_set_handler(nullptr, log_mask, log_handler, self->log_out);
    self->libdnf_log_handler_id = g_log_set_handler("libdnf", log_mask, log_handler, self->log_out);

    g_info("=== Started libdnf-%d.%d.%d ===", LIBDNF_MAJOR_VERSION,
            LIBDNF_MINOR_VERSION, LIBDNF_MICRO_VERSION);
    return TRUE;
}

static int
sack_init(_SackObject *self, PyObject *args, PyObject *kwds) try
{
    g_autoptr(GError) error = NULL;
    PyObject *custom_class = NULL;
    PyObject *custom_val = NULL;
    PycompString cachedir;
    const char *arch = NULL;
    const char *rootdir = NULL;
    PyObject *cachedir_py = NULL;
    PyObject *logfile_py = NULL;
    self->log_out = NULL;
    int make_cache_dir = 0;
    PyObject *debug_object = nullptr;
    gboolean all_arch = FALSE;
    const char *kwlist[] = {"cachedir", "arch", "rootdir", "pkgcls",
                      "pkginitval", "make_cache_dir", "logfile", "logdebug",
                      "all_arch", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OssOOiOO!i", (char**) kwlist,
                                     &cachedir_py, &arch, &rootdir,
                                     &custom_class, &custom_val,
                                     &make_cache_dir, &logfile_py,
                                     &PyBool_Type, &debug_object,
                                     &all_arch))
        return -1;

    bool debug = debug_object != nullptr && PyObject_IsTrue(debug_object);

    if (cachedir_py != NULL) {
        cachedir = PycompString(cachedir_py);
        if (!cachedir.getCString())
            return -1;
    }
    int flags = 0;
    if (make_cache_dir)
        flags |= DNF_SACK_SETUP_FLAG_MAKE_CACHE_DIR;
    self->sack = dnf_sack_new();
    if (all_arch) {
        dnf_sack_set_all_arch(self->sack, all_arch);
    } else {
        if (!dnf_sack_set_arch(self->sack, arch, &error)) {
            PyErr_SetString(HyExc_Arch, "Unrecognized arch for the sack.");
            return -1;
        }
    }
    dnf_sack_set_rootdir(self->sack, rootdir);
    dnf_sack_set_cachedir(self->sack, cachedir.getCString());
    if (logfile_py != NULL) {
        PycompString logfile(logfile_py);
        if (!logfile.getCString())
            return -1;
        if (!sack_set_logfile(self, logfile.getCString(), debug)) {
            PyErr_Format(PyExc_IOError, "Failed to open log file: %s", logfile.getCString());
            return -1;
        }
    }
    if (!dnf_sack_setup(self->sack, flags, &error)) {
        switch (error->code) {
        case DNF_ERROR_FILE_INVALID:
            PyErr_SetString(PyExc_IOError,
                            "Failed creating working files for the Sack.");
            break;
        case DNF_ERROR_INVALID_ARCHITECTURE:
            PyErr_SetString(HyExc_Arch, "Unrecognized arch for the sack.");
            break;
        default:
            assert(0);
        }
        return -1;
    }

    if (custom_class && custom_class != Py_None) {
        if (!PyType_Check(custom_class)) {
            PyErr_SetString(PyExc_TypeError, "Expected a class object.");
            return -1;
        }
        Py_INCREF(custom_class);
        self->custom_package_class = custom_class;
    }
    if (custom_val && custom_val != Py_None) {
        Py_INCREF(custom_val);
        self->custom_package_val = custom_val;

    }
    return 0;
} CATCH_TO_PYTHON_INT

/* getsetters */

static PyObject *
get_cache_dir(_SackObject *self, void *unused) try
{
    const char *cstr = dnf_sack_get_cache_dir(self->sack);
    if (cstr == NULL)
        Py_RETURN_NONE;
    return PyString_FromString(cstr);
} CATCH_TO_PYTHON

static int
set_installonly(_SackObject *self, PyObject *obj, void *unused) try
{
    if (!PySequence_Check(obj)) {
        PyErr_SetString(PyExc_AttributeError, "Expected a sequence.");
        return -1;
    }

    const int len = PySequence_Length(obj);
    PycompString pStrings[len];
    const char *strings[len + 1];

    for (int i = 0; i < len; ++i) {
        UniquePtrPyObject item(PySequence_GetItem(obj, i));
        if (PyUnicode_Check(item.get()) || PyString_Check(item.get())) {
            pStrings[i] = PycompString(item.get());
            strings[i] = pStrings[i].getCString();
        } else
            strings[i] = NULL;
        if (strings[i] == NULL)
            return -1;
    }
    strings[len] = NULL;

    DnfSack *sack = self->sack;
    dnf_sack_set_installonly(sack, strings);

    return 0;
} CATCH_TO_PYTHON_INT

static int
set_installonly_limit(_SackObject *self, PyObject *obj, void *unused) try
{
    int limit = (int)PyLong_AsLong(obj);
    if (PyErr_Occurred())
        return -1;
    dnf_sack_set_installonly_limit(self->sack, limit);
    return 0;
} CATCH_TO_PYTHON_INT

static int
set_module_container(_SackObject *self, PyObject *obj, void *unused) try
{
    UniquePtrPyObject thisPyContainer(PyObject_GetAttrString(obj, "this"));
    auto swigContainer = reinterpret_cast< ModulePackageContainerPyObject * >(thisPyContainer.get());
    if (swigContainer == nullptr) {
        PyErr_SetString(PyExc_SystemError, "Unable to parse ModuleContainer object");
        return -1;
    }
    auto moduleContainer = swigContainer->ptr;
    auto sack = self->sack;
    if (auto oldConteynerPy = self->ModulePackageContainerPy) {
        Py_XDECREF(oldConteynerPy);
        dnf_sack_set_module_container(sack, moduleContainer);
    } else {
        auto oldContainer = dnf_sack_set_module_container(sack, moduleContainer);
        if (oldContainer) {
            delete oldContainer;
        }
    }
    self->ModulePackageContainerPy = obj;
    Py_INCREF(obj);

    return 0;
} CATCH_TO_PYTHON_INT

static PyObject *
get_module_container(_SackObject *self, void *unused) try
{
    if (auto moduleConteinerPy = self->ModulePackageContainerPy) {
        Py_INCREF(moduleConteinerPy);
        return moduleConteinerPy;
    }
    Py_RETURN_NONE;
} CATCH_TO_PYTHON

static PyGetSetDef sack_getsetters[] = {
    {(char*)"cache_dir",        (getter)get_cache_dir, NULL, NULL, NULL},
    {(char*)"installonly",        NULL, (setter)set_installonly, NULL, NULL},
    {(char*)"installonly_limit",        NULL, (setter)set_installonly_limit, NULL, NULL},
    {(char*)"_moduleContainer",        (getter)get_module_container, (setter)set_module_container,
        NULL, NULL},
    {NULL}                        /* sentinel */
};

/* object methods */

static PyObject *
evr_cmp(_SackObject *self, PyObject *args) try
{
    const char *evr1 = NULL, *evr2 = NULL;

    if (!PyArg_ParseTuple(args, "ss", &evr1, &evr2))
        return NULL;
    int cmp = dnf_sack_evr_cmp(self->sack, evr1, evr2);
    return PyLong_FromLong(cmp);
} CATCH_TO_PYTHON

static PyObject *
get_running_kernel(_SackObject *self, PyObject *unused) try
{
    DnfSack *sack = self->sack;
    DnfPackage *cpkg = dnf_sack_get_running_kernel(sack);

    if (cpkg == NULL)
        Py_RETURN_NONE;
    PyObject *pkg = new_package((PyObject*)self, dnf_package_get_id(cpkg));
    g_object_unref(cpkg);
    return pkg;
} CATCH_TO_PYTHON

static PyObject *
create_cmdline_repo(_SackObject *self, PyObject *unused) try
{
    Py_RETURN_NONE;
} CATCH_TO_PYTHON

static PyObject *
create_package(_SackObject *self, PyObject *solvable_id) try
{
    Id id  = PyLong_AsLong(solvable_id);
    if (id <= 0) {
        PyErr_SetString(PyExc_TypeError, "Expected a positive integer.");
        return NULL;
    }
    return new_package((PyObject*)self, id);
} CATCH_TO_PYTHON

static PyObject *
add_cmdline_package(_SackObject *self, PyObject *fn_obj) try
{
    DnfPackage *cpkg;
    PyObject *pkg;
    PycompString fn(fn_obj);

    if (!fn.getCString())
        return NULL;
    cpkg = dnf_sack_add_cmdline_package_nochecksum(self->sack, fn.getCString());
    if (cpkg == NULL) {
        PyErr_Format(PyExc_IOError, "Can not load RPM file: %s.", fn.getCString());
        return NULL;
    }
    pkg = new_package((PyObject*)self, dnf_package_get_id(cpkg));
    g_object_unref(cpkg);
    return pkg;
} CATCH_TO_PYTHON

template<void (*sackExcludeIncludeFunc)(DnfSack *, const DnfPackageSet*)>
static PyObject *
modify_excl_incl(_SackObject *self, PyObject *o) try
{
    DnfSack *sack = self->sack;
    auto pset = pyseq_to_packageset(o, sack);
    if (!pset)
        return NULL;
    sackExcludeIncludeFunc(sack, pset.get());
    Py_RETURN_NONE;
} CATCH_TO_PYTHON

template<void (*sackExcludeIncludeFunc)(DnfSack *)>
static PyObject *
reset_excl_incl(_SackObject *self) try
{
    DnfSack *sack = self->sack;
    sackExcludeIncludeFunc(sack);
    Py_RETURN_NONE;
} CATCH_TO_PYTHON

template<DnfPackageSet * (*sackExcludeIncludeFunc)(DnfSack *)>
static PyObject *
get_excl_incl(_SackObject *self) try
{
    DnfSack *sack = self->sack;
    DnfPackageSet * pset = sackExcludeIncludeFunc(sack);
    if (!pset)
        return PyList_New(0);
    PyObject * pyList = packageset_to_pylist(pset, (PyObject *)self);
    delete pset;
    return pyList;
} CATCH_TO_PYTHON

static PyObject *
set_use_includes(_SackObject *self, PyObject *args) try
{
    PyObject *py_enabled;
    const char *creponame = NULL;
    if (!PyArg_ParseTuple(args, "O!|z", &PyBool_Type, &py_enabled, &creponame))
        return NULL;

    gboolean enabled = PyObject_IsTrue(py_enabled);
    if (!dnf_sack_set_use_includes(self->sack, creponame, enabled)) {
        PyErr_SetString(PyExc_ValueError, "Can't set use_includes for repo with given name.");
        return NULL;
    }

    Py_RETURN_NONE;
} CATCH_TO_PYTHON

static PyObject *
get_use_includes(_SackObject *self, PyObject *reponame) try
{
    DnfSack *sack = self->sack;

    PycompString creponame(reponame);
    if (!creponame.getCString())
        return NULL;

    gboolean enabled;
    if (!dnf_sack_get_use_includes(sack, creponame.getCString(), &enabled)) {
        PyErr_SetString(PyExc_ValueError, "Can't found repo with given name.");
        return NULL;
    }

    if (enabled)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
} CATCH_TO_PYTHON

static PyObject *
disable_repo(_SackObject *self, PyObject *reponame) try
{
    return repo_enabled(self, reponame, 0);
} CATCH_TO_PYTHON

static PyObject *
enable_repo(_SackObject *self, PyObject *reponame) try
{
    return repo_enabled(self, reponame, 1);
} CATCH_TO_PYTHON

static PyObject *
list_arches(_SackObject *self, PyObject *unused) try
{
    const char **arches = dnf_sack_list_arches(self->sack);
    PyObject *list;
    if (!arches && !dnf_sack_get_all_arch(self->sack)) {
        PyErr_SetString(HyExc_Runtime, "Arches not initialized");
        return NULL;
    }

    if (arches) {
        list = strlist_to_pylist(arches);
        g_free(arches);
        return list;
    } else {
        return PyList_New(0);
    }
} CATCH_TO_PYTHON

static PyObject *
filter_modules(_SackObject *self, PyObject *args, PyObject *kwds) try
{
    const char *kwlist[] = {"module_container", "hotfix_repos", "install_root", "platform_module",
        "update_only", "debugsolver", NULL};
    PyObject * pyModuleContainer;
    PyObject * pyHotfixRepos;
    char * installRoot = nullptr;
    char * platformModule = nullptr;
    PyObject * pyUpdateOnly = nullptr;
    PyObject * pyDebugSolver = nullptr;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OOzz|O!O!", (char**) kwlist, &pyModuleContainer,
                                     &pyHotfixRepos, &installRoot, &platformModule,  &PyBool_Type,
                                     &pyUpdateOnly,  &PyBool_Type, &pyDebugSolver))
        return 0;
    bool updateOnly = pyUpdateOnly == NULL || PyObject_IsTrue(pyUpdateOnly);
    bool debugSolver = pyDebugSolver != NULL && PyObject_IsTrue(pyDebugSolver);
    UniquePtrPyObject thisPyModuleContainer(PyObject_GetAttrString(pyModuleContainer, "this"));
    auto swigContainer = reinterpret_cast< ModulePackageContainerPyObject * >(thisPyModuleContainer.get());
    auto moduleContainer = swigContainer->ptr;
    std::vector<std::string> hotfixRepos;
    try {
        hotfixRepos = pySequenceConverter(pyHotfixRepos);
    } catch (std::runtime_error &) {
        return NULL;
    }
    std::vector<const char *> hotfixReposCString(hotfixRepos.size() + 1);
    std::transform(hotfixRepos.begin(), hotfixRepos.end(), hotfixReposCString.begin(),
        std::mem_fn(&std::string::c_str));
    try {
        auto problems = dnf_sack_filter_modules_v2(self->sack, moduleContainer, hotfixReposCString.data(),
            installRoot, platformModule, updateOnly, debugSolver);
        if (problems.second == libdnf::ModulePackageContainer::ModuleErrorType::NO_ERROR) {
            PyObject * returnTuple = PyTuple_New(0);
            return returnTuple;
        }
        PyObject * returnTuple = PyTuple_New(2);
        PyTuple_SetItem(returnTuple, 0, problemRulesPyConverter(problems.first));
        PyTuple_SetItem(returnTuple, 1, PyLong_FromLong(int(problems.second)));
        return returnTuple;
    } catch (libdnf::ModulePackageContainer::ConflictException & exception) {
        PyErr_SetString(HyExc_Runtime, exception.what());
        return NULL;
    }
} CATCH_TO_PYTHON


static PyObject *
set_modules_enabled_by_pkgset(_SackObject *self, PyObject *args, PyObject *kwds) try
{
    const char *kwlist[] = {"module_container", "pkgs", NULL};
    PyObject * pyModuleContainer;
    PyObject * pyPkgSet;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "OO", (char**) kwlist, &pyModuleContainer,
                                     &pyPkgSet))
        return NULL;
    auto pset = pyseq_to_packageset(pyPkgSet, self->sack);
    if (!pset) {
        return NULL;
    }

    UniquePtrPyObject thisPyModuleContainer(PyObject_GetAttrString(pyModuleContainer, "this"));
    auto swigContainer = reinterpret_cast< ModulePackageContainerPyObject * >(thisPyModuleContainer.get());
    auto moduleContainer = swigContainer->ptr;
    auto modules = moduleContainer->requiresModuleEnablement(*pset.get());
    moduleContainer->enableDependencyTree(modules);
    Py_RETURN_NONE;
} CATCH_TO_PYTHON

typedef struct {
    PyObject_HEAD
    libdnf::Repo *ptr;
    void *ty;
    int own;
    PyObject *next;
} RepoSwigPyObject;

static PyObject *
load_system_repo(_SackObject *self, PyObject *args, PyObject *kwds)
{
    g_autoptr(GError) error = NULL;
    const char *kwlist[] = {"repo", "build_cache", "load_filelists", "load_presto",
                      NULL};

    PyObject * repoPyObj = NULL;
    libdnf::Repo * crepo = NULL;
    int build_cache = 0, unused_1 = 0, unused_2 = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|Oiii", (char**) kwlist,
                                     &repoPyObj,
                                     &build_cache, &unused_1, &unused_2))
        return 0;

    if (repoPyObj) {

        // Is it old deprecated _hawkey.Repo object?
        crepo = repoFromPyObject(repoPyObj);

        // Or is it swig object?
        if (!crepo) {
            UniquePtrPyObject thisRepoPyObj(PyObject_GetAttrString(repoPyObj, "this"));
            auto repoSwigPyObj = reinterpret_cast<RepoSwigPyObject *>(thisRepoPyObj.get());
            if (!repoSwigPyObj) {
                PyErr_SetString(PyExc_SystemError, "Unable to parse repoSwigPyObject");
                return NULL;
            }

            crepo = repoSwigPyObj->ptr;
            if (!crepo) {
                PyErr_SetString(PyExc_SystemError, "Unable to parse repo swig object");
                return NULL;
            }
        }
    }

    int flags = 0;
    if (build_cache)
        flags |= DNF_SACK_LOAD_FLAG_BUILD_CACHE;

    gboolean ret = dnf_sack_load_system_repo(self->sack, crepo, flags, &error);
    if (!ret)
        return op_error2exc(error);
    Py_RETURN_NONE;
}

static PyObject *
load_repo(_SackObject *self, PyObject *args, PyObject *kwds) try
{
    const char *kwlist[] = {"repo", "build_cache", "load_filelists", "load_presto",
                      "load_updateinfo", "load_other", NULL};

    PyObject * repoPyObj = NULL;
    int build_cache = 0, load_filelists = 0, load_presto = 0, load_updateinfo = 0, load_other = 0;
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|iiiii", (char**) kwlist,
                                     &repoPyObj,
                                     &build_cache, &load_filelists,
                                     &load_presto, &load_updateinfo, &load_other))
        return 0;

    // Is it old deprecated _hawkey.Repo object?
    libdnf::Repo * crepo = repoFromPyObject(repoPyObj);

    // Or is it swig object?
    if (!crepo) {
        UniquePtrPyObject thisRepoPyObj(PyObject_GetAttrString(repoPyObj, "this"));
        auto repoSwigPyObj = reinterpret_cast<RepoSwigPyObject *>(thisRepoPyObj.get());
        if (!repoSwigPyObj) {
            PyErr_SetString(PyExc_SystemError, "Unable to parse repoSwigPyObject");
            return NULL;
        }

        crepo = repoSwigPyObj->ptr;
        if (!crepo) {
            PyErr_SetString(PyExc_SystemError, "Unable to parse repo swig object");
            return NULL;
        }
    }

    int flags = 0;
    gboolean ret = 0;
    g_autoptr(GError) error = NULL;
    if (build_cache)
        flags |= DNF_SACK_LOAD_FLAG_BUILD_CACHE;
    if (load_filelists)
        flags |= DNF_SACK_LOAD_FLAG_USE_FILELISTS;
    if (load_presto)
        flags |= DNF_SACK_LOAD_FLAG_USE_PRESTO;
    if (load_updateinfo)
        flags |= DNF_SACK_LOAD_FLAG_USE_UPDATEINFO;
    if (load_other)
        flags |= DNF_SACK_LOAD_FLAG_USE_OTHER;
    Py_BEGIN_ALLOW_THREADS;
    ret = dnf_sack_load_repo(self->sack, crepo, flags, &error);
    Py_END_ALLOW_THREADS;
    if (!ret)
        return op_error2exc(error);
    Py_RETURN_NONE;
} CATCH_TO_PYTHON

static PyObject *
rpmdb_version(_SackObject *self, PyObject *unused) try
{
    auto result = dnf_sack_get_rpmdb_version(self->sack);
    return PyString_FromString(result.c_str());
} CATCH_TO_PYTHON

static Py_ssize_t
len(_SackObject *self) try
{
    return dnf_sack_count(self->sack);
} CATCH_TO_PYTHON_INT

static PyObject *
deepcopy(_SackObject *self, PyObject *args, PyObject *kwds) try
{
    PyErr_SetString(PyExc_NotImplementedError, "sack can't be deepcopied");
    return NULL;
} CATCH_TO_PYTHON

static struct
PyMethodDef sack_methods[] = {
    {"__deepcopy__", (PyCFunction)deepcopy, METH_KEYWORDS|METH_VARARGS,
     NULL},
    {"evr_cmp",                (PyCFunction)evr_cmp, METH_VARARGS,
     NULL},
    {"get_running_kernel", (PyCFunction)get_running_kernel, METH_NOARGS,
     NULL},
    {"create_cmdline_repo", (PyCFunction)create_cmdline_repo, METH_NOARGS,
     NULL},
    {"create_package", (PyCFunction)create_package, METH_O,
     NULL},
    {"add_cmdline_package", (PyCFunction)add_cmdline_package, METH_O,
     NULL},
    {"add_excludes", (PyCFunction)modify_excl_incl<&dnf_sack_add_excludes>, METH_O, NULL},
    {"add_module_excludes", (PyCFunction)modify_excl_incl<&dnf_sack_add_module_excludes>, METH_O,
     NULL},
    {"add_includes", (PyCFunction)modify_excl_incl<&dnf_sack_add_includes>, METH_O, NULL},
    {"remove_excludes", (PyCFunction)modify_excl_incl<&dnf_sack_remove_excludes>, METH_O, NULL},
    {"remove_module_excludes", (PyCFunction)modify_excl_incl<&dnf_sack_remove_module_excludes>,
     METH_O, NULL},
    {"remove_includes", (PyCFunction)modify_excl_incl<&dnf_sack_remove_includes>, METH_O, NULL},
    {"set_excludes", (PyCFunction)modify_excl_incl<&dnf_sack_set_excludes>, METH_O, NULL},
    {"set_module_excludes", (PyCFunction)modify_excl_incl<&dnf_sack_set_module_excludes>, METH_O,
     NULL},
    {"set_includes", (PyCFunction)modify_excl_incl<&dnf_sack_set_includes>, METH_O, NULL},
    {"reset_excludes", (PyCFunction)reset_excl_incl<&dnf_sack_reset_excludes>, METH_NOARGS,
     NULL},
    {"reset_module_excludes", (PyCFunction)reset_excl_incl<&dnf_sack_reset_module_excludes>,
     METH_NOARGS, NULL},
    {"reset_includes", (PyCFunction)reset_excl_incl<&dnf_sack_reset_includes>, METH_NOARGS,
     NULL},
    {"get_excludes", (PyCFunction)get_excl_incl<&dnf_sack_get_excludes>, METH_NOARGS,
     NULL},
    {"get_module_excludes", (PyCFunction)get_excl_incl<&dnf_sack_get_module_excludes>, METH_NOARGS,
     NULL},
    {"get_includes", (PyCFunction)get_excl_incl<&dnf_sack_get_includes>, METH_NOARGS,
     NULL},
    {"set_use_includes", (PyCFunction)set_use_includes, METH_VARARGS,
     NULL},
    {"get_use_includes", (PyCFunction)get_use_includes, METH_O,
     NULL},
    {"disable_repo", (PyCFunction)disable_repo, METH_O,
     NULL},
    {"enable_repo", (PyCFunction)enable_repo, METH_O,
     NULL},
    {"list_arches", (PyCFunction)list_arches, METH_NOARGS,
     NULL},
    {"filter_modules", (PyCFunction)filter_modules, METH_VARARGS | METH_KEYWORDS, NULL},
    {"set_modules_enabled_by_pkgset", (PyCFunction)set_modules_enabled_by_pkgset,
        METH_VARARGS | METH_KEYWORDS, NULL},
    {"load_system_repo", (PyCFunction)load_system_repo,
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"load_repo", (PyCFunction)load_repo, METH_VARARGS | METH_KEYWORDS,
     NULL},
    {"_rpmdb_version", (PyCFunction)rpmdb_version, METH_VARARGS | METH_KEYWORDS, NULL},
    {NULL}                      /* sentinel */
};

PySequenceMethods sack_sequence = {
    (lenfunc)len,                /* sq_length */
};

PyTypeObject sack_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "_hawkey.Sack",                /*tp_name*/
    sizeof(_SackObject),        /*tp_basicsize*/
    0,                                /*tp_itemsize*/
    (destructor) sack_dealloc,  /*tp_dealloc*/
    0,                                /*tp_print*/
    0,                                /*tp_getattr*/
    0,                                /*tp_setattr*/
    0,                                /*tp_compare*/
    0,                                /*tp_repr*/
    0,                                /*tp_as_number*/
    &sack_sequence,                /*tp_as_sequence*/
    0,                                /*tp_as_mapping*/
    0,                                /*tp_hash */
    0,                                /*tp_call*/
    0,                                /*tp_str*/
    0,                                /*tp_getattro*/
    0,                                /*tp_setattro*/
    0,                                /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,        /*tp_flags*/
    "Sack object",                /* tp_doc */
    0,                                /* tp_traverse */
    0,                                /* tp_clear */
    0,                                /* tp_richcompare */
    0,                                /* tp_weaklistoffset */
    PyObject_SelfIter,                /* tp_iter */
    0,                                 /* tp_iternext */
    sack_methods,                /* tp_methods */
    0,                                /* tp_members */
    sack_getsetters,                /* tp_getset */
    0,                                /* tp_base */
    0,                                /* tp_dict */
    0,                                /* tp_descr_get */
    0,                                /* tp_descr_set */
    0,                                /* tp_dictoffset */
    (initproc)sack_init,        /* tp_init */
    0,                                /* tp_alloc */
    sack_new,                        /* tp_new */
    0,                                /* tp_free */
    0,                                /* tp_is_gc */
};
