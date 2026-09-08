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
        /* child */
        printf("[child]  pid=%d ppid=%d secret=%d addr=%p (sleeping 5s - check `ps -f` now)\n",
               getpid(), getppid(), secret, (void *)&secret);
        fflush(stdout);
        sleep(5);

        /* Step 5: attempt to modify the parent's secret using an ordinary pointer.
         * &secret is the SAME virtual address the parent printed above, so if the
         * two processes truly shared memory, this write would change the parent's
         * value too. */
        int *p = &secret;
        *p = 999;
        printf("[child]  after *p=999: pid=%d secret=%d addr=%p\n",
               getpid(), secret, (void *)&secret);
        fflush(stdout);

        kill(getppid(), SIGUSR1); /* bonus: memory can't be touched, but signals still cross the boundary */
    } else {
        /* parent */
        signal(SIGUSR1, on_sigusr1);
        printf("[parent] pid=%d ppid=%d secret=%d addr=%p (child now sleeping)\n",
               getpid(), getppid(), secret, (void *)&secret);
        fflush(stdout);

        wait(NULL); /* block until child exits */

        printf("[parent] after child exited: pid=%d secret=%d addr=%p signal_from_child=%d\n",
               getpid(), secret, (void *)&secret, got_signal);
    }

    return 0;
}
