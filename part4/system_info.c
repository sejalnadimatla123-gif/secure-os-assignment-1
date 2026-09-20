#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>

int main(void)
{
    /* PID and PPID */
    pid_t pid = getpid();
    pid_t ppid = getppid();
    printf("PID: %d\n", pid);
    printf("PPID: %d\n", ppid);

    /* UID/GID information */
    uid_t uid = getuid();
    uid_t euid = geteuid();
    gid_t gid = getgid();
    gid_t egid = getegid();
    printf("UID: %d\n", uid);
    printf("EUID: %d\n", euid);
    printf("GID: %d\n", gid);
    printf("EGID: %d\n", egid);

    /* Current working directory */
    char cwd[4096];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("CWD: %s\n", cwd);
    } else {
        perror("getcwd");
    }

    /* Environment information (variable names only, no values) */
    printf("Environment variable names:\n");
    extern char **environ;
    for (char **env = environ; *env != NULL; env++) {
        char *eq = strchr(*env, '=');
        if (eq != NULL) {
            *eq = '\0';
            printf("  %s\n", *env);
        }
    }

    /* Open file descriptors */
    printf("Open file descriptors:\n");
    DIR *dir = opendir("/proc/self/fd");
    if (dir != NULL) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] != '.') {
                char link_path[512];
                char target[4096];
                snprintf(link_path, sizeof(link_path),
                         "/proc/self/fd/%s", entry->d_name);
                ssize_t len = readlink(link_path, target, sizeof(target) - 1);
                if (len != -1) {
                    target[len] = '\0';
                    printf("  fd %s -> %s\n", entry->d_name, target);
                }
            }
        }
        closedir(dir);
    } else {
        perror("opendir /proc/self/fd");
    }

    /* /proc observation: process status */
    printf("/proc/self/status (selected fields):\n");
    FILE *status = fopen("/proc/self/status", "r");
    if (status != NULL) {
        char line[512];
        while (fgets(line, sizeof(line), status) != NULL) {
            if (strncmp(line, "Name:", 5) == 0 ||
                strncmp(line, "State:", 6) == 0 ||
                strncmp(line, "Threads:", 8) == 0 ||
                strncmp(line, "VmRSS:", 6) == 0) {
                printf("  %s", line);
            }
        }
        fclose(status);
    } else {
        perror("fopen /proc/self/status");
    }

    return 0;
}
