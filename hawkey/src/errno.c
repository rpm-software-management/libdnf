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

#include <stdarg.h>
#include <stdio.h>

// hawkey
#include "errno_internal.h"

#define ERR_BUFFER_LENGTH 256

__thread int hy_errno = 0;
static __thread char hy_err_str[ERR_BUFFER_LENGTH];

const char *
format_err_str(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    vsnprintf(hy_err_str, ERR_BUFFER_LENGTH, format, args);
    va_end(args);
    return hy_err_str;
}

const char *
get_err_str(void)
{
    return hy_err_str;
}

/**
 * Get the Hawkey errno value.
 *
 * Recommended over direct variable access since that in a far-out theory could
 * change implementation.
 */
int
hy_get_errno(void)
{
    return hy_errno;
}
