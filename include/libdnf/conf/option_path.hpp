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

#ifndef LIBDNF_CONF_OPTION_PATH_HPP
#define LIBDNF_CONF_OPTION_PATH_HPP

#include "option_string.hpp"

namespace libdnf {

/**
* @class OptionPath
*
* @brief Option for file path which can validate path existence.
*
*/
class OptionPath : public OptionString {
public:
    class NotAllowedValue : public InvalidValue {
    public:
        using InvalidValue::InvalidValue;
        const char * get_domain_name() const noexcept override { return "libdnf::OptionPath"; }
        const char * get_name() const noexcept override { return "NotAllowedValue"; }
    };

    class PathNotExists : public InvalidValue {
    public:
        using InvalidValue::InvalidValue;
        const char * get_domain_name() const noexcept override { return "libdnf::OptionPath"; }
        const char * get_name() const noexcept override { return "PathNotExists"; }
    };

    explicit OptionPath(const std::string & default_value, bool exists = false, bool abs_path = false);
    explicit OptionPath(const char * default_value, bool exists = false, bool abs_path = false);
    OptionPath(
        const std::string & default_value,
        const std::string & regex,
        bool icase,
        bool exists = false,
        bool abs_path = false);
    OptionPath(
        const char * default_value, const std::string & regex, bool icase, bool exists = false, bool abs_path = false);
    OptionPath * clone() const override;
    void test(const std::string & value) const;
    void set(Priority priority, const std::string & value) override;

private:
    bool exists;
    bool abs_path;
};

inline OptionPath * OptionPath::clone() const {
    return new OptionPath(*this);
}

}  // namespace libdnf

#endif
