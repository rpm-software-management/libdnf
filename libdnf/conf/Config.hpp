/*
 * Copyright (C) 2018 Red Hat, Inc.
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef _LIBDNF_CONFIG_HPP
#define _LIBDNF_CONFIG_HPP

#include "Option.hpp"

#include <functional>
#include <map>

namespace libdnf {

class Config;

class OptionBinding {
public:
    typedef std::function<void(Option::Priority, const std::string &)> NewStringFunc;
    typedef std::function<const std::string & ()> GetValueStringFunc;

    OptionBinding(Config & config, Option & option, const std::string & name,
                  NewStringFunc && newString, GetValueStringFunc && getValueString);
    OptionBinding(Config & config, Option & option, const std::string & name);
    OptionBinding(const OptionBinding & src) = delete;

    OptionBinding & operator=(const OptionBinding & src) = delete;

    Option::Priority getPriority() const;
    void newString(Option::Priority priority, const std::string & value);
    std::string getValueString() const;

private:
    Option & option;
    NewStringFunc newStr;
    GetValueStringFunc getValueStr;
};

class OptionBinds {
public:
    struct Exception : public std::runtime_error {
        Exception(const std::string & what) : runtime_error(what) {}
    protected:
        mutable std::string tmpMsg;
    };
    struct OutOfRange : public Exception {
        OutOfRange(const std::string & id) : Exception(id) {}
        const char * what() const noexcept override;
    };
    struct AlreadyExists : public Exception {
        AlreadyExists(const std::string & id) : Exception(id) {}
        const char * what() const noexcept override;
    };

    typedef std::map<std::string, OptionBinding &> Container;
    typedef Container::iterator iterator;
    typedef Container::const_iterator const_iterator;

    OptionBinds(const OptionBinds &) = delete;
    OptionBinds & operator=(const OptionBinds &) = delete;

    OptionBinding & at(const std::string & id);
    const OptionBinding & at(const std::string & id) const;
    bool empty() const noexcept { return items.empty(); }
    std::size_t size() const noexcept { return items.size(); }
    iterator begin() noexcept { return items.begin(); }
    const_iterator begin() const noexcept { return items.begin(); }
    const_iterator cbegin() const noexcept { return items.cbegin(); }
    iterator end() noexcept { return items.end(); }
    const_iterator end() const noexcept { return items.end(); }
    const_iterator cend() const noexcept { return items.cend(); }
    iterator find(const std::string & id) { return items.find(id); }
    const_iterator find(const std::string & id) const { return items.find(id); }

private:
    friend class Config;
    friend class OptionBinding;
    OptionBinds() = default;
    iterator add(const std::string & id, OptionBinding & optBind);
    Container items;
};

class Config {
public:
    OptionBinds & optBinds() { return binds; }

private:
    OptionBinds binds;
};

}

#endif
