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

#ifndef HY_ERRNO_H
#define HY_ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif

enum _hy_errors_e {
    HY_E_FAILED		= 1,	// general runtime error
    HY_E_OP,			// client programming error
    HY_E_LIBSOLV,		// error propagated from libsolv
    HY_E_IO,			// I/O error
    HY_E_CACHE_WRITE,		// cache write error
    HY_E_QUERY,			// ill-formed query
    HY_E_ARCH,			// unknown arch
    HY_E_VALIDATION,		// validation check failed
    HY_E_SELECTOR,		// ill-specified selector
    HY_E_NO_SOLUTION,		// goal found no solutions
    HY_E_NO_CAPABILITY,		// the capability was not available
};

extern __thread int hy_errno;

int hy_get_errno(void);

#ifdef __cplusplus
}
#endif

#endif /* HY_ERRNO_H */
