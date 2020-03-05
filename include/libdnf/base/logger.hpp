/*
Copyright (C) 2020 Red Hat, Inc.

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


#ifndef LIBDNF_BASE_LOGGER_HPP
#define LIBDNF_BASE_LOGGER_HPP

// forward declarations
namespace libdnf {
class Logger;
}  // namespace libdnf


#include "base.hpp"

#include <string>


namespace libdnf {


/// Logger is a logging proxy that forwards data via callbacks.
/// It also buffers early logging data before logging is fully initiated.
///
/// @replaces libdnf:libdnf/log.hpp:class:Log
class Logger {
public:
    void trace(std::string msg);
    void debug(std::string msg);
    void info(std::string msg);
    void warning(std::string msg);
    void error(std::string msg);

protected:
    explicit Logger(Base & dnf_base);

private:
    const Base & dnf_base;
};


/*
Logger::Logger(Base & dnf_base)
    : dnf_base{dnf_base}
{
}
*/


}  // namespace libdnf

#endif
