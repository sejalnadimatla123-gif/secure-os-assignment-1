# Part 1 – Process Isolation Investigation

## Predictions (made before running)
1. The parent and child will print **different** virtual addresses for
   `secret`, since `fork()` creates a copy of the process.
2. Changing the child's `secret` will **not** change the parent's `secret`
   — they have independent memory.

## Program output
```
[before fork] pid=54168 secret=100 addr=0x7ff7b8a5d458
[parent] pid=54168 ppid=54165 secret=100 addr=0x7ff7b8a5d458 (child now sleeping)
[child]  pid=54170 ppid=54168 secret=100 addr=0x7ff7b8a5d458 (sleeping 5s - check `ps -f` now)
[child]  after *p=999: pid=54170 secret=999 addr=0x7ff7b8a5d458
[parent] after child exited: pid=54168 secret=100 addr=0x7ff7b8a5d458 signal_from_child=1
```

## Process observation (`ps -f`, taken during the child's 5s sleep)
```
  UID   PID  PPID   C STIME   TTY           TIME CMD
  501 54168 54165   0 10:46AM ??         0:00.00 ./process_demo
  501 54170 54168   0 10:46AM ??         0:00.00 ./process_demo
```
Two independent OS processes, same binary, distinct PIDs — `54170`'s `PPID`
(`54168`) matches the parent's own `PID`, confirming the parent/child
relationship the program reported. Only one row is "running" work
(child in `sleep()`, parent blocked in `wait()`), visible as two live entries
rather than one — direct proof `fork()` produced a second schedulable process,
not just a new function call.

## Analysis — answering the experiment questions

**1. Same or different virtual address?**
My prediction was **wrong**. Parent and child print the exact same address
(`0x7ff7b8a5d458`) for `secret`. `fork()` copies the *entire* address space
layout (stack, heap, code segment placement chosen once at `exec()`/ASLR
time) — it does not re-lay-out memory per process. Both processes end up with
identical virtual addresses; what changes is what physical page backs that
address after copy-on-write splits them apart.

**2. Does changing the child's secret change the parent's?**
My prediction was **correct**. After `*p = 999` in the child, the child shows
`secret=999` while the parent's final print still shows `secret=100`. Same
virtual address, different value — this is only possible because they are
backed by different physical memory. The two facts together (identical
address, independent value) are the real lesson: **virtual address equality
says nothing about a shared physical page.**

**3–4. `ps -f` during the child's sleep**
Confirms two live OS-scheduled processes exist simultaneously, `54170`'s
`PPID` matching `54168`, matching the program's own `getppid()` output.

**5. Ordinary-pointer write attempt**
`int *p = &secret; *p = 999;` executes in the child using the identical
virtual address the parent is also using. It "succeeds" from the child's own
point of view (`secret` becomes `999` there) but the parent's copy, checked
after `wait()` returns, is untouched (`100`). The write never had a chance to
reach the parent — the MMU translates that same virtual address to a
different physical frame in each process, so an ordinary pointer, with no
debugger, no `ptrace`, no shared memory, physically cannot cross the
boundary. This is the OS enforcing process isolation at the hardware level
(per-process page tables), not merely a language-level convention.

## Security analysis
Because isolation is enforced by the MMU/page tables rather than by
convention, a process cannot corrupt or read another unrelated process's
memory just because it happens to know (or guess) an address — the same
address in a different process maps to different physical RAM. This is what
lets the OS run mutually-untrusting processes (e.g. a browser tab and your
password manager) side by side: compromising one process's memory does not,
by itself, give an attacker a pointer-write path into another's. Any actual
cross-process interaction (signals, shared memory, ptrace) has to go through
an explicit, permission-checked OS syscall — it's never a side effect of an
ordinary pointer.

## Adversarial / misuse test (bonus, beyond the required pointer attempt)
The child also calls `kill(getppid(), SIGUSR1)` after its pointer-write
attempt. The parent shows `signal_from_child=1` — delivered. So while
*memory* is strictly isolated, same-UID processes still share a signal
namespace by PID: a compromised process cannot read/corrupt another's data,
but if it knows the PID it can still disrupt it (e.g. `SIGKILL`/`SIGSTOP`),
a real basis for PID-based denial-of-service and part of why containers add
PID-namespace isolation on top of plain memory isolation.

## Improvement
First run (before adding `fflush(stdout)` before `fork()`) printed the
`[before fork]` line twice, because stdout was fully buffered (non-TTY
output) and `fork()` duplicated that unflushed buffer along with everything
else. Fixed by flushing before `fork()`/after each `printf`. Real-world
takeaway: `fork()` copies *all* process state, including stdio buffers — code
that forks after buffering sensitive or large output should flush (or use
`_exit()` in the child) to avoid leaking/duplicating that buffered data.
