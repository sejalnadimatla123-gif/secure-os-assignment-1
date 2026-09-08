/* Part 3: Identity and Least-Privilege Investigation */
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

static void try_access(const char *path) {
    printf("\nAttempting to open \"%s\" for reading...\n", path);
    FILE *f = fopen(path, "r");
    if (f) {
        printf("  SUCCESS: file opened.\n");
        fclose(f);
    } else {
        printf("  FAILED: %s (errno %d)\n", strerror(errno), errno);
    }
}

int main(int argc, char *argv[]) {
    uid_t ruid = getuid();
    uid_t euid = geteuid();
    gid_t rgid = getgid();
    gid_t egid = getegid();
    char *login = getlogin();

    printf("=== Process Identity ===\n");
    printf("Real UID:      %d\n", ruid);
    printf("Effective UID: %d\n", euid);
    printf("Real GID:      %d\n", rgid);
    printf("Effective GID: %d\n", egid);
    printf("Login name:    %s\n", login ? login : "(unavailable)");

    /* A file the normal account should NOT have permission to read.
       Override with argv[1] to test a different path/account. */
    const char *target = (argc > 1) ? argv[1] : "/etc/master.passwd";
    try_access(target);

    printf("\n=== Comparison ===\n");
    printf("Identity shown above reflects RUID/EUID %d/%d.\n", ruid, euid);
    printf("Access to \"%s\" succeeds only if EUID/EGID satisfy that file's permission bits.\n", target);

    return 0;
}
