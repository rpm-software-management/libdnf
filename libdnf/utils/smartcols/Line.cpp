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

#include "Line.hpp"

std::shared_ptr<Line> Line::nextChild(struct libscols_iter *iter)
{
    struct libscols_line *scolsLine;
    scols_line_next_child(line, iter, &scolsLine);

    return std::make_shared<Line>(scolsLine);
}

void Line::referColumnData(Column *column, char *data)
{
    scols_line_refer_column_data(line, column->column, data);
}

std::shared_ptr<Cell> Line::getColumnCell(Column *column) const
{
    return std::make_shared<Cell>(scols_line_get_column_cell(line, column->column));
}

void Line::setColumnData(const std::shared_ptr<Column> &column, const std::string &data)
{
    setColumnData(column.get(), data);
}

void Line::setColumnData(Column *column, const std::string &data)
{
    scols_line_set_column_data(line, column->column, data.c_str());
}

std::shared_ptr<Cell> Line::getCell(size_t n) const
{
    if (n >= getCellCount()) {
        std::string message = "Out of bound, Index: " + n;
        message += " Size: " + getCellCount();
        throw std::out_of_range(message);
    }
    return std::make_shared<Cell>(scols_line_get_cell(line, n));
}

void Line::setUserdata(void *data)
{
    if (scols_line_set_userdata(line, data) == -EINVAL)
        throw std::runtime_error("Cannot set userdata");
}

void Line::setColor(const std::string &color)
{
    if (scols_line_set_color(line, color.c_str()) == -EINVAL)
        throw std::runtime_error("Cannot set color");
}

void Line::setData(size_t n, const std::string &data)
{
    if (scols_line_set_data(line, n, data.c_str()) == -EINVAL)
        throw std::runtime_error("Cannot set data");
}