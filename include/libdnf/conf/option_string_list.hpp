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

#ifndef LIBDNF_CONF_OPTION_STRING_LIST_HPP
#define LIBDNF_CONF_OPTION_STRING_LIST_HPP

#include "option.hpp"

#include <vector>

namespace libdnf {

class OptionStringList : public Option {
public:
    using ValueType = std::vector<std::string>;

    class NotAllowedValue : public InvalidValue {
    public:
        using InvalidValue::InvalidValue;
        const char * get_domain_name() const noexcept override { return "libdnf::OptionStringList"; }
        const char * get_name() const noexcept override { return "NotAllowedValue"; }
    };

    OptionStringList(const ValueType & default_value);
    OptionStringList(const ValueType & default_value, std::string regex, bool icase);
    OptionStringList(const std::string & default_value);
    OptionStringList(const std::string & default_value, std::string regex, bool icase);
    OptionStringList * clone() const override;
    void test(const std::vector<std::string> & value) const;
    ValueType from_string(const std::string & value) const;
    virtual void set(Priority priority, const ValueType & value);
    void set(Priority priority, const std::string & value) override;
    const ValueType & get_value() const;
    const ValueType & get_default_value() const;
    std::string to_string(const ValueType & value) const;
    std::string get_value_string() const override;

protected:
    std::string regex;
    bool icase;
    ValueType default_value;
    ValueType value;
};

inline OptionStringList * OptionStringList::clone() const {
    return new OptionStringList(*this);
}

inline const OptionStringList::ValueType & OptionStringList::get_value() const {
    return value;
}

inline const OptionStringList::ValueType & OptionStringList::get_default_value() const {
    return default_value;
}

inline std::string OptionStringList::get_value_string() const {
    return to_string(value);
}

}  // namespace libdnf

#endif
