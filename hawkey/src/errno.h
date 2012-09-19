#ifndef HY_ERRNO_H
#define HY_ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif

#define HY_E_CACHE_WRITE -1	// Cache write error
#define HY_E_IO 2		// I/O error
#define HY_E_QUERY 3		// Ill-formed Query.

extern __thread int hy_errno;

int hy_get_errno(void);

#ifdef __cplusplus
}
#endif

#endif /* HY_ERRNO_H */
