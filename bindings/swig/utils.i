%module utils


%include <exception.i>
%include <std_string.i>
%include <stdint.i>


%{
    // make SWIG wrap following headers
    #include "libdnf/utils/number.hpp"
%}

#define CV __perl_CV
%include "libdnf/utils/number.hpp"
