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

#ifndef LIBDNF_LINE_HPP
#define LIBDNF_LINE_HPP

#include "Cell.hpp"
#include "Column.hpp"

#include <memory>
#include <libsmartcols/libsmartcols.h>

class Table;

class Line
{
    friend class Table;

public:
    explicit Line(struct libscols_line *line) : line(line) { scols_ref_line(line); }
    Line(const Line &line) { this->line = scols_copy_line(line.line); }
    ~Line() { scols_unref_line(line); }

    void allocCells(size_t n) { scols_line_alloc_cells(line, n); }
    void freeCells() { scols_line_free_cells(line); }

    void addChild(Line *child) { scols_line_add_child(line, child->line); }
    void removeChild(Line *child) { scols_line_remove_child(line, child->line); }
    std::shared_ptr<Line> nextChild(struct libscols_iter *iter);

    void referData(size_t n, char *data) { scols_line_refer_data(line, n, data); }
    void referColumnData(Column *column, char *data);

    bool hasChildren() const noexcept { return scols_line_has_children(line) == 1; }
    bool isAncestor(Line *parent) const noexcept { return scols_line_is_ancestor(line, parent->line) == 1; }

    size_t getCellCount() const noexcept { return scols_line_get_ncells(line); }
    const char *getColor() const { return scols_line_get_color(line); }
    std::shared_ptr<Cell> getCell(size_t n) const;
    std::shared_ptr<Cell> getColumnCell(const std::shared_ptr<Column> &column) const { return getColumnCell(column.get()); }
    std::shared_ptr<Cell> getColumnCell(Column *column) const;
    std::shared_ptr<Line> getParent() const { return std::make_shared<Line>(scols_line_get_parent(line)); }
    void *getUserdata() const { return scols_line_get_userdata(line); }

    void setUserdata(void *data);
    void setColor(const std::string &color);
    void setData(size_t n, const std::string &data);
    void setColumnData(const std::shared_ptr<Column> &column, const std::string &data);
    void setColumnData(Column *column, const std::string &data);


private:
    struct libscols_line *line;
};


#endif //LIBDNF_LINE_HPP
