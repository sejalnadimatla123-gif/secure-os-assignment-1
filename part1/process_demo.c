#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

static volatile sig_atomic_t got_signal = 0;
static void on_sigusr1(int sig) { (void)sig; got_signal = 1; }

int main(void) {
    int secret = 100;

    printf("[before fork] pid=%d secret=%d addr=%p\n", getpid(), secret, (void *)&secret);
    fflush(stdout); /* avoid duplicated output: fork() copies the unflushed stdio buffer too */

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        /* child: attempted "violation" - modify secret and try to disturb the parent */
        secret = 999;
        printf("[child]  pid=%d ppid=%d secret=%d addr=%p\n",
               getpid(), getppid(), secret, (void *)&secret);
        kill(getppid(), SIGUSR1); /* memory can't be touched, but signals still cross the boundary */
    } else {
        /* parent */
        signal(SIGUSR1, on_sigusr1);
        sleep(1); /* let child print/signal first so output stays readable */
        printf("[parent] pid=%d ppid=%d secret=%d addr=%p signal_from_child=%d\n",
               getpid(), getppid(), secret, (void *)&secret, got_signal);
        wait(NULL);
    }

    return 0;
}
