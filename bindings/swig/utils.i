%module utils

%begin %{
#define SWIG_PYTHON_2_UNICODE
%}

%include <exception.i>
%include <std_shared_ptr.i>
%include <std_string.i>


%{
    // make SWIG wrap following headers
    #include "libdnf/utils/sqlite3/Sqlite3.hpp"
%}


%exception {
    try {
        $action
    }
    catch (const std::exception & e)
    {
       SWIG_exception(SWIG_RuntimeError, (std::string("C++ std::exception: ") + e.what()).c_str());
    }
    catch (...)
    {
       SWIG_exception(SWIG_UnknownError, "C++ anonymous exception");
    }
}

%shared_ptr(SQLite3)
%nocopyctor SQLite3;


// make SWIG look into following headers
class SQLite3 {
public:
    SQLite3(const SQLite3 &) = delete;
    SQLite3 & operator=(const SQLite3 &) = delete;
    SQLite3(const char *dbPath);
    void close();
};
