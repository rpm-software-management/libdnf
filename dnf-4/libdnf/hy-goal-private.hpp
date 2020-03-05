/*
 * Copyright (C) 2012-2013 Red Hat, Inc.
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

#ifndef HY_GOAL_INTERNAL_H
#define HY_GOAL_INTERNAL_H

// libsolv
#include <solv/queue.h>
#include <solv/transaction.h>
#include <solv/solver.h>

// hawkey
#include "hy-goal.h"

inline DnfGoalActions operator|(DnfGoalActions a, DnfGoalActions b)
{
    return static_cast<DnfGoalActions>(static_cast<int>(a) | static_cast<int>(b));
}

inline DnfGoalActions & operator|=(DnfGoalActions & a, DnfGoalActions b)
{
    return a = a | b;
}

namespace libdnf {

void sltrToJob(const HySelector sltr, Queue *job, int solver_action);

}

#endif // HY_GOAL_INTERNAL_H
