#ifndef QUERY_H
#define QUERY_H

// hawkey
#include "sack.h"
#include "packagelist.h"

enum _filter_type_e {
    FT_EQ	= (1 << 0),
    FT_LT	= (1 << 1),
    FT_GT	= (1 << 2),
    FT_SUBSTR	= (1 << 3),
};

enum _key_name_e {
    KN_PKG_NAME,
    KN_PKG_ARCH,
    KN_PKG_SUMMARY,
    KN_PKG_REPO,
    KN_PKG_PROVIDES,
    KN_PKG_LATEST,
    KN_PKG_UPDATES,
    KN_PKG_OBSOLETING
};

struct _Filter {
    int filter_type;
    int keyname;
    char *match;
    char *evr;
};

struct _Query {
    Sack sack;
    struct _Filter *filters;
    int nfilters;
    int updates; /* 1 for "only updates for installed packages" */
    int latest; /* 1 for "only the latest version per arch" */
    int obsoleting; /* 1 for "only those obsoleting installed packages" */
};

typedef struct _Query * Query;

Query query_create(Sack sack);
void query_free(Query q);
void query_filter(Query q, int keyname, int filter_type, const char *match);
void query_filter_provides(Query q, int filter_type, const char *name,
			   const char *evr);
void query_filter_updates(Query q, int val);
void query_filter_latest(Query q, int val);
void query_filter_obsoleting(Query q, int val);

PackageList query_run(Query q);

// internal/deprecated

PackageList sack_f_by_name(Sack sack, const char *name);
PackageList sack_f_by_summary(Sack sack, const char *summary_substr);

#endif // QUERY_H