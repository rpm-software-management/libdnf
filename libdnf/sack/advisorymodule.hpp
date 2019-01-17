/*
 * Copyright (C) 2019 Red Hat, Inc.
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


#ifndef __ADVISORY_MODULE_HPP
#define __ADVISORY_MODULE_HPP

#include "../dnf-types.h"
#include "advisory.hpp"

#include <memory>

#include <solv/pooltypes.h>


namespace libdnf {

struct AdvisoryModule {
public:
    AdvisoryModule(DnfSack *sack, Id advisory, Id name, Id stream, Id version, Id context, Id arch);
    AdvisoryModule(const AdvisoryModule & src);
    AdvisoryModule(AdvisoryModule && src);
    ~AdvisoryModule();
    AdvisoryModule & operator=(const AdvisoryModule & src);
    AdvisoryModule & operator=(AdvisoryModule && src) noexcept;
    bool nsvcaEQ(AdvisoryModule & other);
    Advisory * getAdvisory() const;
    const char * getName() const;
    const char * getStream() const;
    const char * getVersion() const;
    const char * getContext() const;
    const char * getArch() const;
    DnfSack * getSack();
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

}

#endif /* __ADVISORY_MODULE_HPP */
