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

#ifndef TEST_SUITES_H
#define TEST_SUITES_H

#include <check.h>

Suite *advisory_suite(void);
Suite *advisorypkg_suite(void);
Suite *advisoryref_suite(void);
Suite *goal_suite(void);
Suite *iutil_suite(void);
Suite *package_suite(void);
Suite *packagelist_suite(void);
Suite *packageset_suite(void);
Suite *query_suite(void);
Suite *reldep_suite(void);
Suite *repo_suite(void);
Suite *sack_suite(void);
Suite *selector_suite(void);
Suite *subject_suite(void);
Suite *util_suite(void);

#endif // TEST_SUITES_H
