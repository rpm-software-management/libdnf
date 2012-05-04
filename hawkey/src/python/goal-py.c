#include <Python.h>
#include <structmember.h>
#include <stddef.h>

// libsolv
#include "solv/util.h"

// hawkey
#include "src/goal.h"
#include "src/package_internal.h"

// pyhawkey
#include "goal-py.h"
#include "iutil-py.h"
#include "package-py.h"
#include "sack-py.h"

typedef struct {
    PyObject_HEAD
    HyGoal goal;
    PyObject *sack;
} _GoalObject;

/* functions on the type */

static PyObject *
goal_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    _GoalObject *self = (_GoalObject*)type->tp_alloc(type, 0);
    if (self) {
	self->goal = NULL;
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
    HySack csack;

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
erase(_GoalObject *self, PyObject *pkgob)
{
    HyPackage pkg = packageFromPyObject(pkgob);
    if (pkg == NULL)
	return NULL;
    hy_goal_erase(self->goal, pkg);
    Py_RETURN_NONE;
}

static PyObject *
install(_GoalObject *self, PyObject *pkgob)
{
    HyPackage pkg = packageFromPyObject(pkgob);
    if (pkg == NULL)
	return NULL;
    hy_goal_install(self->goal, pkg);
    Py_RETURN_NONE;
}

static PyObject *
update(_GoalObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *pkgob = NULL;
    int check_later_version = 1;
    char *kwlist[] = {"package", "check_later_version", NULL};
    int ret;
    int flags = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O|i", kwlist,
				     &pkgob, &check_later_version))
	return NULL;
    HyPackage pkg = packageFromPyObject(pkgob);
    if (pkg == NULL)
	return NULL;

    if (check_later_version)
	flags |= HY_CHECK_INSTALLED;
    ret = hy_goal_update_flags(self->goal, pkg, flags);
    if (!ret)
	Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject *
upgrade_all(_GoalObject *self, PyObject *unused)
{
    int ret = hy_goal_upgrade_all(self->goal);
    if (!ret)
	Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject *
go(_GoalObject *self, PyObject *unused)
{
    int ret = hy_goal_go(self->goal);
    if (!ret)
	Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyObject *
count_problems(_GoalObject *self, PyObject *unused)
{
    return PyInt_FromLong(hy_goal_count_problems(self->goal));
}

static PyObject *
describe_problem(_GoalObject *self, PyObject *index_obj)
{
    char *cstr;
    PyObject *str;

    if (!PyInt_Check(index_obj)) {
	PyErr_SetString(PyExc_ValueError, "integer value expected");
	return NULL;
    }
    cstr = hy_goal_describe_problem(self->goal, PyInt_AsLong(index_obj));
    str = PyString_FromString(cstr);
    solv_free(cstr);
    return str;
}

static PyObject *
list_generic(_GoalObject *self, HyPackageList (*func)(HyGoal))
{
    HyPackageList plist = func(self->goal);
    PyObject *list;

    list = packagelist_to_pylist(plist, self->sack);
    hy_packagelist_free(plist);
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
list_upgrades(_GoalObject *self, PyObject *unused)
{
    return list_generic(self, hy_goal_list_upgrades);
}

static PyObject *
package_upgrades(_GoalObject *self, PyObject *pkg)
{
    HyPackage cpkg = packageFromPyObject(pkg);
    HyPackage cpkg_upgraded;
    PyObject *pkg_upgraded;

    if (cpkg == NULL)
	return NULL;
    cpkg_upgraded = hy_goal_package_upgrades(self->goal, cpkg);
    pkg_upgraded = new_package(self->sack, package_id(cpkg_upgraded));
    hy_package_free(cpkg_upgraded);
    return pkg_upgraded;
}

static struct PyMethodDef goal_methods[] = {
    {"erase",		(PyCFunction)erase,		METH_O, NULL},
    {"install",		(PyCFunction)install,		METH_O, NULL},
    {"update",		(PyCFunction)update,
     METH_VARARGS | METH_KEYWORDS, NULL},
    {"upgrade_all",	(PyCFunction)upgrade_all,	METH_NOARGS, NULL},
    {"go",		(PyCFunction)go,		METH_NOARGS, NULL},
    {"count_problems",	(PyCFunction)count_problems,	METH_NOARGS, NULL},
    {"describe_problem",(PyCFunction)describe_problem,	METH_O, NULL},
    {"list_erasures",	(PyCFunction)list_erasures,	METH_NOARGS, NULL},
    {"list_installs",	(PyCFunction)list_installs,	METH_NOARGS, NULL},
    {"list_upgrades",	(PyCFunction)list_upgrades,	METH_NOARGS, NULL},
    {"package_upgrades",(PyCFunction)package_upgrades,	METH_O, NULL},
    {NULL}                      /* sentinel */
};

static struct PyMemberDef goal_members[] = {
    {"sack", T_OBJECT, offsetof(_GoalObject, sack), READONLY, NULL},
    {NULL}
};

PyTypeObject goal_Type = {
    PyObject_HEAD_INIT(NULL)
    0,				/*ob_size*/
    "_hawkey.Goal",		/*tp_name*/
    sizeof(_GoalObject),	/*tp_basicsize*/
    0,				/*tp_itemsize*/
    (destructor) goal_dealloc,  /*tp_dealloc*/
    0,				/*tp_print*/
    0,				/*tp_getattr*/
    0,				/*tp_setattr*/
    0,				/*tp_compare*/
    0,				/*tp_repr*/
    0,				/*tp_as_number*/
    0,				/*tp_as_sequence*/
    0,				/*tp_as_mapping*/
    0,				/*tp_hash */
    0,				/*tp_call*/
    0,				/*tp_str*/
    PyObject_GenericGetAttr,	/*tp_getattro*/
    0,				/*tp_setattro*/
    0,				/*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,	/*tp_flags*/
    "Goal object",		/* tp_doc */
    0,				/* tp_traverse */
    0,				/* tp_clear */
    0,				/* tp_richcompare */
    0,				/* tp_weaklistoffset */
    PyObject_SelfIter,		/* tp_iter */
    0,                         	/* tp_iternext */
    goal_methods,		/* tp_methods */
    goal_members,		/* tp_members */
    0,				/* tp_getset */
    0,				/* tp_base */
    0,				/* tp_dict */
    0,				/* tp_descr_get */
    0,				/* tp_descr_set */
    0,				/* tp_dictoffset */
    (initproc)goal_init,	/* tp_init */
    0,				/* tp_alloc */
    goal_new,			/* tp_new */
    0,				/* tp_free */
    0,				/* tp_is_gc */
};
