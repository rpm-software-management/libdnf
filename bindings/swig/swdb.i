%module swdb


%include <std_string.i>
%include <stdint.i>


%{
    // make SWIG wrap following headers
    #include "libdnf/swdb/swdb.hpp"
    #include "libdnf/swdb/transaction.hpp"
%}


// make SWIG look into following headers
%include "libdnf/swdb/swdb.hpp"
%include "libdnf/swdb/transaction.hpp"
