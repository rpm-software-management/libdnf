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

#ifndef LIBDNF_CONF_OPTION_CHILD_HPP
#define LIBDNF_CONF_OPTION_CHILD_HPP

#include "option.hpp"

namespace libdnf {

template <class ParentOptionType, class Enable = void>
class OptionChild : public Option {
public:
    explicit OptionChild(const ParentOptionType & parent);
    OptionChild * clone() const override;
    Priority get_priority() const override;
    void set(Priority priority, const typename ParentOptionType::ValueType & value);
    void set(Priority priority, const std::string & value) override;
    typename ParentOptionType::ValueType get_value() const;
    typename ParentOptionType::ValueType get_default_value() const;
    std::string get_value_string() const override;
    bool empty() const noexcept override;

private:
    const ParentOptionType * parent;
    typename ParentOptionType::ValueType value;
};

template <class ParentOptionType>
class OptionChild<
    ParentOptionType,
    typename std::enable_if<std::is_same<typename ParentOptionType::ValueType, std::string>::value>::type>
    : public Option {
public:
    explicit OptionChild(const ParentOptionType & parent);
    OptionChild * clone() const override;
    Priority get_priority() const override;
    void set(Priority priority, const std::string & value) override;
    const std::string & get_value() const;
    const std::string & get_default_value() const;
    std::string get_value_string() const override;
    bool empty() const noexcept override;

private:
    const ParentOptionType * parent;
    std::string value;
};

template <class ParentOptionType, class Enable>
inline OptionChild<ParentOptionType, Enable>::OptionChild(const ParentOptionType & parent) : parent(&parent) {}

template <class ParentOptionType, class Enable>
inline OptionChild<ParentOptionType, Enable> * OptionChild<ParentOptionType, Enable>::clone() const {
    return new OptionChild<ParentOptionType>(*this);
}

template <class ParentOptionType, class Enable>
inline Option::Priority OptionChild<ParentOptionType, Enable>::get_priority() const {
    return Option::get_priority() != Priority::EMPTY ? Option::get_priority() : parent->get_priority();
}

template <class ParentOptionType, class Enable>
inline void OptionChild<ParentOptionType, Enable>::set(
    Priority priority, const typename ParentOptionType::ValueType & value) {
    if (priority >= Option::get_priority()) {
        parent->test(value);
        set_priority(priority);
        this->value = value;
    }
}

template <class ParentOptionType, class Enable>
inline void OptionChild<ParentOptionType, Enable>::set(Priority priority, const std::string & value) {
    if (priority >= Option::get_priority()) {
        set(priority, parent->from_string(value));
    }
}

template <class ParentOptionType, class Enable>
inline typename ParentOptionType::ValueType OptionChild<ParentOptionType, Enable>::get_value() const {
    return Option::get_priority() != Priority::EMPTY ? value : parent->get_value();
}

template <class ParentOptionType, class Enable>
inline typename ParentOptionType::ValueType OptionChild<ParentOptionType, Enable>::get_default_value() const {
    return parent->getDefaultValue();
}

template <class ParentOptionType, class Enable>
inline std::string OptionChild<ParentOptionType, Enable>::get_value_string() const {
    return Option::get_priority() != Priority::EMPTY ? parent->to_string(value) : parent->get_value_string();
}

template <class ParentOptionType, class Enable>
inline bool OptionChild<ParentOptionType, Enable>::empty() const noexcept {
    return Option::get_priority() == Priority::EMPTY && parent->empty();
}

template <class ParentOptionType>
inline OptionChild<
    ParentOptionType,
    typename std::enable_if<std::is_same<typename ParentOptionType::ValueType, std::string>::value>::type>::
    OptionChild(const ParentOptionType & parent)
    : parent(&parent) {}

template <class ParentOptionType>
inline OptionChild<
    ParentOptionType,
    typename std::enable_if<std::is_same<typename ParentOptionType::ValueType, std::string>::value>::type> *
OptionChild<
    ParentOptionType,
    typename std::enable_if<std::is_same<typename ParentOptionType::ValueType, std::string>::value>::type>::clone()
    const {
    return new OptionChild<ParentOptionType>(*this);
}

template <class ParentOptionType>
inline Option::Priority OptionChild<
    ParentOptionType,
    typename std::enable_if<std::is_same<typename ParentOptionType::ValueType, std::string>::value>::type>::
    get_priority() const {
    return Option::get_priority() != Priority::EMPTY ? Option::get_priority() : parent->get_priority();
}

template <class ParentOptionType>
inline void OptionChild<
    ParentOptionType,
    typename std::enable_if<std::is_same<typename ParentOptionType::ValueType, std::string>::value>::type>::
    set(Priority priority, const std::string & value) {
    auto val = parent->from_string(value);
    if (priority >= Option::get_priority()) {
        parent->test(val);
        set_priority(priority);
        this->value = val;
    }
}

template <class ParentOptionType>
inline const std::string & OptionChild<
    ParentOptionType,
    typename std::enable_if<std::is_same<typename ParentOptionType::ValueType, std::string>::value>::type>::get_value()
    const {
    return Option::get_priority() != Priority::EMPTY ? value : parent->get_value();
}

template <class ParentOptionType>
inline const std::string & OptionChild<
    ParentOptionType,
    typename std::enable_if<std::is_same<typename ParentOptionType::ValueType, std::string>::value>::type>::
    get_default_value() const {
    return parent->getDefaultValue();
}

template <class ParentOptionType>
inline std::string OptionChild<
    ParentOptionType,
    typename std::enable_if<std::is_same<typename ParentOptionType::ValueType, std::string>::value>::type>::
    get_value_string() const {
    return Option::get_priority() != Priority::EMPTY ? value : parent->get_value();
}

template <class ParentOptionType>
inline bool OptionChild<
    ParentOptionType,
    typename std::enable_if<std::is_same<typename ParentOptionType::ValueType, std::string>::value>::type>::empty()
    const noexcept {
    return Option::get_priority() == Priority::EMPTY && parent->empty();
}

}  // namespace libdnf

#endif
