#ifndef LIBDNF_ERROR_HPP
#define LIBDNF_ERROR_HPP

#include <stdexcept>


namespace libdnf {

/**
 * A class to use or inherit from for all errors that are expected to happen
 * (e.g. file not found, permissions, a failed HTTP request, even a corrupted
 * SQLite database).
 */
class Error : public std::runtime_error {
public:
    Error(const std::string & what) : runtime_error(what) {}
};

/**
 * An unexpected error (that is, really an exception), for errors that
 * shouldn't occur. Similar in semantics to an assert error, or
 * std::logic_error (which is misused in the standard library and e.g.
 * std::stoi throws it as the std::invalid_argument error, which is an error
 * that can occur under regular circumstances).
 *
 * By decision we do not catch these exceptions as of now. By not catching it,
 * a terminate() is called and a traceback can be retrieved, which is not the
 * case when the exception is caught. Since these are an unexpected state, a
 * crash is an acceptable outcome and having the traceback is important for
 * debugging the issue.
 *
 * Therefore, Exception is unrelated to Error (neither inherits from the other).
 */
class Exception : public std::runtime_error {
public:
    Exception(const std::string & what) : runtime_error(what) {}
};

} // namespace libdnf

#endif
