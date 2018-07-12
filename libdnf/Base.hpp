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

#ifndef _LIBDNF_BASE_HPP_
#define _LIBDNF_BASE_HPP_

#include "logger.hpp"
#include <memory>

namespace libdnf {

struct Base {
    Base();
    ~Base();
    void setLogger(std::unique_ptr<Logger> logger);
    Logger * getLogger() noexcept;
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}

#endif // _LIBDNF_BASE_HPP_
