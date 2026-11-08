#ifndef _SHIM_SIGNAL_H
#define _SHIM_SIGNAL_H

#ifndef _WASI_EMULATED_SIGNAL
#define _WASI_EMULATED_SIGNAL 1
#endif

#include_next <signal.h>

#define __NEED_sigset_t
#define __NEED_pid_t
#define __NEED_uid_t
#define __NEED_size_t
#include <bits/alltypes.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SIG_BLOCK
#define SIG_BLOCK     0
#define SIG_UNBLOCK   1
#define SIG_SETMASK   2
#endif

#ifndef SA_NOCLDSTOP
#define SA_NOCLDSTOP  1
#define SA_NOCLDWAIT  2
#define SA_SIGINFO    4
#define SA_ONSTACK    0x08000000
#define SA_RESTART    0x10000000
#define SA_NODEFER    0x40000000
#define SA_RESETHAND  0x80000000
#endif

#ifndef __wasilibc_unmodified_upstream
typedef struct {
    int si_signo;
    int si_errno;
    int si_code;
    pid_t si_pid;
    uid_t si_uid;
    int si_status;
    void *si_addr;
} siginfo_t;

struct sigaction {
    union {
        void (*sa_handler)(int);
        void (*sa_sigaction)(int, siginfo_t *, void *);
    } __sa_handler;
    sigset_t sa_mask;
    int sa_flags;
    void (*sa_restorer)(void);
};
#define sa_handler   __sa_handler.sa_handler
#define sa_sigaction __sa_handler.sa_sigaction

int kill(pid_t pid, int sig);
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigaddset(sigset_t *set, int signum);
int sigdelset(sigset_t *set, int signum);
int sigismember(const sigset_t *set, int signum);
int sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
int sigsuspend(const sigset_t *mask);
int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact);
int sigpending(sigset_t *set);
int siginterrupt(int sig, int flag);
#endif

#ifdef __cplusplus
}
#endif

#endif
