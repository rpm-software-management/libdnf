#ifndef HY_TYPES_H
#define HY_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _HyRepo * HyRepo;
typedef struct _HyGoal * HyGoal;
typedef struct _HyPackage * HyPackage;
typedef struct _HyPackageDelta * HyPackageDelta;
typedef struct _HyPackageList * HyPackageList;
typedef struct _HyPackageListIter * HyPackageListIter;
typedef struct _HyQuery * HyQuery;
typedef struct _HySack * HySack;

typedef const unsigned char HyChecksum;

#ifdef __cplusplus
}
#endif

#endif /* HY_TYPES_H */
