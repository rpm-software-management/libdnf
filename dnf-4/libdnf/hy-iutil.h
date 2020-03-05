/*
 * Copyright (C) 2012-2014 Red Hat, Inc.
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

#ifndef HY_IUTIL_H
#define HY_IUTIL_H

#include <regex.h>
#include <solv/bitmap.h>
#include <solv/queue.h>
#include <solv/repo.h>
#include <solv/rules.h>
#include <solv/transaction.h>

#include "dnf-sack.h"

#define CHKSUM_BYTES 32

G_BEGIN_DECLS

/* crypto utils */
int checksum_type2length(int type);

G_END_DECLS

#endif
