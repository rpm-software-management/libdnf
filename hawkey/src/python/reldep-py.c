#include <Python.h>

// libsolv
#include <solv/util.h>

// hawkey
#include "src/reldep_internal.h"
#include "src/sack_internal.h"

// pyhawkey
#include "exception-py.h"
#include "sack-py.h"
#include "reldep-py.h"

typedef struct {
    PyObject_HEAD
    HyReldep reldep;
    PyObject *sack;
} _ReldepObject;

PyObject *
new_reldep(PyObject *sack, Id r_id)
{
    PyObject *arglist = Py_BuildValue("Oi", sack, r_id);
    if (arglist == NULL)
	return NULL;
    PyObject *reldep = PyObject_CallObject((PyObject*)&reldep_Type, arglist);
    return reldep;
}

HyReldep
reldepFromPyObject(PyObject *o)
{
    if (!PyType_IsSubtype(o->ob_type, &reldep_Type)) {
	PyErr_SetString(PyExc_TypeError, "Expected a Reldep object.");
	return NULL;
    }
    return ((_ReldepObject*)o)->reldep;
}

static Id reldep_hash(_ReldepObject *self);

static PyObject *
reldep_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *sack;
    Id id;
    HySack csack;
    if (!PyArg_ParseTuple(args, "O!i", &sack_Type, &sack, &id))
	return NULL;
    csack = sackFromPyObject(sack);
    if (csack == NULL)
	return NULL;

    _ReldepObject *self = (_ReldepObject*)type->tp_alloc(type, 0);
    if (self == NULL)
	return NULL;
    self->reldep = reldep_create(sack_pool(csack), id);
    self->sack = sack;
    Py_INCREF(self->sack);
    return (PyObject*)self;
}

static void
reldep_dealloc(_ReldepObject *self)
{
    if (self->reldep)
	hy_reldep_free(self->reldep);

    Py_XDECREF(self->sack);
    Py_TYPE(self)->tp_free(self);
}

static PyObject *
reldep_repr(_ReldepObject *self)
{
    return PyString_FromFormat("<_hawkey.Reldep object, id: %x>",
			       reldep_hash(self));
}

static PyObject *
reldep_str(_ReldepObject *self)
{
    HyReldep reldep = self->reldep;
    char *cstr = hy_reldep_str(reldep);
    PyObject *retval = PyString_FromString(cstr);
    solv_free(cstr);
    return retval;
}

static Id
reldep_hash(_ReldepObject *self)
{
    return reldep_id(self->reldep);
}

PyTypeObject reldep_Type = {
    PyObject_HEAD_INIT(NULL)
    0,				/*ob_size*/
    "_hawkey.Reldep",		/*tp_name*/
    sizeof(_ReldepObject),	/*tp_basicsize*/
    0,				/*tp_itemsize*/
    (destructor) reldep_dealloc, /*tp_dealloc*/
    0,				/*tp_print*/
    0,				/*tp_getattr*/
    0,				/*tp_setattr*/
    0,				/*tp_compare*/
    (reprfunc)reldep_repr,	/*tp_repr*/
    0,				/*tp_as_number*/
    0,				/*tp_as_sequence*/
    0,				/*tp_as_mapping*/
    (hashfunc)reldep_hash,	/*tp_hash */
    0,				/*tp_call*/
    (reprfunc)reldep_str,	/*tp_str*/
    0,				/*tp_getattro*/
    0,				/*tp_setattro*/
    0,				/*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,	/*tp_flags*/
    "Reldep object",		/* tp_doc */
    0,				/* tp_traverse */
    0,				/* tp_clear */
    0,				/* tp_richcompare */
    0,				/* tp_weaklistoffset */
    0,				/* tp_iter */
    0,                         	/* tp_iternext */
    0,				/* tp_methods */
    0,				/* tp_members */
    0,				/* tp_getset */
    0,				/* tp_base */
    0,				/* tp_dict */
    0,				/* tp_descr_get */
    0,				/* tp_descr_set */
    0,				/* tp_dictoffset */
    0,				/* tp_init */
    0,				/* tp_alloc */
    reldep_new,			/* tp_new */
    0,				/* tp_free */
    0,				/* tp_is_gc */
};
