#include <termios.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#define WEAK __attribute__((weak))

static struct termios g_termios = {
    .c_iflag = ICRNL | IXON,
    .c_oflag = OPOST | ONLCR,
    .c_cflag = CS8 | CREAD | CLOCAL | B38400,
    .c_lflag = ISIG | ICANON | ECHO | ECHOE | ECHOK | IEXTEN,
    .c_line = 0,
    .c_cc = {
        [VINTR] = 3,
        [VQUIT] = 28,
        [VERASE] = 127,
        [VKILL] = 21,
        [VEOF] = 4,
        [VTIME] = 0,
        [VMIN] = 1,
        [VSTART] = 17,
        [VSTOP] = 19,
        [VSUSP] = 26,
    },
    .__c_ispeed = B38400,
    .__c_ospeed = B38400,
};

WEAK speed_t cfgetospeed(const struct termios *t) {
    return t ? t->__c_ospeed : B38400;
}

WEAK speed_t cfgetispeed(const struct termios *t) {
    return t ? t->__c_ispeed : B38400;
}

WEAK int cfsetospeed(struct termios *t, speed_t speed) {
    if (!t) { errno = EINVAL; return -1; }
    t->__c_ospeed = speed;
    return 0;
}

WEAK int cfsetispeed(struct termios *t, speed_t speed) {
    if (!t) { errno = EINVAL; return -1; }
    t->__c_ispeed = speed;
    return 0;
}

WEAK int tcgetattr(int fd, struct termios *t) {
    (void)fd;
    if (!t) { errno = EINVAL; return -1; }
    *t = g_termios;
    return 0;
}

WEAK int tcsetattr(int fd, int optional_actions, const struct termios *t) {
    (void)fd;
    (void)optional_actions;
    if (!t) { errno = EINVAL; return -1; }
    g_termios = *t;
    return 0;
}

WEAK void cfmakeraw(struct termios *t) {
    if (!t) return;
    t->c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    t->c_oflag &= ~OPOST;
    t->c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    t->c_cflag &= ~(CSIZE | PARENB);
    t->c_cflag |= CS8;
    t->c_cc[VMIN] = 1;
    t->c_cc[VTIME] = 0;
}

WEAK int tcsendbreak(int fd, int duration) {
    (void)fd; (void)duration;
    return 0;
}

WEAK int tcdrain(int fd) {
    (void)fd;
    return 0;
}

WEAK int tcflush(int fd, int queue_selector) {
    (void)fd; (void)queue_selector;
    return 0;
}

WEAK int tcflow(int fd, int action) {
    (void)fd; (void)action;
    return 0;
}

WEAK pid_t tcgetsid(int fd) {
    (void)fd;
    return 1;
}

WEAK int resizeterm(int lines, int columns);

static unsigned short g_terminal_rows = 0;
static unsigned short g_terminal_cols = 0;

__attribute__((export_name("set_terminal_size")))
void set_terminal_size(int cols, int rows) {
    if (cols > 0) g_terminal_cols = (unsigned short)cols;
    if (rows > 0) g_terminal_rows = (unsigned short)rows;
    if (cols > 0 && rows > 0 && resizeterm) {
        resizeterm(rows, cols);
    }
}

WEAK int ioctl(int fd, int request, ...) {
    (void)fd;
    va_list ap;
    va_start(ap, request);
    if (request == TIOCGWINSZ) {
        struct winsize *ws = va_arg(ap, struct winsize *);
        va_end(ap);
        if (!ws) { errno = EINVAL; return -1; }
        if (g_terminal_cols > 0 && g_terminal_rows > 0) {
            ws->ws_row = g_terminal_rows;
            ws->ws_col = g_terminal_cols;
        } else {
            const char *lines_env = getenv("LINES");
            const char *cols_env = getenv("COLUMNS");
            int rows = lines_env ? atoi(lines_env) : 0;
            int cols = cols_env ? atoi(cols_env) : 0;
            ws->ws_row = (rows > 0) ? (unsigned short)rows : 24;
            ws->ws_col = (cols > 0) ? (unsigned short)cols : 80;
        }
        ws->ws_xpixel = 0;
        ws->ws_ypixel = 0;
        return 0;
    }
    va_end(ap);
    return 0;
}

