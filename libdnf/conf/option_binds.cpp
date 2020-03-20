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

#include "libdnf/conf/option_binds.hpp"

#include <utility>

namespace libdnf {

// ========== OptionBinds::Item class ===============

OptionBinds::Item::Item(Option & option, NewStringFunc new_string, GetValueStringFunc get_value_string, bool add_value)
    : option(&option)
    , new_str(std::move(new_string))
    , get_value_str(std::move(get_value_string))
    , add_value(add_value) {}

OptionBinds::Item::Item(Option & option) : option(&option) {}

Option::Priority OptionBinds::Item::get_priority() const {
    return option->get_priority();
}

void OptionBinds::Item::new_string(Option::Priority priority, const std::string & value) {
    if (new_str) {
        new_str(priority, value);
    } else {
        option->set(priority, value);
    }
}

std::string OptionBinds::Item::get_value_string() const {
    if (get_value_str) {
        return get_value_str();
    }
    return option->get_value_string();
}

bool OptionBinds::Item::get_add_value() const {
    return add_value;
}


// =========== OptionBinds class ===============

OptionBinds::Item & OptionBinds::at(const std::string & id) {
    auto item = items.find(id);
    if (item == items.end()) {
        throw OptionNotFound(id);
    }
    return item->second;
}

const OptionBinds::Item & OptionBinds::at(const std::string & id) const {
    auto item = items.find(id);
    if (item == items.end()) {
        throw OptionNotFound(id);
    }
    return item->second;
}

OptionBinds::Item & OptionBinds::add(
    const std::string & id,
    Option & option,
    Item::NewStringFunc new_string,
    Item::GetValueStringFunc get_value_string,
    bool add_value) {
    auto item = items.find(id);
    if (item != items.end()) {
        throw OptionAlreadyExists(id);
    }
    auto res = items.emplace(id, Item(option, std::move(new_string), std::move(get_value_string), add_value));
    return res.first->second;
}

OptionBinds::Item & OptionBinds::add(const std::string & id, Option & option) {
    auto item = items.find(id);
    if (item != items.end()) {
        throw OptionAlreadyExists(id);
    }
    auto res = items.emplace(id, Item(option));
    return res.first->second;
}

}  // namespace libdnf
