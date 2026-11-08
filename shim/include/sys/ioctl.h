#ifndef _SHIM_SYS_IOCTL_H
#define _SHIM_SYS_IOCTL_H

#include_next <sys/ioctl.h>

#define __NEED_struct_winsize
#include <bits/alltypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TIOCGWINSZ
#define TIOCGWINSZ 0x5413
#endif

#ifndef TIOCSWINSZ
#define TIOCSWINSZ 0x5414
#endif

#ifndef TIOCGPGRP
#define TIOCGPGRP  0x540F
#endif

#ifndef TIOCSPGRP
#define TIOCSPGRP  0x5410
#endif

#ifndef TIOCSTI
#define TIOCSTI    0x5412
#endif

int ioctl(int fd, int request, ...);

#ifdef __cplusplus
}
#endif

#endif
