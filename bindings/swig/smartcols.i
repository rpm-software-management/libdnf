%module smartcols

%include <exception.i>
%include <std_shared_ptr.i>
%include <std_string.i>


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

%exception {
    try {
        $action
    } catch (const std::exception & e) {
       SWIG_exception(SWIG_RuntimeError, e.what());
    }
}

%include "libdnf/utils/smartcols/Table.hpp"
%include "libdnf/utils/smartcols/Column.hpp"
%include "libdnf/utils/smartcols/Line.hpp"
%include "libdnf/utils/smartcols/Cell.hpp"
