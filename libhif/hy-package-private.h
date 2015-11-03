/*
 * Copyright (C) 2012-2013 Red Hat, Inc.
 * Copyright (C) 2015 Richard Hughes <richard@hughsie.com>
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

#ifndef __HY_PACKAGE_INTERNAL_H
#define __HY_PACKAGE_INTERNAL_H

#include "hy-package.h"
#include "hif-sack.h"

HifPackage  *hif_package_clone          (HifPackage *pkg);
HifPackage  *hif_package_new            (HifSack    *sack, Id id);
Id           hif_package_get_id         (HifPackage *pkg);
Pool        *hif_package_get_pool       (HifPackage *pkg);
HifSack     *hif_package_get_sack       (HifPackage *pkg);
HifPackage  *hif_package_from_solvable  (HifSack    *sack, Solvable *s);

#endif // __HY_PACKAGE_INTERNAL_H
