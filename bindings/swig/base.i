%module(directors="1") base

%begin %{
#define SWIG_PYTHON_2_UNICODE
%}

%{
    // make SWIG wrap following headers
    #include "libdnf/base/Base.hpp"
    #include "libdnf/base/Logger.hpp"
%}

%feature("director") libdnf::Logger;
