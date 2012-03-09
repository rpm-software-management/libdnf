#ifndef IUTIL_H
#define IUTIL_H

// libsolv
#include "queue.h"
#include "repo.h"
#include "rules.h"
#include "transaction.h"

// hawkey
#include "packagelist.h"
#include "sack.h"

int is_readable_rpm(const char *fn);
void repo_internalize_trigger(Repo *r);
Transaction *job2transaction(Sack sack, Queue *job, Queue *errors);
void queue2plist(Sack sack, Queue *q, PackageList plist);
char *problemruleinfo2str(Pool *pool, SolverRuleinfo type, Id source, Id target, Id dep);
Id what_updates(Pool *pool, Id p);

#endif
