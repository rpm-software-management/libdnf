/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2014 Richard Hughes <richard@hughsie.com>
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

#ifndef __LIBDNF_H
#define __LIBDNF_H

#define __LIBDNF_H_INSIDE__

#include <libdnf/dnf-advisory.h>
#include <libdnf/dnf-advisorypkg.h>
#include <libdnf/dnf-advisoryref.h>
#include <libdnf/dnf-conf.h>
#include <libdnf/dnf-context.h>
#include <libdnf/dnf-goal.h>
#include <libdnf/dnf-keyring.h>
#include <libdnf/dnf-lock.h>
#include <libdnf/dnf-package.h>
#include <libdnf/dnf-packagedelta.h>
#include <libdnf/dnf-repo-loader.h>
#include <libdnf/dnf-repo.h>
#include <libdnf/dnf-rpmts.h>
#include <libdnf/dnf-sack.h>
#include <libdnf/dnf-state.h>
#include <libdnf/dnf-transaction.h>
#include <libdnf/dnf-types.h>
#include <libdnf/dnf-utils.h>
#include <libdnf/dnf-version.h>

/* In progress conversion to dnf */
#include <libdnf/dnf-reldep-list.h>
#include <libdnf/dnf-reldep.h>
#include <libdnf/hy-goal.h>
#include <libdnf/hy-nevra.h>
#include <libdnf/hy-package.h>
#include <libdnf/hy-packageset.h>
#include <libdnf/hy-query.h>
#include <libdnf/hy-repo.h>
#include <libdnf/hy-selector.h>
#include <libdnf/hy-subject.h>
#include <libdnf/hy-util.h>

#undef __LIBDNF_H_INSIDE__

#endif /* __LIBDNF_H */