/* Signal shims */
WEAK int kill(pid_t pid, int sig) {
    (void)pid; (void)sig;
    return 0;
}

WEAK int sigemptyset(sigset_t *set) {
    if (set) memset(set, 0, sizeof(*set));
    return 0;
}

WEAK int sigfillset(sigset_t *set) {
    if (set) memset(set, 0xff, sizeof(*set));
    return 0;
}

WEAK int sigaddset(sigset_t *set, int signum) {
    (void)set; (void)signum;
    return 0;
}

WEAK int sigdelset(sigset_t *set, int signum) {
    (void)set; (void)signum;
    return 0;
}

WEAK int sigismember(const sigset_t *set, int signum) {
    (void)set; (void)signum;
    return 0;
}

WEAK int sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {
    (void)how; (void)set;
    if (oldset) memset(oldset, 0, sizeof(*oldset));
    return 0;
}

WEAK int sigsuspend(const sigset_t *mask) {
    (void)mask;
    errno = EINTR;
    return -1;
}

WEAK int sigaction(int signum, const struct sigaction *act, struct sigaction *oldact) {
    (void)signum; (void)act;
    if (oldact) memset(oldact, 0, sizeof(*oldact));
    return 0;
}

WEAK int sigpending(sigset_t *set) {
    if (set) memset(set, 0, sizeof(*set));
    return 0;
}

WEAK int siginterrupt(int sig, int flag) {
    (void)sig; (void)flag;
    return 0;
}

/* Process / User / File shims */
WEAK mode_t umask(mode_t mask) {
    (void)mask;
    return 022;
}

WEAK int chmod(const char *pathname, mode_t mode) {
    (void)pathname; (void)mode;
    return 0;
}

WEAK int fchmod(int fd, mode_t mode) {
    (void)fd; (void)mode;
    return 0;
}

WEAK int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags) {
    (void)dirfd; (void)pathname; (void)mode; (void)flags;
    return 0;
}

WEAK int chown(const char *pathname, uid_t owner, gid_t group) {
    (void)pathname; (void)owner; (void)group;
    return 0;
}

WEAK int fchown(int fd, uid_t owner, gid_t group) {
    (void)fd; (void)owner; (void)group;
    return 0;
}

WEAK int lchown(const char *pathname, uid_t owner, gid_t group) {
    (void)pathname; (void)owner; (void)group;
    return 0;
}

WEAK int mkfifo(const char *pathname, mode_t mode) {
    (void)pathname; (void)mode;
    errno = ENOSYS;
    return -1;
}

WEAK int mknod(const char *pathname, mode_t mode, dev_t dev) {
    (void)pathname; (void)mode; (void)dev;
    errno = ENOSYS;
    return -1;
}

WEAK uid_t getuid(void) { return 1000; }
WEAK uid_t geteuid(void) { return 1000; }
WEAK gid_t getgid(void) { return 1000; }
WEAK gid_t getegid(void) { return 1000; }
WEAK int setuid(uid_t uid) { (void)uid; return 0; }
WEAK int seteuid(uid_t euid) { (void)euid; return 0; }
WEAK int setgid(gid_t gid) { (void)gid; return 0; }
WEAK int setegid(gid_t egid) { (void)egid; return 0; }
WEAK int getgroups(int size, gid_t list[]) { (void)size; (void)list; return 0; }

static struct passwd g_passwd = {
    .pw_name = "user",
    .pw_passwd = "x",
    .pw_uid = 1000,
    .pw_gid = 1000,
    .pw_gecos = "User",
    .pw_dir = "/",
    .pw_shell = "/bin/sh",
};

