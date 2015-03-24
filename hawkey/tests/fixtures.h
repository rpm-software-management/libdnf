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

#ifndef FIXTURES_H
#define FIXTURES_H

// hawkey
#include "src/sack.h"

struct TestGlobals_s {
    char *repo_dir;
    HySack sack;
    char *tmpdir;
};

/* global data used to pass values from fixtures to tests */
extern struct TestGlobals_s test_globals;

void fixture_cmdline_only(void);
void fixture_empty(void);
void fixture_greedy_only(void);
void fixture_installonly(void);
void fixture_system_only(void);
void fixture_verify(void);
void fixture_with_change(void);
void fixture_with_cmdline(void);
void fixture_with_forcebest(void);
void fixture_with_main(void);
void fixture_with_updates(void);
void fixture_with_vendor(void);
void fixture_all(void);
void fixture_yum(void);
void fixture_reset(void);
void setup_yum_sack(HySack sack, const char *yum_repo_name);
void teardown(void);

#endif /* FIXTURES_H */
