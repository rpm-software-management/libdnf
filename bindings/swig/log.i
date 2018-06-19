%module log

%begin %{
#define SWIG_PYTHON_2_UNICODE
%}

%include <std_shared_ptr.i>
%include <std_string.i>


%shared_ptr(FileLogger)
%shared_ptr(StderrLogger)
%shared_ptr(StdoutLogger)


%{
    // make SWIG wrap following headers
    #include "libdnf/log/log.hpp"
%}


// make SWIG look into following headers
%include "libdnf/log/log.hpp"
