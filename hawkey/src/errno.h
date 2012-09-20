#ifndef HY_ERRNO_H
#define HY_ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif

#define HY_E_IO 1		// I/O error
#define HY_E_CACHE_WRITE 2	// cache write error
#define HY_E_QUERY 3		// ill-formed query
#define HY_E_ARCH 4		// unknown arch

extern __thread int hy_errno;

int hy_get_errno(void);

#ifdef __cplusplus
}
#endif

#endif /* HY_ERRNO_H */
