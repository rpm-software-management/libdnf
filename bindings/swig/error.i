%module error

%{
    #include "libdnf/error.hpp"
    #define SWIG_FILE_WITH_INIT
    PyObject* libdnf_error;
%}

%init %{
    libdnf_error = PyErr_NewException("libdnf._error.Error", NULL, NULL);
    Py_INCREF(libdnf_error);
    PyModule_AddObject(m, "Error", libdnf_error);
%}

%pythoncode %{
    Error = _error.Error
%}
