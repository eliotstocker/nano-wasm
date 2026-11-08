#ifndef _SHIM_STDLIB_H
#define _SHIM_STDLIB_H

#include_next <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

int mkstemp(char *template_name);

#ifdef __cplusplus
}
#endif

#endif
