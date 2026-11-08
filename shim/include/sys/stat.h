#ifndef _SHIM_SYS_STAT_H
#define _SHIM_SYS_STAT_H

#include_next <sys/stat.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __wasilibc_unmodified_upstream
mode_t umask(mode_t mask);
int mkfifo(const char *pathname, mode_t mode);
int mknod(const char *pathname, mode_t mode, dev_t dev);
#endif

#ifdef __cplusplus
}
#endif

#endif
