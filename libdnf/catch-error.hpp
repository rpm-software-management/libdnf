#ifndef LIBDNF_CATCH_ERROR_HPP
#define LIBDNF_CATCH_ERROR_HPP

#include "error.hpp"

#include <glib.h>


#define CATCH_TO_GERROR(RET) catch (const libdnf::Error& e) {                  \
    g_set_error_literal(error, DNF_ERROR, DNF_ERROR_INTERNAL_ERROR, e.what()); \
    return RET;                                                                \
}

#endif
