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

#ifndef LIBDNF_CONF_OPTION_BOOL_HPP
#define LIBDNF_CONF_OPTION_BOOL_HPP


#include "option.hpp"

#include <array>
#include <memory>
#include <vector>

namespace libdnf {


class OptionBool : public Option {
public:
    using ValueType = bool;

    class InvalidValue : public Option::InvalidValue {
    public:
        using Option::InvalidValue::InvalidValue;
        const char * get_domain_name() const noexcept override { return "libdnf::OptionBool"; }
    };

    explicit OptionBool(bool default_value);
    OptionBool(
        bool default_value, const std::vector<std::string> & true_vals, const std::vector<std::string> & false_vals);
    OptionBool(const OptionBool & src);
    OptionBool(OptionBool && src) noexcept = default;
    ~OptionBool() override = default;

    OptionBool & operator=(const OptionBool & src);
    OptionBool & operator=(OptionBool && src) noexcept = default;

    OptionBool * clone() const override;
    void test(bool /*unused*/) const;
    bool from_string(const std::string & value) const;
    void set(Priority priority, bool value);
    void set(Priority priority, const std::string & value) override;
    bool get_value() const noexcept;
    bool get_default_value() const noexcept;
    std::string to_string(bool value) const;
    std::string get_value_string() const override;
    static const std::vector<std::string> & get_default_true_values() noexcept;
    static const std::vector<std::string> & get_default_false_values() noexcept;
    const std::vector<std::string> & get_true_values() const noexcept;
    const std::vector<std::string> & get_false_values() const noexcept;

private:
    std::unique_ptr<std::vector<std::string>> true_values;
    std::unique_ptr<std::vector<std::string>> false_values;
    bool default_value;
    bool value;
};


inline OptionBool * OptionBool::clone() const {
    return new OptionBool(*this);
}

inline void OptionBool::test(bool /*unused*/) const {}

inline bool OptionBool::get_value() const noexcept {
    return value;
}

inline bool OptionBool::get_default_value() const noexcept {
    return default_value;
}

inline std::string OptionBool::get_value_string() const {
    return to_string(value);
}

inline const std::vector<std::string> & OptionBool::get_default_true_values() noexcept {
    static std::vector<std::string> true_values = {"1", "yes", "true", "on"};
    return true_values;
}

inline const std::vector<std::string> & OptionBool::get_default_false_values() noexcept {
    static std::vector<std::string> false_values = {"0", "no", "false", "off"};
    return false_values;
}

inline const std::vector<std::string> & OptionBool::get_true_values() const noexcept {
    return true_values ? *true_values : get_default_true_values();
}

inline const std::vector<std::string> & OptionBool::get_false_values() const noexcept {
    return false_values ? *false_values : get_default_false_values();
}

}  // namespace libdnf

#endif
