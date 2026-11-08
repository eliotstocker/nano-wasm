#ifndef _SHIM_SYS_RESOURCE_H
#define _SHIM_SYS_RESOURCE_H

#ifndef _WASI_EMULATED_PROCESS_CLOCKS
#define _WASI_EMULATED_PROCESS_CLOCKS 1
#endif

#include_next <sys/resource.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __wasilibc_unmodified_upstream
typedef unsigned long long rlim_t;

struct rlimit {
    rlim_t rlim_cur;
    rlim_t rlim_max;
};

#define RLIM_INFINITY (~0ULL)
#define RLIM_SAVED_CUR RLIM_INFINITY
#define RLIM_SAVED_MAX RLIM_INFINITY

#define RLIMIT_CPU     0
#define RLIMIT_FSIZE   1
#define RLIMIT_DATA    2
#define RLIMIT_STACK   3
#define RLIMIT_CORE    4
#define RLIMIT_RSS     5
#define RLIMIT_NPROC   6
#define RLIMIT_NOFILE  7
#define RLIMIT_MEMLOCK 8
#define RLIMIT_AS      9

int getrlimit(int resource, struct rlimit *rlim);
int setrlimit(int resource, const struct rlimit *rlim);
#endif

#ifdef __cplusplus
}
#endif

#endif
