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

#ifndef LIBDNF_COLUMN_HPP
#define LIBDNF_COLUMN_HPP

#include "Cell.hpp"

#include <memory>
#include <libsmartcols/libsmartcols.h>
#include <cerrno>

size_t wrapNewlineChunksize(const struct libscols_column *column, const char *data, void *userdata);
char *wrapNewlineNextchunk(const struct libscols_column *column, char *data, void *userdata);

class Line;
class Table;

class Column
{
    friend class Line;
    friend class Table;

public:
    explicit Column(struct libscols_column *column) : column(column) { scols_ref_column(column); }
    Column(Column &column) { this->column = scols_copy_column(column.column); }
    ~Column() { scols_unref_column(column); }

    bool isTree() const noexcept { return scols_column_is_tree(column) == 1; }
    bool isTrunc() const noexcept { return scols_column_is_trunc(column) == 1; }
    bool isRight() const noexcept { return scols_column_is_right(column) == 1; }
    bool isStrictWidth() const noexcept { return scols_column_is_strict_width(column) == 1; }
    bool isHidden() const noexcept { return scols_column_is_hidden(column) == 1; }
    bool isNoextremes() const noexcept { return scols_column_is_noextremes(column) == 1; }
    bool isWrap() const noexcept { return scols_column_is_wrap(column) == 1; }
    bool isCustomwrap() const noexcept { return scols_column_is_customwrap(column) == 1; }

    int getFlags() const noexcept { return scols_column_get_flags(column); }
    double getWidthHint() const { return scols_column_get_whint(column); }
    size_t getWidth() const noexcept { return scols_column_get_width(column); }
    std::string getSafechars() const noexcept { return scols_column_get_safechars(column); }
    std::string getColor() const noexcept { return scols_column_get_color(column); }
    std::shared_ptr<Cell> getHeader() const;

    void setWrap(bool enable);
    void setColor(const std::string &color);
    void setFlags(int flags);
    void setSafechars(const std::string &safe);
    void setWidthHint(double hint);


    void setCompareFunction(int (*function)(struct libscols_cell *a, struct libscols_cell *b, void *), void *data);
    void setWrapFunction(size_t (*wrapChunksize)(const struct libscols_column *, const char *, void *),
                         char *(*wrapNextchunk)(const struct libscols_column *, char *, void *),
                         void *userdata);

    void setNewlineWrapFunction() { setWrapFunction(wrapNewlineChunksize, wrapNewlineNextchunk, nullptr); }

private:
    struct libscols_column *column;
};


#endif //LIBDNF_COLUMN_HPP
