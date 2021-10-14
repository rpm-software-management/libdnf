/*
Copyright (C) 2021 Red Hat, Inc.

This file is part of libdnf: https://github.com/rpm-software-management/libdnf/

Libdnf is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Libdnf is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with libdnf.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef LIBDNF_COMMON_FORMAT_HPP
#define LIBDNF_COMMON_FORMAT_HPP

#if __has_include(<format>)
#include <format>
#endif

#ifdef __cpp_lib_format

namespace libdnf {
    using std::format;
    using std::vformat;
    using std::make_format_args;
}

#else

// If the compiler does not support `std::format`, use the fmt library.
#include <fmt/format.h>
namespace libdnf {
    using fmt::format;
    using fmt::vformat;
    using fmt::make_format_args;
}

#endif  // __cpp_lib_format

namespace libdnf {

/// Format `args` according to the `runtime_format_string`, and return the result as a string.
template <typename... Args>
inline std::string format_runtime(std::string_view runtime_format_string, Args&&... args) {
    return vformat(runtime_format_string, make_format_args(args...));
}

}

#endif  // LIBDNF_COMMON_FORMAT_HPP