WEAK struct passwd *getpwnam(const char *name) {
    (void)name;
    const char *home = getenv("HOME");
    if (home) g_passwd.pw_dir = (char *)home;
    return &g_passwd;
}

WEAK struct passwd *getpwuid(uid_t uid) {
    (void)uid;
    const char *home = getenv("HOME");
    if (home) g_passwd.pw_dir = (char *)home;
    return &g_passwd;
}

WEAK int getpwnam_r(const char *name, struct passwd *pwd, char *buf, size_t buflen, struct passwd **result) {
    (void)name; (void)buf; (void)buflen;
    if (pwd) *pwd = *getpwnam(name);
    if (result) *result = pwd;
    return 0;
}

WEAK int getpwuid_r(uid_t uid, struct passwd *pwd, char *buf, size_t buflen, struct passwd **result) {
    (void)uid; (void)buf; (void)buflen;
    if (pwd) *pwd = *getpwuid(uid);
    if (result) *result = pwd;
    return 0;
}

WEAK void setpwent(void) {}
WEAK void endpwent(void) {}
WEAK struct passwd *getpwent(void) { return NULL; }

static char *g_gr_mem[] = { NULL };
static struct group g_group = {
    .gr_name = "user",
    .gr_passwd = "x",
    .gr_gid = 1000,
    .gr_mem = g_gr_mem,
};

WEAK struct group *getgrgid(gid_t gid) {
    (void)gid;
    return &g_group;
}

WEAK struct group *getgrnam(const char *name) {
    (void)name;
    return &g_group;
}

WEAK int pipe(int pipefd[2]) {
    (void)pipefd;
    errno = ENOSYS;
    return -1;
}

WEAK int pipe2(int pipefd[2], int flags) {
    (void)pipefd; (void)flags;
    errno = ENOSYS;
    return -1;
}

WEAK int dup(int oldfd) {
    return oldfd;
}

WEAK int dup2(int oldfd, int newfd) {
    (void)oldfd;
    return newfd;
}

WEAK int fchdir(int fd) {
    (void)fd;
    errno = ENOSYS;
    return -1;
}

WEAK unsigned alarm(unsigned seconds) {
    (void)seconds;
    return 0;
}

WEAK int pause(void) {
    errno = EINTR;
    return -1;
}

WEAK pid_t fork(void) {
    errno = ENOSYS;
    return -1;
}

WEAK pid_t vfork(void) {
    errno = ENOSYS;
    return -1;
}

WEAK int execve(const char *pathname, char *const argv[], char *const envp[]) {
    (void)pathname; (void)argv; (void)envp;
    errno = ENOSYS;
    return -1;
}

WEAK int execv(const char *pathname, char *const argv[]) {
    (void)pathname; (void)argv;
    errno = ENOSYS;
    return -1;
}

WEAK int execvp(const char *file, char *const argv[]) {
    (void)file; (void)argv;
    errno = ENOSYS;
    return -1;
}

WEAK int execl(const char *pathname, const char *arg, ...) {
    (void)pathname; (void)arg;
    errno = ENOSYS;
    return -1;
}

WEAK int execlp(const char *file, const char *arg, ...) {
    (void)file; (void)arg;
    errno = ENOSYS;
    return -1;
}

WEAK int execle(const char *pathname, const char *arg, ...) {
    (void)pathname; (void)arg;
    errno = ENOSYS;
    return -1;
}

WEAK pid_t wait(int *wstatus) {
    (void)wstatus;
    errno = ECHILD;
    return -1;
}

WEAK pid_t waitpid(pid_t pid, int *wstatus, int options) {
    (void)pid; (void)wstatus; (void)options;
    errno = ECHILD;
    return -1;
}

