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

#ifndef LIBDNF_CELL_HPP
#define LIBDNF_CELL_HPP

#include <libsmartcols/libsmartcols.h>
#include <cerrno>
#include <string>
#include <stdexcept>

class Cell
{
public:
    explicit Cell(struct libscols_cell *cell) : cell(cell) {}

    void reset() { scols_reset_cell(cell); }

    int getFlags() const noexcept { return scols_cell_get_flags(cell); }
    int getAlignment() const noexcept { return scols_cell_get_alignment(cell); }
    const char *getData() const { return scols_cell_get_data(cell); }
    const char *getColor() const noexcept { return scols_cell_get_color(cell); }
    void *getUserdata() const { return scols_cell_get_userdata(cell); }

    void setData(const std::string &data);
    void setColor(const std::string &color);
    void setFlags(int flags);
    void setUserdata(void *data);

    bool operator==(const Cell &cell) const { return scols_cmpstr_cells(this->cell, cell.cell, nullptr) == 0; }

private:
    struct libscols_cell *cell;
};

inline void Cell::setData(const std::string &data)
{
    if (scols_cell_set_data(cell, data.c_str()) == -EINVAL)
        throw std::runtime_error("Cannot set data");
}

inline void Cell::setColor(const std::string &color)
{
    if (scols_cell_set_color(cell, color.c_str()) == -EINVAL)
        throw std::runtime_error("Cannot set color");
}

inline void Cell::setFlags(int flags)
{
    if (scols_cell_set_flags(cell, flags) == -EINVAL)
        throw std::runtime_error("Cannot set flags");
}

inline void Cell::setUserdata(void *data)
{
    if (scols_cell_set_userdata(cell, data) == -EINVAL)
        throw std::runtime_error("Cannot set userdata");
}

#endif //LIBDNF_CELL_HPP
