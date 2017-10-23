/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2013-2015 Richard Hughes <richard@hughsie.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef __DNF_GOAL_H
#define __DNF_GOAL_H

#include <glib.h>

#include "hy-goal.h"
#include "hy-package.h"

G_BEGIN_DECLS

gboolean         dnf_goal_depsolve                      (HyGoal          goal,
                                                         DnfGoalActions  flags,
                                                         GError          **error);
GPtrArray       *dnf_goal_get_packages                  (HyGoal          goal,
                                                         ...);
void             dnf_goal_add_protected                 (HyGoal goal,
                                                         DnfPackageSet  *pset);
void             dnf_goal_set_protected                 (HyGoal goal,
                                                         DnfPackageSet  *pset);

G_END_DECLS

#endif /* __DNF_GOAL_H */
