#ifndef HY_ERRNO_H
#define HY_ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif

#define HY_E_FAILED		1	// general runtime error
#define HY_E_IO			2	// I/O error
#define HY_E_CACHE_WRITE	3	// cache write error
#define HY_E_QUERY		4	// ill-formed query
#define HY_E_ARCH		5	// unknown arch
#define HY_E_VALIDATION		6	// validation check failed
#define HY_E_LIBSOLV		7	// erorr propagated from libsolv
#define HY_E_SELECTOR		8	// ill-specified selector
#define HY_E_OP			9	// client programming error
#define HY_E_NO_SOLUTION	10	// goal found no solutions


extern __thread int hy_errno;

int hy_get_errno(void);

#ifdef __cplusplus
}
#endif

#endif /* HY_ERRNO_H */
