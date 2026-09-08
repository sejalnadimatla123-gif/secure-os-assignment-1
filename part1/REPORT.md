# Part 1 – Process Isolation Investigation

## Prediction (made before running)
`fork()` creates a separate address space (copy-on-write). Expected:
- Same virtual address for `secret` in parent and child (stack layout is copied).
- After the child sets `secret = 999`, the parent's copy stays `100` — the two
  are backed by different physical pages despite sharing a virtual address.
- Child's `getppid()` == parent's `getpid()`; each process has its own PID.

## Evidence
```
[before fork] pid=54056 secret=100 addr=0x7ff7b81a4458
[child]  pid=54057 ppid=54056 secret=999 addr=0x7ff7b81a4458
[parent] pid=54056 ppid=54051 secret=100 addr=0x7ff7b81a4458 signal_from_child=1
```
- Child and parent report the **identical virtual address** for `secret`.
- Child's value is `999`, parent's stays `100` → confirms separate physical
  backing (copy-on-write).
- `child ppid (54056) == parent pid (54056)`; parent's own `ppid` is the shell.

### Edge case found while collecting evidence
First run (before adding `fflush(stdout)`) printed the `[before fork]` line
**twice** — once from each process. Cause: when stdout isn't a TTY (e.g.
piped), glibc/libc fully buffers it, so the line written before `fork()` sits
unflushed in the stdio buffer. `fork()` duplicates the *entire* address space,
including that buffer, so both parent and child flush their own copy of it at
exit. Fixed with `fflush(stdout)` right before `fork()`. This is itself
evidence that fork() truly copies everything, not just the variables we're
watching.

## Security analysis
The OS gives each process its own virtual→physical page table. Two processes
can show the *same virtual address* for a variable yet that address maps to
different physical RAM, so one process cannot read or corrupt another's
memory just by knowing an address — a would-be attacker needs a separate,
OS-mediated channel (shared memory, ptrace, a kernel bug) to cross that
boundary. This is the basis for the security guarantee that a compromised
process can't directly read secrets out of another process's memory.

## Adversarial / misuse test
Added: after mutating `secret`, the child calls `kill(getppid(), SIGUSR1)` to
try to "reach into" the parent's execution. Result: `signal_from_child=1` —
the signal **is** delivered, even though memory is not shared. This shows
process isolation is *not absolute*: the address space is private, but
same-privilege processes still share a signal namespace by PID and can
interrupt each other. An attacker who can't read/write another process's
memory can still, e.g., send it `SIGKILL`/`SIGSTOP` if they know (or guess)
its PID and share its UID — a real basis for PID-based denial-of-service and
part of why containers/namespaces additionally isolate PID space, not just
memory.

## Improvement
Given the buffering bug found above, any future forking code in this project
flushes stdout (or uses `_exit()` in the child) before `fork()`/`exit()` to
avoid duplicated output — a real-world instance of "fork() copies more state
than you think," which matters for programs that fork after buffering
sensitive log data.
