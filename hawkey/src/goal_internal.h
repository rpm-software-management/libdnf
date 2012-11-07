#ifndef HY_GOAL_INTERNAL_H
#define HY_GOAL_INTERNAL_H

// libsolv
#include <solv/queue.h>

// hawkey
#include "goal.h"

int sltr2job(const HySelector sltr, Queue *job, int solver_action);

#endif // HY_GOAL_INTERNAL_H
