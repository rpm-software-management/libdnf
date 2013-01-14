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

void fixture_empty(void);
void fixture_greedy_only(void);
void fixture_system_only(void);
void fixture_with_main(void);
void fixture_with_updates(void);
void fixture_with_vendor(void);
void fixture_all(void);
void fixture_yum(void);
void fixture_reset(void);
void setup_yum_sack(HySack sack, const char *yum_repo_name);
void teardown(void);

#endif /* FIXTURES_H */
