#ifndef _SHIM_FCNTL_H
#define _SHIM_FCNTL_H

#include_next <fcntl.h>

#ifndef F_DUPFD
#define F_DUPFD 0
#endif

#endif
