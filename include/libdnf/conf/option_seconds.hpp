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

#ifndef LIBDNF_CONF_OPTION_SECONDS_HPP
#define LIBDNF_CONF_OPTION_SECONDS_HPP

#include "option_number.hpp"

namespace libdnf {

/**
* @class OptionSeconds
*
* @brief An option representing an integer value of seconds.
*
* Valid inputs: 100, 1.5m, 90s, 1.2d, 1d, 0xF, 0.1, -1, never.
* Invalid inputs: -10, -0.1, 45.6Z, 1d6h, 1day, 1y.
*/
class OptionSeconds : public OptionNumber<std::int32_t> {
public:
    class InvalidValue : public Option::InvalidValue {
    public:
        using Option::InvalidValue::InvalidValue;
        const char * get_domain_name() const noexcept override { return "libdnf::OptionSeconds"; }
    };

    class NegativeValue : public InvalidValue {
    public:
        using InvalidValue::InvalidValue;
        const char * get_name() const noexcept override { return "Negative value"; }
        const char * get_description() const noexcept override { return "Seconds value must not be negative"; };
    };

    class UnknownUnit : public InvalidValue {
    public:
        using InvalidValue::InvalidValue;
        const char * get_name() const noexcept override { return "UnknownUnit"; }
    };

    OptionSeconds(ValueType default_value, ValueType min, ValueType max);
    OptionSeconds(ValueType default_value, ValueType min);
    explicit OptionSeconds(ValueType default_value);
    OptionSeconds * clone() const override;
    ValueType from_string(const std::string & value) const;
    using OptionNumber<std::int32_t>::set;
    void set(Priority priority, const std::string & value) override;
};

inline OptionSeconds * OptionSeconds::clone() const {
    return new OptionSeconds(*this);
}

}  // namespace libdnf

#endif
