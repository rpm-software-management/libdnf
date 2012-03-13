#ifndef TESTSYS_H
#define TESTSYS_H

struct TestGlobals_s {
    char *repo_dir;
};

/* global data used to pass values from fixtures to tests */
extern struct TestGlobals_s test_globals;

#endif /* TESTSYS_H */
