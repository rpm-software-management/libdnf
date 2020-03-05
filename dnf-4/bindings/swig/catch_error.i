// Beware of nested %includes / %imports. SWIG has a guard to %include each
// file only once, but if you include this in a file that you later import into
// another file, the exception handler will not work and a subsequent %include of
// this file will not work either.

%include <exception.i>

%{
    #include "libdnf/error.hpp"
    extern PyObject* libdnf_error;
%}

%exception {
    try {
        $action
    } catch (const libdnf::Error & e) {
        PyErr_SetString(libdnf_error, const_cast<char*>(e.what()));
        SWIG_fail;
    } catch (const std::out_of_range & e) {
        SWIG_exception(SWIG_IndexError, e.what());
    } catch (const std::exception & e) {
        SWIG_exception(SWIG_RuntimeError, e.what());
    }
}
