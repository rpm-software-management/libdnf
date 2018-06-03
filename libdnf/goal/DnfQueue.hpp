/*
 * Copyright (C) 2017 Red Hat, Inc.
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

#ifndef __DNF_QUEUE_HPP
#define __DNF_QUEUE_HPP

extern "C" {
#include <solv/queue.h>
}

namespace libdnf {

struct DnfQueue {
public:
    DnfQueue();
    ~DnfQueue();

    void pushBack(Id id);
    void pushBack(Id id1, Id id2);
    Id get(int index);
    int * getElements() const noexcept;
    const Queue * getQueue() const noexcept;
    int size() const noexcept;
    void clear() noexcept;

private:
    Queue queue;

};

inline DnfQueue::DnfQueue() { queue_init(&queue); }

inline DnfQueue::~DnfQueue() { queue_free(&queue); }

inline void DnfQueue::pushBack(Id id) { queue_push(&queue, id); }
inline void DnfQueue::pushBack(Id id1, Id id2) { queue_push2(&queue, id1, id2); }
inline Id DnfQueue::get(int index) { return queue.elements[index]; }
inline int * DnfQueue::getElements() const noexcept { return queue.elements; }
inline const Queue * DnfQueue::getQueue() const noexcept { return &queue; }
inline int DnfQueue::size() const noexcept { return queue.count; }
inline void DnfQueue::clear() noexcept { queue_empty(&queue); }


}

#endif /* __DNF_QUEUE_HPP */
