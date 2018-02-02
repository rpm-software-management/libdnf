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

#include "Table.hpp"

Table::Table()
    : table(scols_new_table())
{
    if (!table) {
        throw std::runtime_error("Could not create table");
    }
}

void Table::setColumnSeparator(const std::string &separator)
{
    scols_table_set_column_separator(table, separator.c_str());
}

std::shared_ptr<Column> Table::getColumn(size_t n) const
{
    if (columns.size() < n) {
        std::string message = "Out of bounds. Index: " + n;
        message += " Size: " + columns.size();
        throw std::out_of_range(message);
    }
    return columns[n];
}

std::shared_ptr<Line> Table::getLine(size_t n) const
{
    if (lines.size() < n) {
        std::string message = "Out of bounds. Index: " + n;
        message += " Size: " + lines.size();
        throw std::out_of_range(message);
    }
    return lines[n];
}

void Table::moveColumn(const std::shared_ptr<Column> &before, const std::shared_ptr<Column> &toMove)
{
    scols_table_move_column(table, before->column, toMove->column);
}

std::shared_ptr<Column> Table::newColumn(const std::string &name, double widthHint, int flags)
{
    auto smartColsColumn = scols_table_new_column(table, name.c_str(), widthHint, flags);
    auto column = std::make_shared<Column>(smartColsColumn);

    columns.push_back(column);
    return column;
}

std::shared_ptr<Line> Table::newLine()
{
    auto smartColsLine = scols_table_new_line(table, nullptr);
    auto line = std::make_shared<Line>(smartColsLine);

    lines.push_back(line);
    return line;
}


std::shared_ptr<Line> Table::newLine(const std::shared_ptr<Line> &parent)
{
    auto smartColsLine = scols_table_new_line(table, parent->line);
    auto line = std::make_shared<Line>(smartColsLine);

    scols_unref_line(smartColsLine);
    lines.push_back(line);
    return line;
}

void Table::addColumn(const std::shared_ptr<Column> &column)
{
    scols_table_add_column(table, column->column);
    columns.push_back(column);
}

void Table::addLine(const std::shared_ptr<Line> &line)
{
    scols_table_add_line(table, line->line);
    lines.push_back(line);
}

void Table::removeColumn(const std::shared_ptr<Column> &column)
{
    std::remove(std::begin(columns), std::end(columns), column);
    scols_table_remove_column(table, column->column);
}

void Table::removeColumns()
{
    columns.clear();
    scols_table_remove_columns(table);
}

void Table::removeLine(const std::shared_ptr<Line> &line)
{
    std::remove(std::begin(lines), std::end(lines), line);
    scols_table_remove_line(table, line->line);
}

void Table::removeLines()
{
    lines.clear();
    scols_table_remove_lines(table);
}

std::string Table::toString()
{
    char *data;
    scols_print_table_to_string(table, &data);
    return std::string(data);
}

std::string Table::toString(const std::shared_ptr<Line> &start, const std::shared_ptr<Line> &end)
{
    char *data;
    if (start != nullptr && end != nullptr) {
        scols_table_print_range_to_string(table, start->line, end->line, &data);
        auto lines = std::string(data);
        free(data);
        return lines;
    } else {
        return std::string();
    }
}

void Table::setSymbols(struct libscols_symbols *symbols)
{
    if (scols_table_set_symbols(table, symbols) == -EINVAL)
        std::runtime_error("Cannot set stream");
}

void Table::setDefaultSymbols()
{
    if (scols_table_set_default_symbols(table) == -EINVAL)
        std::runtime_error("Cannot set default symbols");
}

void Table::setStream(FILE *stream)
{
    if (scols_table_set_stream(table, stream) == -EINVAL)
        std::runtime_error("Cannot set stream");
}

void Table::setTermforce(TermForce force)
{
    if (scols_table_set_termforce(table, static_cast<int>(force)) == -EINVAL)
        std::runtime_error("Cannot set default symbols");
}
