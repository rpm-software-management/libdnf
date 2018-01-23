%module utils


%include <std_shared_ptr.i>
%include <std_string.i>


%{
    // make SWIG wrap following headers
    #include "libdnf/utils/sqlite3/sqlite3.hpp"
%}


%shared_ptr(SQLite3)
%nocopyctor SQLite3;


// make SWIG look into following headers
class SQLite3 {
public:
    SQLite3(const SQLite3 &) = delete;
    SQLite3 & operator=(const SQLite3 &) = delete;
    SQLite3(const char *dbPath);
};
