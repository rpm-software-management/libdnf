%module base


%include <exception.i>
%include <std_string.i>


%{
    // make SWIG wrap following headers
    #include "libdnf/base/all.hpp"
%}

#define CV __perl_CV
%include "libdnf/base/all.hpp"
