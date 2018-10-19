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

#ifndef __ID_QUEUE_HPP
#define __ID_QUEUE_HPP

#include <utility>

extern "C" {
#include <solv/queue.h>
}

namespace libdnf {

struct IdQueue {
public:
    IdQueue();
    IdQueue(const IdQueue & src);
    IdQueue(IdQueue && src);
    explicit IdQueue(const Queue & src);
    ~IdQueue();

    void pushBack(Id id);
    void pushBack(Id id1, Id id2);
    Id operator[](int index) const;
    int * data() const noexcept;
    Queue * getQueue() noexcept;
    int size() const noexcept;
    void clear() noexcept;

private:
    Queue queue;

};

inline IdQueue::IdQueue() { queue_init(&queue); }
inline IdQueue::IdQueue(const IdQueue & src) { queue_init_clone(&queue, &src.queue); }
inline IdQueue::IdQueue(IdQueue && src)
{
    queue_init(&queue);
    std::swap(queue, src.queue);
}
inline IdQueue::IdQueue(const Queue & src) { queue_init_clone(&queue, &src); }

inline IdQueue::~IdQueue() { queue_free(&queue); }

inline void IdQueue::pushBack(Id id) { queue_push(&queue, id); }
inline void IdQueue::pushBack(Id id1, Id id2) { queue_push2(&queue, id1, id2); }
inline Id IdQueue::operator[](int index) const { return queue.elements[index]; }
inline int * IdQueue::data() const noexcept { return queue.elements; }
inline Queue * IdQueue::getQueue() noexcept { return &queue; }
inline int IdQueue::size() const noexcept { return queue.count; }
inline void IdQueue::clear() noexcept { queue_empty(&queue); }

}

#endif /* __ID_QUEUE_HPP */
