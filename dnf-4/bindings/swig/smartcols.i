%module smartcols

%include <std_shared_ptr.i>
%include <std_string.i>

%include "catch_error.i"

%{
// make SWIG wrap following headers
#include "libdnf/utils/smartcols/Table.hpp"
#include "libdnf/utils/smartcols/Column.hpp"
#include "libdnf/utils/smartcols/Line.hpp"
#include "libdnf/utils/smartcols/Cell.hpp"
%}

%rename(_print) print;

%shared_ptr(Column)
%shared_ptr(Line)
%shared_ptr(Cell)
%shared_ptr(Table)

%include "libdnf/utils/smartcols/Table.hpp"
%include "libdnf/utils/smartcols/Column.hpp"
%include "libdnf/utils/smartcols/Line.hpp"
%include "libdnf/utils/smartcols/Cell.hpp"