WEAK pid_t getppid(void) { return 1; }
WEAK pid_t getpgrp(void) { return 1; }
WEAK pid_t getpgid(pid_t pid) { (void)pid; return 1; }
WEAK int setpgid(pid_t pid, pid_t pgid) { (void)pid; (void)pgid; return 0; }
WEAK pid_t setsid(void) { return 1; }
WEAK pid_t getsid(pid_t pid) { (void)pid; return 1; }
WEAK pid_t tcgetpgrp(int fd) { (void)fd; return 1; }
WEAK int tcsetpgrp(int fd, pid_t pgrp) { (void)fd; (void)pgrp; return 0; }

WEAK char *ttyname(int fd) {
    (void)fd;
    return "/dev/tty";
}

WEAK int ttyname_r(int fd, char *buf, size_t buflen) {
    (void)fd;
    const char *name = "/dev/tty";
    if (!buf || buflen < strlen(name) + 1) {
        errno = ERANGE;
        return ERANGE;
    }
    strcpy(buf, name);
    return 0;
}

WEAK int getrlimit(int resource, struct rlimit *rlim) {
    (void)resource;
    if (rlim) {
        rlim->rlim_cur = 1024;
        rlim->rlim_max = 1024;
    }
    return 0;
}

WEAK int setrlimit(int resource, const struct rlimit *rlim) {
    (void)resource; (void)rlim;
    return 0;
}

WEAK int mkstemp(char *tmpl) {
    if (!tmpl) { errno = EINVAL; return -1; }
    size_t len = strlen(tmpl);
    if (len < 6 || strcmp(tmpl + len - 6, "XXXXXX") != 0) {
        errno = EINVAL;
        return -1;
    }
    static unsigned int counter = 0;
    for (int attempt = 0; attempt < 1000; attempt++) {
        unsigned int val = ++counter + (unsigned int)attempt * 7919U;
        char suffix[7];
        for (int i = 0; i < 6; i++) {
            suffix[i] = "0123456789abcdefghijklmnopqrstuvwxyz"[val % 36];
            val /= 36;
        }
        suffix[6] = '\0';
        memcpy(tmpl + len - 6, suffix, 6);
        int fd = open(tmpl, O_RDWR | O_CREAT | O_EXCL, 0600);
        if (fd >= 0) return fd;
        if (errno != EEXIST) return -1;
    }
    errno = EEXIST;
    return -1;
}

WEAK int gethostname(char *name, size_t len) {
    const char *host = "brsh";
    if (!name || len == 0) { errno = EINVAL; return -1; }
    strncpy(name, host, len);
    name[len - 1] = '\0';
    return 0;
}

WEAK char *getlogin(void) {
    const char *user = getenv("USER");
    return (char *)(user ? user : "user");
}

WEAK int getlogin_r(char *buf, size_t bufsize) {
    const char *user = getlogin();
    if (!buf || bufsize < strlen(user) + 1) {
        errno = ERANGE;
        return ERANGE;
    }
    strcpy(buf, user);
    return 0;
}

ssize_t __wrap_readlink(const char *pathname, char *buf, size_t bufsiz) {
    (void)pathname; (void)buf; (void)bufsiz;
    errno = EINVAL;
    return -1;
}

ssize_t __wrap_readlinkat(int dirfd, const char *pathname, char *buf, size_t bufsiz) {
    (void)dirfd; (void)pathname; (void)buf; (void)bufsiz;
    errno = EINVAL;
    return -1;
}

int __wrap_isatty(int fd) {
    return (fd >= 0 && fd <= 2) ? 1 : 0;
}

int __wrap_poll(struct pollfd *fds, nfds_t nfds, int timeout) {
    if (timeout == 0) {
        for (nfds_t i = 0; i < nfds; i++) fds[i].revents = 0;
        return 0;
    }
    int ready = 0;
    for (nfds_t i = 0; i < nfds; i++) {
        if (fds[i].fd >= 0) {
            fds[i].revents = fds[i].events & (POLLIN | POLLOUT | POLLRDNORM | POLLWRNORM);
            if (fds[i].revents) ready++;
        } else {
            fds[i].revents = 0;
        }
    }
    return ready;
}




