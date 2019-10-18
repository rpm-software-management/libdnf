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

#include <logger.hpp>

#include <stdexcept>

namespace libdnf {

constexpr const char * Logger::levelCStr[];

void Logger::write(int source, Level level, const std::string & message)
{
    write(source, time(NULL), getpid(), level, message);
}

MemoryBufferLogger::MemoryBufferLogger(std::size_t maxItemsToKeep, std::size_t reserve)
:maxItems(maxItemsToKeep), firstItemIdx(0)
{
    if (reserve > 0)
        items.reserve(reserve < maxItemsToKeep ? reserve : maxItemsToKeep);
}

void MemoryBufferLogger::write(int source, time_t time, pid_t pid, libdnf::Logger::Level level, const std::string & message)
{
    std::lock_guard<std::mutex> guard(itemsMutex);
    if (items.size() < maxItems) {
        items.push_back({source, time, pid, level, message});
    } else {
        items[firstItemIdx] = {source, time, pid, level, message};
        if (++firstItemIdx >= maxItems)
            firstItemIdx = 0;
    }
}

MemoryBufferLogger::Item MemoryBufferLogger::getItem(std::size_t itemIdx) const
{
    if (itemIdx >= items.size() || itemIdx >= maxItems)
        throw std::out_of_range("MemoryBufferLogger");

    std::lock_guard<std::mutex> guard(itemsMutex);
    auto idx = itemIdx + firstItemIdx;
    if (idx >= maxItems)
        idx -= maxItems;
    return items[idx];
}

void MemoryBufferLogger::clear()
{
    std::lock_guard<std::mutex> guard(itemsMutex);
    firstItemIdx = 0;
    items.clear();
}

}
