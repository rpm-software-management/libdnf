%module utils


%include <std_string.i>


%{
    // make SWIG wrap following headers
    #include "libdnf/utils/sqlite3/sqlite3.hpp"
%}


// make SWIG look into following headers
class SQLite3
{
public:
    SQLite3(const char *dbPath);
};
