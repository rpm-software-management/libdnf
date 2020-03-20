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

#ifndef LIBDNF_CONF_OPTION_ENUM_HPP
#define LIBDNF_CONF_OPTION_ENUM_HPP

#include "option.hpp"

#include <functional>
#include <vector>

namespace libdnf {

template <typename T>
class OptionEnum : public Option {
public:
    using ValueType = T;
    using FromStringFunc = std::function<ValueType(const std::string &)>;

    class InvalidValue : public Option::InvalidValue {
    public:
        using Option::InvalidValue::InvalidValue;
        const char * get_domain_name() const noexcept override { return "libdnf::OptionEnum"; }
    };

    class NotAllowedValue : public InvalidValue {
    public:
        using InvalidValue::InvalidValue;
        const char * get_name() const noexcept override { return "NotAllowedValue"; }
    };

    OptionEnum(ValueType default_value, const std::vector<ValueType> & enum_vals);
    OptionEnum(ValueType default_value, std::vector<ValueType> && enum_vals);
    OptionEnum(ValueType default_value, const std::vector<ValueType> & enum_vals, FromStringFunc && from_string_func);
    OptionEnum(ValueType default_value, std::vector<ValueType> && enum_vals, FromStringFunc && from_string_func);
    OptionEnum * clone() const override;
    void test(ValueType value) const;
    ValueType from_string(const std::string & value) const;
    void set(Priority priority, ValueType value);
    void set(Priority priority, const std::string & value) override;
    T get_value() const;
    T get_default_value() const;
    std::string to_string(ValueType value) const;
    std::string get_value_string() const override;

private:
    FromStringFunc from_string_user;
    std::vector<ValueType> enum_vals;
    ValueType default_value;
    ValueType value;
};

template <>
class OptionEnum<std::string> : public Option {
public:
    using ValueType = std::string;
    using FromStringFunc = std::function<ValueType(const std::string &)>;

    class InvalidValue : public Option::InvalidValue {
    public:
        using Option::InvalidValue::InvalidValue;
        const char * get_domain_name() const noexcept override { return "libdnf::OptionEnum"; }
    };

    class NotAllowedValue : public InvalidValue {
    public:
        using InvalidValue::InvalidValue;
        const char * get_name() const noexcept override { return "NotAllowedValue"; }
    };

    OptionEnum(const std::string & default_value, std::vector<ValueType> enum_vals);
    OptionEnum(const std::string & default_value, std::vector<ValueType> enum_vals, FromStringFunc && from_string_func);
    OptionEnum * clone() const override;
    void test(const std::string & value) const;
    std::string from_string(const std::string & value) const;
    void set(Priority priority, const std::string & value) override;
    const std::string & get_value() const;
    const std::string & get_default_value() const;
    std::string get_value_string() const override;

private:
    FromStringFunc from_string_user;
    std::vector<ValueType> enum_vals;
    ValueType default_value;
    ValueType value;
};

template <typename T>
inline OptionEnum<T> * OptionEnum<T>::clone() const {
    return new OptionEnum<T>(*this);
}

inline OptionEnum<std::string> * OptionEnum<std::string>::clone() const {
    return new OptionEnum<std::string>(*this);
}

inline const std::string & OptionEnum<std::string>::get_value() const {
    return value;
}

inline const std::string & OptionEnum<std::string>::get_default_value() const {
    return default_value;
}

inline std::string OptionEnum<std::string>::get_value_string() const {
    return value;
}

}  // namespace libdnf

#endif
