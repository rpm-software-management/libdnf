#ifndef TESTSYS_H
#define TESTSYS_H

// libsolv
#include "src/packagelist.h"
#include "src/sack.h"

struct TestGlobals_s {
    char *repo_dir;
    Sack sack;
};

/* global data used to pass values from fixtures to tests */
extern struct TestGlobals_s test_globals;

void setup(void);
void setup_with_updates(void);
void setup_all(void);
void teardown(void);
void dump_packagelist(PackageList plist);

#endif /* TESTSYS_H */
