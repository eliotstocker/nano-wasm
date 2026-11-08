#ifndef _SHIM_STDIO_H
#define _SHIM_STDIO_H

#include_next <stdio.h>

#ifndef P_tmpdir
#define P_tmpdir "/tmp"
#endif

#endif
