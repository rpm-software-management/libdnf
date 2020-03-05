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

#ifndef LIBDNF_TABLE_HPP
#define LIBDNF_TABLE_HPP

#include "Cell.hpp"
#include "Column.hpp"
#include "Line.hpp"

#include "../tinyformat/tinyformat.hpp"

#include <memory>
#include <libsmartcols/libsmartcols.h>
#include <string>
#include <vector>

class Table
{
public:
    enum class TermForce {
        AUTO = 0,
        NEVER,
        ALWAYS
    };

    Table();
    explicit Table(struct libscols_table *table) : table(table) { scols_ref_table(this->table); }
    Table(const Table &table) { this->table = scols_copy_table(table.table); }
    ~Table() { scols_unref_table(table); }

    void setName(const std::string &name) { scols_table_set_name(table, name.c_str()); }
    void setColumnSeparator(const std::string &separator);
    void setLineSeparator(const std::string &separator) { scols_table_set_line_separator(table, separator.c_str()); }
    void setSymbols(struct libscols_symbols *symbols);
    void setDefaultSymbols();
    void setStream(FILE *stream);
    void setTermforce(TermForce force);
    void setTermwidth(size_t width) { scols_table_set_termwidth(table, width); }

    const char *getName() const noexcept { return scols_table_get_name(table); }
    const char *getColumnSeparator() const { return scols_table_get_column_separator(table); }
    const char *getLineSeparator() const { return scols_table_get_line_separator(table); }
    size_t getNumberOfColumns() const noexcept { return scols_table_get_ncols(table); }
    size_t getNumberOfLines() const noexcept { return scols_table_get_nlines(table); }
    std::shared_ptr<Cell> getTitle() const { return std::make_shared<Cell>(scols_table_get_title(table)); }
    std::shared_ptr<Column> getColumn(size_t n) const;
    std::shared_ptr<Line> getLine(size_t n) const;
    TermForce getTermforce() const { return static_cast<TermForce>(scols_table_get_termforce(table)); }

    struct libscols_symbols *getSymbols() const { return scols_table_get_symbols(table); }
    size_t getTermwidth() const { return scols_table_get_termwidth(table); }
    FILE *getStream() const { return scols_table_get_stream(table); }

    bool isColorsEnabled() const noexcept { return scols_table_colors_wanted(table) == 1; }
    bool isRaw() const noexcept { return scols_table_is_raw(table) == 1; }
    bool isAscii() const noexcept { return scols_table_is_ascii(table) == 1; }
    bool isJson() const noexcept { return scols_table_is_json(table) == 1; }
    bool isNoheadings() const noexcept { return scols_table_is_noheadings(table) == 1; }
    bool isEmpty() const noexcept { return scols_table_is_empty(table) == 1; }
    bool isExport() const noexcept { return scols_table_is_export(table) == 1; }
    bool isMaxout() const noexcept { return scols_table_is_maxout(table) == 1; }
    bool isNowrap() const noexcept { return scols_table_is_nowrap(table) == 1; }
    bool isNoLineSeparator() const noexcept { return scols_table_is_nolinesep(table) == 1; }
    bool isTree() const noexcept { return scols_table_is_tree(table) == 1; }

    void enableColors(bool enable) { scols_table_enable_colors(table, enable); }
    void enableRaw(bool enable) { scols_table_enable_raw(table, enable); }
    void enableAscii(bool enable) { scols_table_enable_ascii(table, enable); }
    void enableJson(bool enable) { scols_table_enable_json(table, enable); }
    void enableNoheadings(bool enable) { scols_table_enable_noheadings(table, enable); }
    void enableExport(bool enable) { scols_table_enable_export(table, enable); }
    void enableMaxout(bool enable) { scols_table_enable_maxout(table, enable); }
    void enableNowrap(bool enable) { scols_table_enable_nowrap(table, enable); }
    void enableNolinesep(bool enable) { scols_table_enable_nolinesep(table, enable); }

    void addColumn(const std::shared_ptr<Column> &column);
    void removeColumn(const std::shared_ptr<Column> &column);
    void removeColumns();
    void moveColumn(const std::shared_ptr<Column> &before, const std::shared_ptr<Column> &toMove);
    std::shared_ptr<Column> newColumn(const std::string &name, double widthHint = 0, int flags = 0);
    std::shared_ptr<Column> nextColumn(std::vector<std::shared_ptr<Column>>::iterator &iterator) { return *(iterator++); }

    void addLine(const std::shared_ptr<Line> &line);
    void removeLine(const std::shared_ptr<Line> &line);
    void removeLines();
    std::shared_ptr<Line> newLine();
    std::shared_ptr<Line> newLine(const std::shared_ptr<Line> &parent);
    std::shared_ptr<Line> nextLine(std::vector<std::shared_ptr<Line>>::iterator &iterator) { return *(iterator++); }

    void reduceTermwidth(size_t reduce) { scols_table_reduce_termwidth(table, reduce); }

    void sort(const std::shared_ptr<Column> &column) { scols_sort_table(table, column->column); }
    void sortByTree() { scols_sort_table_by_tree(table); }

    void print() { tinyformat::printf("%s\n", toString()); }
    void print(const std::shared_ptr<Line> &start, const std::shared_ptr<Line> &end) { tinyformat::printf("%s\n", toString(start, end)); }

    std::string toString();
    std::string toString(const std::shared_ptr<Line> &start, const std::shared_ptr<Line> &end);

private:
    struct libscols_table *table;

    std::vector<std::shared_ptr<Line>> lines;
    std::vector<std::shared_ptr<Column>> columns;
};

#endif //LIBDNF_TABLE_HPP
