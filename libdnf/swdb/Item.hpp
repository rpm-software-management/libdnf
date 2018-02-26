/*
 * Copyright (C) 2017-2018 Red Hat, Inc.
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

#ifndef LIBDNF_SWDB_ITEM_HPP
#define LIBDNF_SWDB_ITEM_HPP

#include <memory>
#include <string>

#include "../utils/sqlite3/Sqlite3.hpp"

class Item;
typedef std::shared_ptr< Item > ItemPtr;

#include "SwdbTypes.hpp"

/// Class representing a package.
/**
 * Here goes the detailed description.
 * It can span multiple lines in the comment block.
 *
 * Paragraphs are delimited with blank lines.
 * Markdown is supported.
 */
class Item {
public:
    /// Default constructor.
    /** A more elaborate description of the constructor. */
    explicit Item(SQLite3Ptr conn);

    /// Default destructor.
    /** A more elaborate description of the destructor. */
    virtual ~Item() = default;

    /// Returns the ID of this item.
    /**
     * A more elaborate description of the getter.
     * \return the ID of this package
     */
    int64_t getId() const noexcept { return id; }

    /// Sets the ID of this item.
    /**
     * A more elaborate description of the setter.
     * \param value the ID to use as the new ID
     */
    void setId(int64_t value) { id = value; }

    virtual const ItemType getItemType() const noexcept { return itemType; }
    virtual std::string toStr();
    virtual void save();

protected:
    void dbInsert();

    SQLite3Ptr conn;
    int64_t id = 0;
    const ItemType itemType = ItemType::UNKNOWN;
};

#endif // LIBDNF_SWDB_ITEM_HPP
