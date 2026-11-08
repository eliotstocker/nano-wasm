#ifndef _SHIM_UNISTD_H
#define _SHIM_UNISTD_H

#ifndef _WASI_EMULATED_GETPID
#define _WASI_EMULATED_GETPID 1
#endif

#include_next <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __wasilibc_unmodified_upstream
int pipe(int pipefd[2]);
int pipe2(int pipefd[2], int flags);
int dup(int oldfd);
int dup2(int oldfd, int newfd);
int chown(const char *pathname, uid_t owner, gid_t group);
int fchown(int fd, uid_t owner, gid_t group);
int lchown(const char *pathname, uid_t owner, gid_t group);
int fchdir(int fd);
unsigned alarm(unsigned seconds);
int pause(void);
pid_t fork(void);
pid_t vfork(void);
int execve(const char *pathname, char *const argv[], char *const envp[]);
int execv(const char *pathname, char *const argv[]);
int execle(const char *pathname, const char *arg, ...);
int execl(const char *pathname, const char *arg, ...);
int execvp(const char *file, char *const argv[]);
int execlp(const char *file, const char *arg, ...);
pid_t getppid(void);
pid_t getpgrp(void);
pid_t getpgid(pid_t pid);
int setpgid(pid_t pid, pid_t pgid);
pid_t setsid(void);
pid_t getsid(pid_t pid);
char *ttyname(int fd);
int ttyname_r(int fd, char *buf, size_t buflen);
pid_t tcgetpgrp(int fd);
int tcsetpgrp(int fd, pid_t pgrp);
uid_t getuid(void);
uid_t geteuid(void);
gid_t getgid(void);
gid_t getegid(void);
int getgroups(int size, gid_t list[]);
int setuid(uid_t uid);
int seteuid(uid_t euid);
int setgid(gid_t gid);
int setegid(gid_t egid);
#endif

#ifdef __cplusplus
}
#endif

#endif
