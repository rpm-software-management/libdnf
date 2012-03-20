#ifndef HY_TYPES_H
#define HY_TYPES_H

typedef struct _FRepo * FRepo; // can't call this Repo, nameclash libsolv
typedef struct _Goal * Goal;
typedef struct _Package * Package;
typedef struct _PackageList * PackageList;
typedef struct _PackageListIter * PackageListIter;
typedef struct _Query * Query;
typedef struct _Sack * Sack;

#endif // HY_TYPES_H
