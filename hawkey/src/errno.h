#ifndef HY_ERRNO_H
#define HY_ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif

enum _hy_errors_e {
    HY_E_FAILED		= 1,	// general runtime error
    HY_E_OP,			// client programming error
    HY_E_LIBSOLV,		// erorr propagated from libsolv
    HY_E_IO,			// I/O error
    HY_E_CACHE_WRITE,		// cache write error
    HY_E_QUERY,			// ill-formed query
    HY_E_ARCH,			// unknown arch
    HY_E_VALIDATION,		// validation check failed
    HY_E_SELECTOR,		// ill-specified selector
    HY_E_NO_SOLUTION,		// goal found no solutions
};

extern __thread int hy_errno;

int hy_get_errno(void);

#ifdef __cplusplus
}
#endif

#endif /* HY_ERRNO_H */
