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

#include "Column.hpp"

size_t wrapNewlineChunksize(const struct libscols_column *column, const char *data, void *userdata) { return scols_wrapnl_chunksize(column, data, userdata); }
char *wrapNewlineNextchunk(const struct libscols_column *column, char *data, void *userdata) { return scols_wrapnl_nextchunk(column, data, userdata); }

std::shared_ptr<Cell> Column::getHeader() const
{
    return std::make_shared<Cell>(scols_column_get_header(column));
}

void Column::setCompareFunction(int (*function)(struct libscols_cell *, struct libscols_cell *, void *), void *data)
{
    scols_column_set_cmpfunc(column, function, data);
}

void Column::setWrapFunction(size_t (*wrapChunksize)(const struct libscols_column *, const char *, void *),
                             char *(*wrapNextchunk)(const struct libscols_column *, char *, void *), void *userdata)
{
    scols_column_set_wrapfunc(column, wrapChunksize, wrapNextchunk, userdata);
}

void Column::setWrap(bool enable)
{
    int formerFlags = scols_column_get_flags(column);
    int current = formerFlags & SCOLS_FL_WRAP;
    if (!current && enable)
        scols_column_set_flags(column, formerFlags | SCOLS_FL_WRAP);
    else if (current && !enable)
        scols_column_set_flags(column, formerFlags ^ SCOLS_FL_WRAP);
}

void Column::setColor(const std::string &color)
{
    if (scols_column_set_color(column, color.c_str()) == -EINVAL)
        throw std::runtime_error("Cannot set color");
}

void Column::setFlags(int flags)
{
    if (scols_column_set_flags(column, flags) == -EINVAL)
        throw std::runtime_error("Cannot set flags");
}

void Column::setSafechars(const std::string &safe)
{
    if (scols_column_set_safechars(column, safe.c_str()) == -EINVAL)
        throw std::runtime_error("Cannot set safechars");
}

void Column::setWidthHint(double hint)
{
    if (scols_column_set_whint(column, hint) == -EINVAL)
        throw std::runtime_error("Cannot set width hint");
}