# Part 1: Process Isolation Investigation

## 1. Predictions (made before running)

1. The parent and child will print different virtual addresses for `secret`, since `fork()` creates a copy of the process.
2. Changing the child's `secret` will not change the parent's `secret`, they have independent memory.

## 2. Program output

```
[before fork] pid=54168 secret=100 addr=0x7ff7b8a5d458
[parent] pid=54168 ppid=54165 secret=100 addr=0x7ff7b8a5d458 (child now sleeping)
[child]  pid=54170 ppid=54168 secret=100 addr=0x7ff7b8a5d458 (sleeping 5s - check `ps -f` now)
[child]  after *p=999: pid=54170 secret=999 addr=0x7ff7b8a5d458
[parent] after child exited: pid=54168 secret=100 addr=0x7ff7b8a5d458 signal_from_child=1
```

## 3. Process observation (`ps -f`, taken during the child's 5s sleep)

```
  UID   PID  PPID   C STIME   TTY           TIME CMD
  501 54168 54165   0 10:46AM ??         0:00.00 ./process_demo
  501 54170 54168   0 10:46AM ??         0:00.00 ./process_demo
```

Two independent OS processes, same binary, distinct PIDs. `54170`'s PPID (`54168`) matches the parent's own PID, confirming the parent/child relationship the program reported. Only one row is "running" work (child in `sleep()`, parent blocked in `wait()`), visible as two live entries rather than one, direct proof `fork()` produced a second schedulable process, not just a new function call.

## 4. Experiment

### 4.1 Same or different virtual address?

My prediction was wrong. Parent and child print the exact same address (`0x7ff7b8a5d458`) for `secret`. `fork()` copies the entire address space layout (stack, heap, code segment placement chosen once at `exec()`/ASLR time), it does not re-lay-out memory per process. Both processes end up with identical virtual addresses; what changes is what physical page backs that address after copy-on-write splits them apart.

### 4.2 Does changing the child's secret change the parent's?

My prediction was correct. After `*p = 999` in the child, the child shows `secret=999` while the parent's final print still shows `secret=100`. Same virtual address, different value, this is only possible because they are backed by different physical memory. The two facts together (identical address, independent value) are the real lesson: virtual address equality says nothing about a shared physical page.

### 4.3 Child sleeps 5 seconds while the parent continues

The child calls `sleep(5)` right after printing its own line, before touching `secret` at all. The parent does not wait on the sleep, it moves straight on to `wait(NULL)`, so for those 5 seconds both processes are alive at once: the child is blocked in `sleep()`, the parent is blocked in `wait()`. That overlap is what step 4 observes with `ps -f`.

### 4.4 `ps -f` during the child's sleep

Confirms two live OS-scheduled processes exist simultaneously, `54170`'s PPID matching `54168`, matching the program's own `getppid()` output.

### 4.5 Ordinary-pointer write attempt

`int *p = &secret; *p = 999;` executes in the child using the identical virtual address the parent is also using. It "succeeds" from the child's own point of view (`secret` becomes `999` there) but the parent's copy, checked after `wait()` returns, is untouched (`100`). The write never had a chance to reach the parent, the MMU translates that same virtual address to a different physical frame in each process, so an ordinary pointer, with no debugger, no `ptrace`, no shared memory, physically cannot cross the boundary. This is the OS enforcing process isolation at the hardware level (per-process page tables), not merely a language-level convention.

## 5. Security analysis

Because isolation is enforced by the MMU/page tables rather than by convention, a process cannot corrupt or read another unrelated process's memory just because it happens to know (or guess) an address, the same address in a different process maps to different physical RAM. This is what lets the OS run mutually-untrusting processes (e.g. a browser tab and a password manager) side by side: compromising one process's memory does not, by itself, give an attacker a pointer-write path into another's. Any actual cross-process interaction (signals, shared memory, ptrace) has to go through an explicit, permission-checked OS syscall, it's never a side effect of an ordinary pointer.

## 6. Adversarial / misuse test (bonus, beyond the required pointer attempt)

The child also calls `kill(getppid(), SIGUSR1)` after its pointer-write attempt. The parent shows `signal_from_child=1`, delivered. So while memory is strictly isolated, same-UID processes still share a signal namespace by PID: a compromised process cannot read/corrupt another's data, but if it knows the PID it can still disrupt it (e.g. `SIGKILL`/`SIGSTOP`), a real basis for PID-based denial-of-service and part of why containers add PID-namespace isolation on top of plain memory isolation.

## 7. Improvement

The first run (before adding `fflush(stdout)` before `fork()`) printed the `[before fork]` line twice, because stdout was fully buffered (non-TTY output) and `fork()` duplicated that unflushed buffer along with everything else. Fixed by flushing before `fork()`/after each `printf`. Real-world takeaway: `fork()` copies all process state, including stdio buffers, code that forks after buffering sensitive or large output should flush (or use `_exit()` in the child) to avoid leaking/duplicating that buffered data.

## 8. Analysis questions

### Q1. If parent and child print the same address for `secret`, does that mean they are accessing the same physical memory? Explain.

No. Identical virtual addresses do not imply the same physical memory. Every process has its own page table, and the MMU translates a virtual address to a physical frame independently per process, so the same number can (and here does) map to two different physical pages. This experiment shows exactly that: the parent and child print the same address for `secret`, but the child's write of `999` never reaches the parent's copy, which stays `100`. That divergence is only possible if the identical virtual address is backed by different physical memory in each process, which is what copy-on-write isolation looks like. Virtual address equality is a coincidence of how `fork()` copies the address space layout; it says nothing about the underlying physical memory.

### Q2. What evidence demonstrates process isolation?

Every program runs in its own isolated environment. This is shown through hardware mechanisms, kernel behaviour, and direct software observation:

- Hardware architecture evidence: separate virtual address spaces, CPU privilege rings, per-process page tables and control register context switching.
- Runtime behaviour: a process's own crash (e.g. segmentation fault) does not take down unrelated processes.
- Software evidence: this experiment itself, `fork()` produces a child whose write to `secret` never appears in the parent.
- Hardware security vulnerabilities: some of the strongest evidence for process isolation comes from exploits that try to bypass it, such as Spectre and Meltdown, which leak isolated memory across process boundaries via microarchitectural side channels rather than through the normal memory-access path.

### Q3. Why is process isolation a security property rather than merely a programming convenience?

Programming convenience helps apps run smoothly together, whereas process isolation prevents untrusted applications from attacking each other or the system itself. Process isolation is a security property because it defines boundaries of trust:

- It enforces the principle of least privilege.
- It prevents horizontal escalation between unrelated processes.
- It guarantees system integrity and availability.
- It is enforced by hardware (the MMU and per-process page tables), not by trust or agreement between programs.

### Q4. What do PID and PPID tell you, and what do they not tell you?

PID is the Process ID, PPID is the Parent Process ID; together they represent the genealogy of running programs.

What PID and PPID tell you:

- Process identity: PID is the unique numeric handle the kernel assigns to a specific running process.
- Process relationships: PPID is the PID of the process that created (forked) this process.
- Process hierarchy: they establish parent-child lineage.
- Signal targets and control scope.
- Roughly, the chronological order in which processes were spawned.

What PID and PPID do not tell you:

- The program's identity or the code it's actually executing.
- Thread architecture within the process.
- Security context or permissions.
- A permanent identity, PIDs are recycled over time.
- Resource consumption or current state.
