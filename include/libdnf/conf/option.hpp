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

#ifndef LIBDNF_CONF_OPTION_HPP
#define LIBDNF_CONF_OPTION_HPP

#include "libdnf/utils/exception.hpp"

#include <string>

namespace libdnf {

class Option {
public:
    enum class Priority {
        EMPTY = 0,
        DEFAULT = 10,
        MAINCONFIG = 20,
        AUTOMATICCONFIG = 30,
        REPOCONFIG = 40,
        PLUGINDEFAULT = 50,
        PLUGINCONFIG = 60,
        DROPINCONFIG = 65,
        COMMANDLINE = 70,
        RUNTIME = 80
    };

    class Exception : public RuntimeError {
    public:
        using RuntimeError::RuntimeError;
        const char * get_domain_name() const noexcept override { return "libdnf::Option"; }
        const char * get_name() const noexcept override { return "Exception"; }
        const char * get_description() const noexcept override { return "Option exception"; }
    };

    class InvalidValue : public Exception {
    public:
        explicit InvalidValue(const std::string & msg) : Exception(msg) {}
        explicit InvalidValue(const char * msg) : Exception(msg) {}
        const char * get_name() const noexcept override { return "InvalidValue"; }
        const char * get_description() const noexcept override { return "Invalid value"; }
    };

    explicit Option(Priority priority = Priority::EMPTY);
    Option(const Option & src) = default;
    virtual ~Option() = default;

    virtual Option * clone() const = 0;
    virtual Priority get_priority() const;
    virtual void set(Priority priority, const std::string & value) = 0;
    virtual std::string get_value_string() const = 0;
    virtual bool empty() const noexcept;

protected:
    void set_priority(Priority priority);

private:
    Priority priority;
};

inline Option::Option(Priority priority) : priority(priority) {}

inline Option::Priority Option::get_priority() const {
    return priority;
}

inline bool Option::empty() const noexcept {
    return priority == Priority::EMPTY;
}

inline void Option::set_priority(Priority priority) {
    this->priority = priority;
}

}  // namespace libdnf

#endif
