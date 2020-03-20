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

#ifndef LIBDNF_CONF_OPTION_STRING_HPP
#define LIBDNF_CONF_OPTION_STRING_HPP

#include "option.hpp"

namespace libdnf {

class OptionString : public Option {
public:
    using ValueType = std::string;

    class InvalidValue : public Option::InvalidValue {
    public:
        using Option::InvalidValue::InvalidValue;
        const char * get_domain_name() const noexcept override { return "libdnf::OptionString"; }
    };

    class NotAllowedValue : public InvalidValue {
    public:
        using InvalidValue::InvalidValue;
        const char * get_name() const noexcept override { return "NotAllowedValue"; }
    };

    class ValueNotSet : public Exception {
    public:
        ValueNotSet() : Exception("") {}
        const char * get_domain_name() const noexcept override { return "libdnf::OptionString"; }
        const char * get_name() const noexcept override { return "ValueNotSet"; }
    };

    explicit OptionString(const std::string & default_value);
    explicit OptionString(const char * default_value);
    OptionString(const std::string & default_value, std::string regex, bool icase);
    OptionString(const char * default_value, std::string regex, bool icase);
    OptionString * clone() const override;
    void test(const std::string & value) const;
    void set(Priority priority, const std::string & value) override;
    std::string from_string(const std::string & value) const;
    const std::string & get_value() const;
    const std::string & get_default_value() const noexcept;
    std::string get_value_string() const override;

protected:
    std::string regex;
    bool icase;
    std::string default_value;
    std::string value;
};

inline OptionString * OptionString::clone() const {
    return new OptionString(*this);
}

inline const std::string & OptionString::get_default_value() const noexcept {
    return default_value;
}

inline std::string OptionString::get_value_string() const {
    return get_value();
}

inline std::string OptionString::from_string(const std::string & value) const {
    return value;
}

}  // namespace libdnf

#endif
