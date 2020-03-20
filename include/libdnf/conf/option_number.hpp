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

#ifndef LIBDNF_CONF_OPTION_NUMBER_HPP
#define LIBDNF_CONF_OPTION_NUMBER_HPP

#include "option.hpp"

#include <functional>

namespace libdnf {

template <typename T>
class OptionNumber : public Option {
public:
    using ValueType = T;
    using FromStringFunc = std::function<ValueType(const std::string &)>;

    class InvalidValue : public Option::InvalidValue {
    public:
        using Option::InvalidValue::InvalidValue;
        const char * get_domain_name() const noexcept override { return "libdnf::OptionNumber"; }
    };

    class NotAllowedValue : public InvalidValue {
    public:
        using InvalidValue::InvalidValue;
        const char * get_name() const noexcept override { return "NotAllowedValue"; }
    };

    OptionNumber(T default_value, T min, T max);
    OptionNumber(T default_value, T min);
    explicit OptionNumber(T default_value);
    OptionNumber(T default_value, T min, T max, FromStringFunc && from_string_func);
    OptionNumber(T default_value, T min, FromStringFunc && from_string_func);
    OptionNumber(T default_value, FromStringFunc && from_string_func);
    OptionNumber * clone() const override;
    void test(ValueType value) const;
    T from_string(const std::string & value) const;
    void set(Priority priority, ValueType value);
    void set(Priority priority, const std::string & value) override;
    T get_value() const;
    T get_default_value() const;
    std::string to_string(ValueType value) const;
    std::string get_value_string() const override;

private:
    FromStringFunc from_string_user;
    ValueType default_value;
    ValueType min;
    ValueType max;
    ValueType value;
};

template <typename T>
inline OptionNumber<T> * OptionNumber<T>::clone() const {
    return new OptionNumber<T>(*this);
}

template <typename T>
inline T OptionNumber<T>::get_value() const {
    return value;
}

template <typename T>
inline T OptionNumber<T>::get_default_value() const {
    return default_value;
}

template <typename T>
inline std::string OptionNumber<T>::get_value_string() const {
    return to_string(value);
}

extern template class OptionNumber<std::int32_t>;
extern template class OptionNumber<std::uint32_t>;
extern template class OptionNumber<std::int64_t>;
extern template class OptionNumber<std::uint64_t>;
extern template class OptionNumber<float>;

}  // namespace libdnf

#endif
