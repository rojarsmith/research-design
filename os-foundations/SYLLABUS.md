# Build plan

**English** | [繁體中文](SYLLABUS.zh-TW.md) | [Course overview](README.md)

> **Draft for review, 2026-11-15.** None of this is built yet. Stages, hours and tools are proposals. Technical claims link to the official page they were checked against. The full list is under [Sources](#sources).

This is a build sequence, not a list of topics. Every stage lists **Prerequisites**, **Hours** (focused hours, reading included) and **Runs on**. Then come **Build**, **Learn as needed**, a **Linux / NT** comparison where it helps, and a **Checkpoint**. As in the graphics course, a stage is finished when you can explain what the code does, not just when it runs.

"MIT lab" refers to the public [6.1810 Fall 2025 labs](https://pdos.csail.mit.edu/6.1810/2025/schedule.html). Treat them as ready-made exercises. The builds below add what this course needs beyond them.

## Part I: OS foundations

### 00: Workbench, booting and debugging xv6

**Prerequisites:** C with pointers and structs, a Linux shell, git and make, and reading RISC-V assembly at the level of stage 1 in the [`c-compiler-foundations` path](../c-compiler-foundations/research/04-integration-path.zh-TW.md).
**Hours:** 4–8. **Runs on:** WSL 2 with Ubuntu 24.04.

**Build:** install WSL 2 and the packages from MIT's [tools page](https://pdos.csail.mit.edu/6.1810/2025/tools.html): `git build-essential gdb-multiarch qemu-system-misc gcc-riscv64-linux-gnu binutils-riscv64-linux-gnu`. The page asks for QEMU 7.2+ and GDB 8.3+. Clone [xv6-riscv](https://github.com/mit-pdos/xv6-riscv), run `make qemu`, then `make qemu-gdb` and attach `gdb-multiarch`. Step from `_entry` through `start()` to `main()`. Record every tool version in `NOTES.md`.

**Learn as needed:**
- QEMU's `virt` machine and the Makefile's `-machine virt -bios none -kernel … -m 128M -smp $(CPUS)`, where `CPUS` defaults to 3.
- Machine, supervisor and user modes.
- How `start()` uses `mret` to drop into supervisor mode.
- The kernel linker script.

**Checkpoint:** explain each QEMU flag. Show the first instruction xv6 executes and the point where it leaves machine mode. Compare this with reset to `main` on a microcontroller (stage 1 of [`mcu-foundations`](../mcu-foundations/02-learning-path.md)). What extra work exists because this machine has privilege levels?

### 01: System calls, crossing the boundary

**Prerequisites:** 00. **Hours:** 8–14. **Runs on:** WSL 2, plus Windows for the NT part. **MIT lab:** `syscall`.

**Build:** add a system call to xv6. Follow one call from the user stub (`ecall` in `usys.S`) through `uservec` in `trampoline.S`, `usertrap()` and `syscall()`, and back out through `userret`. Then pass a bad pointer from user space and show that `copyin`/`copyout` reject it. On Linux, run the same small program under `strace` once, then once more calling `syscall(2)` directly. On Windows, write the same program against Win32 and use a debugger to find the `Nt*` routine underneath it.

**Learn as needed:**
- The trap CSRs: `stvec`, `sepc`, `scause` and `sstatus`.
- The trampoline page and the trap frame.
- How syscall arguments are fetched.
- Why the kernel never trusts a user pointer.

**Linux / NT:** NT system services have Nt and Zw entry points. From user mode, a call is treated as untrusted and its parameters are probed. In kernel mode, the routine uses the thread's previous mode to decide whether to trust them ([Learn](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/using-nt-and-zw-versions-of-the-native-system-services-routines)). Win32 is the documented interface. Learn describes native calls such as `NtQueryInformationProcess` as internal and subject to change between releases ([Learn](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryinformationprocess)).

**Checkpoint:** draw `write(1, buf, n)` going into the kernel and back. Show every register saved, every stack switched and every page table in use. Explain what happens when `buf` points into kernel memory, once in xv6 terms and once in NT's previous-mode terms.

### 02: Processes, threads and context switches

**Prerequisites:** 01. **Hours:** 8–14. **Runs on:** WSL 2, Windows. **MIT lab:** `util`.

**Build:** a small pipeline in xv6 using `fork`, `exec`, `pipe` and `wait`. Trace `fork()` and `swtch()`. Then write user-level threads for xv6: a thread table, your own context switch in assembly, and a `yield`. On Linux, create threads with `pthread_create` and use `strace -f` to find the `clone` flags underneath.

**Learn as needed:**
- Process states, the process table and kernel stacks.
- Which registers `swtch` saves, and why caller-saved registers don't need saving. This is the calling convention from [`c-compiler-foundations`](../c-compiler-foundations/README.md).
- xv6's per-CPU scheduler thread.

**Linux / NT:** on Linux, a thread is a task created by `clone` with flags such as `CLONE_VM` and `CLONE_THREAD` ([clone(2)](https://man7.org/linux/man-pages/man2/clone.2.html)). NT separates the two. A process provides the resources (address space, handles, security context), and a thread is the unit that gets scheduled ([Learn](https://learn.microsoft.com/en-us/windows/win32/procthread/about-processes-and-threads)). NT has no `fork`, so compare `CreateProcess` with `fork` + `exec`.

**Checkpoint:** list what `swtch` saves and where it saves it. Explain why a switch from one process to another in xv6 passes through the scheduler thread. Explain what your user-level switch does not have to save.

### 03: Virtual memory

**Prerequisites:** 02. **Hours:** 14–24. **Runs on:** WSL 2, Windows. **MIT labs:** `pgtbl`, `cow`.

**Build:** print and walk an xv6 page table. Implement copy-on-write `fork`, and count the pages copied before and after. On Linux, map one file in two processes, once with `MAP_SHARED` and once with `MAP_PRIVATE`, and compare what each process sees. Read `/proc/self/maps`. Truncate the file under a live mapping and catch the `SIGBUS`. On Windows, reserve and commit memory with `VirtualAlloc`, and share a page-file-backed file mapping between two processes.

**Learn as needed:**
- Sv39 three-level page tables, PTE bits, the TLB and `sfence.vma`.
- Kernel and user page tables in xv6.
- Page-fault handling and demand paging.
- Reference counts for COW pages.
- `mmap` semantics ([mmap(2)](https://man7.org/linux/man-pages/man2/mmap.2.html)).

**Linux / NT:** Learn says a file mapping object is also known as a section object ([Learn](https://learn.microsoft.com/en-us/windows/win32/memory/file-mapping)). Reserve and commit are separate steps on NT ([Learn](https://learn.microsoft.com/en-us/windows/win32/memory/reserving-and-committing-memory)). Linux raises `SIGBUS` when a process touches a mapped page beyond the end of the file.

**Checkpoint:** translate one virtual address by hand, then confirm it in GDB. Show the pages COW saved. Explain why the `SIGBUS` case is a security problem for any server that maps memory a client handed it. Stage 17 comes back to this.

### 04: Interrupts, timers and device drivers

**Prerequisites:** 03. **Hours:** 12–20. **Runs on:** WSL 2 for xv6, a Linux VM for the module, Windows for observation only. **MIT labs:** `traps`, `net`.

**Build:** in xv6, follow a UART interrupt from the PLIC to `devintr()` and into the UART driver. Also follow the timer interrupt that forces preemption. In a Linux VM, write a small kernel module with a character device and a periodic kernel timer that wakes blocked readers. Each `read()` blocks until the next tick and returns a sequence number and a timestamp. This is a miniature of the DRM event stream you will read in stage 12.

**Learn as needed:**
- Enabling and disabling interrupts, and the PLIC.
- Top halves and deferred work.
- Linux threaded interrupt handlers (`request_threaded_irq`, [genericirq](https://docs.kernel.org/core-api/genericirq.html)).
- The Linux device model of buses, devices and drivers, exported through sysfs ([driver model](https://docs.kernel.org/driver-api/driver-model/overview.html)).
- udev.

**Linux / NT:**
- NT code runs at an IRQL. An ISR finishes quickly and queues a DPC to complete the work at a lower IRQL ([DPC](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-dpc-objects)).
- At `DISPATCH_LEVEL` or above, a driver may not wait. Above `APC_LEVEL`, a page fault is fatal ([IRQL](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/managing-hardware-priorities)).
- Devices sit in device stacks: a PDO, exactly one function driver, and optional filter drivers ([Learn](https://learn.microsoft.com/en-us/windows-hardware/drivers/gettingstarted/device-nodes-and-device-stacks)).
- WDM is no longer the recommended model. New drivers use KMDF or UMDF ([WDM](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-wdm), [WDF](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/)).
- Loading a test-signed NT driver needs `TESTSIGNING` and may mean turning off Secure Boot ([Learn](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/the-testsigning-boot-configuration-option)). So any NT driver work stays in a VM, never on the main desktop.

**Checkpoint:** for xv6, Linux and NT, name what may and may not sleep in interrupt context, and why. Show your module's blocked reader waking on every tick.

### 05: Synchronization

**Prerequisites:** 04. **Hours:** 12–20. **Runs on:** WSL 2, Windows. **MIT lab:** `lock`.

**Build:**
- Measure and reduce lock contention in xv6.
- Read `acquire()` and explain `push_off()`.
- Write a broken `sleep`/`wakeup` pair that loses a wakeup, then fix it.
- On Linux, build a mutex on `futex(2)` and compare it with `pthread_mutex` under contention.
- On Windows, build the same test with an SRW lock and with `WaitOnAddress`.

**Learn as needed:**
- C11 atomics and memory ordering, and the RISC-V `amoswap` and `fence` instructions.
- Spinlocks vs sleep locks, and why an xv6 spinlock disables interrupts on its own CPU.
- Lock ordering and deadlock.
- Linux lockdep ([lockdep](https://docs.kernel.org/locking/lockdep-design.html)) and RCU for read-mostly data ([RCU](https://docs.kernel.org/RCU/whatisRCU.html)).

**Linux / NT:**
- A futex keeps uncontended locking in user space and enters the kernel only to wait ([futex(2)](https://man7.org/linux/man-pages/man2/futex.2.html)).
- NT kernel spin locks must be held briefly and live in nonpaged memory ([spin locks](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-spin-locks)). Waiting on dispatcher objects is not allowed at `DISPATCH_LEVEL` or above ([dispatcher objects](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-kernel-dispatcher-objects)).
- `WaitOnAddress` resembles a futex, but its wakers must be in the same process ([Learn](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress)). SRW locks also work within one process only ([Learn](https://learn.microsoft.com/en-us/windows/win32/sync/slim-reader-writer--srw--locks)).

**Checkpoint:** show contention before and after, with numbers. Describe a deadlock that lockdep would report. Explain why a cross-process lock on Windows needs something other than `WaitOnAddress`.

### 06: Scheduling and latency

**Prerequisites:** 05. **Hours:** 10–16. **Runs on:** WSL 2 and Windows. The hardware validation repeats the measurement on the Linux PC.

**Build:** replace xv6's round-robin loop with a priority or lottery scheduler. Test it with one CPU-bound and one interactive workload. On Linux, write a thread that wakes 60 times a second via `timerfd` while the machine is under load. Plot how late each wakeup is, first under the default policy and then under `SCHED_FIFO`. Repeat on Windows at different thread priorities, and record one run with WPR for inspection in WPA ([WPT](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/)).

**Learn as needed:**
- Timer-driven preemption, run queues, and fairness vs latency.
- Linux's fair scheduler: Linux began moving from CFS to EEVDF in 6.6 ([EEVDF](https://docs.kernel.org/scheduler/sched-eevdf.html)).
- Real-time classes, and `SCHED_DEADLINE`, which is EDF with a constant bandwidth server ([docs](https://docs.kernel.org/scheduler/sched-deadline.html)).
- `sched_ext`, which lets BPF programs define a scheduler and has no stable ABI ([docs](https://docs.kernel.org/scheduler/sched-ext.html)).

**Linux / NT:** NT schedules threads by priority, 0 to 31. Threads at the same level take round-robin turns, and a higher level preempts a lower one ([Learn](https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities)). Threads with a base priority of 0–15 get boosts for the foreground process, window input and completed waits ([Learn](https://learn.microsoft.com/en-us/windows/win32/procthread/priority-boosts)).

**Checkpoint:** produce a latency histogram and explain its tail. Connect it to the display: a compositor that wakes up late misses vblank and drops a frame, so scheduling is part of display quality. WSL 2 runs in a VM, so treat its numbers as relative.

### 07: File systems

**Prerequisites:** 03, 05. **Hours:** 12–20. **Runs on:** WSL 2. **MIT labs:** `fs`, `mmap`.

**Build:** extend the xv6 file system and implement file-backed `mmap`. Kill QEMU in the middle of a write, then show that the log recovers a consistent file system on the next boot.

**Learn as needed:**
- The buffer cache and write-ahead logging.
- Inodes, directories and path lookup.
- File descriptors vs open file descriptions.
- The Linux VFS and page cache.

**Linux / NT:** NT offers file-backed sharing through the file mapping objects from stage 03. For each system, find who decides when dirty mapped pages reach the disk.

**Checkpoint:** walk through the crash recovery step by step. Explain the difference between an fd and the open file description behind it. Stage 08 passes the open file description between processes.

### 08: IPC, shared memory and event loops

**Prerequisites:** 03, 07. **Hours:** 12–20. **Runs on:** WSL 2, Windows.

**Build on Linux:**
1. A Unix-domain socket server that receives fds with `SCM_RIGHTS` and checks the peer with `SO_PEERCRED` ([unix(7)](https://man7.org/linux/man-pages/man7/unix.7.html)).
2. A shared buffer from `memfd_create` with `MFD_ALLOW_SEALING`. The receiver refuses any buffer that could still shrink ([memfd_create(2)](https://man7.org/linux/man-pages/man2/memfd_create.2.html)).
3. One single-threaded `epoll` loop serving the socket, a `timerfd`, a `signalfd` and an `eventfd` ([epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html), [timerfd_create(2)](https://man7.org/linux/man-pages/man2/timerfd_create.2.html), [signalfd(2)](https://man7.org/linux/man-pages/man2/signalfd.2.html), [eventfd(2)](https://man7.org/linux/man-pages/man2/eventfd.2.html)).

**Build on Windows:**
1. A named-pipe server.
2. A page-file-backed file mapping handed to the client with `DuplicateHandle` ([Learn](https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-duplicatehandle), [named shared memory](https://learn.microsoft.com/en-us/windows/win32/memory/creating-named-shared-memory)). Find the mapping handle in both processes with [Process Explorer](https://learn.microsoft.com/en-us/sysinternals/downloads/process-explorer).
3. A GUI thread that waits on handles and window messages together with `MsgWaitForMultipleObjectsEx` ([Learn](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-msgwaitformultipleobjectsex)).
4. Optionally, an I/O completion port version ([Learn](https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports)).

**Learn as needed:**
- Pipes vs sockets vs shared memory.
- Passing an fd as passing a capability.
- Why untrusted peers need sealing: without it, a peer can rewrite the memory at any time or shrink it and cause `SIGBUS`.
- Readiness models (`epoll`) vs completion models (IOCP, [io_uring](https://man7.org/linux/man-pages/man7/io_uring.7.html)).
- Per-thread message queues on Windows ([Learn](https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues)).

**Checkpoint:** show that a sealed buffer cannot be shrunk by its sender. Explain readiness vs completion. Read libwayland's event loop source and say which model it uses.

### 09: Part I project, a headless display server

**Prerequisites:** 06 and 08. Also the graphics course's T4 compositing module if it exists; otherwise a stub. **Hours:** 20–35. **Runs on:** WSL 2.

**Build:** a server and at least two clients that speak a small protocol you design over a Unix socket. Each client creates a sealed memfd buffer, sends it with `SCM_RIGHTS`, and then sends attach, damage and commit messages. The server:
- composes the client buffers into an offscreen framebuffer, using the graphics course's compositing module or a stub that blends premultiplied ARGB8888;
- ticks a simulated 60 Hz vblank with `timerfd`;
- sends each client a "frame done" event;
- writes the frames to image files.

Then feed it bad clients: one crashes mid-frame, one never reads its socket, one sends damage outside its buffer, and one sends an unsealed buffer and shrinks it.

**Learn as needed:**
- Protocol design: objects, requests and events, versioning.
- Per-client state and back-pressure.
- Why a server must never block on a client.

**Checkpoint:** the server survives every bad client and keeps its frame cadence. Write one page comparing your protocol with Wayland's `wl_surface` attach, damage, commit and frame callback ([protocol](https://wayland.freedesktop.org/docs/html/apa.html)).

## Part II: Window and display system

Before starting Part II, finish the graphics course's Part I through lab 05. By stage 18, the 2D, text and compositing rendering track in its [Part II draft](../computer-graphics-practice/SYLLABUS-PART2.md) should have reached T4 (compositing). Until then, the stage 09 stub stands in.

**Where Part II runs.** Stages 10 and 11 run on Windows. Stages 12–18 run in a Linux VM, with QEMU's virtio-gpu as the display, and VKMS when you need overlay planes or writeback. Rendering in the guest is software-only. Each stage marks what a VM cannot show as a **Hardware check**. Those items are collected in [Hardware validation](#hardware-validation-linux-pc) after stage 18. The README compares the VM display drivers.

### 10: Window system architecture, measured on Windows

**Prerequisites:** Part I. **Hours:** 8–12. **Runs on:** Windows 11 on the RTX 5070 Ti desktop.

**Build:** a small D3D11 or D3D12 app with a flip-model swapchain, or the graphics course's native app. Capture it with [PresentMon](https://github.com/GameTechDev/PresentMon) three ways: windowed, borderless full screen, and with another window overlapping it. Record each run's PresentMode, for example `Composed: Flip`, `Hardware: Independent Flip` or `Hardware Composed: Independent Flip`. Draw the presentation paths of X11, Wayland and Windows side by side.

**Learn as needed:**
- **X11 vs Wayland.** Under X, the X server sits between the clients and a separate compositor. Under Wayland, the compositor is the display server: it owns KMS and evdev, and clients render into their own buffers ([architecture](https://wayland.freedesktop.org/architecture.html)).
- **Windows layers.** WDDM has three parts: a vendor user-mode driver loaded by the Direct3D runtime, a vendor kernel-mode miniport driver, and the DirectX graphics kernel `Dxgkrnl` ([WDDM architecture](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/windows-vista-and-later-display-driver-model-architecture)).
- **Flip model.** Swapchain buffers are shared with DWM, which composes them without copying ([flip model](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model)). DWM can let a qualifying window flip directly (Direct Flip, Independent Flip) or give it a hardware overlay plane ([DirectX blog](https://devblogs.microsoft.com/directx/dxgi-flip-model/)).
- **Overlays and resets.** Multiplane overlays arrived with WDDM 1.3 in Windows 8.1 ([MPO](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/multiplane-overlay-support)). If the GPU is unresponsive past the default two-second timeout, TDR resets it ([TDR](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/timeout-detection-and-recovery)).
- **Why DWM can't be replaced.**
  - Since Windows 8, DWM can no longer be disabled programmatically ([DwmEnableComposition](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmenablecomposition)).
  - The only documented route for a custom compositor is `Windows.Devices.Display.Core`, and it only covers displays DWM ignores. Those are head-mounted displays and monitors marked as specialized. They leave the desktop and get no shell and no mouse or touch input ([namespace](https://learn.microsoft.com/en-us/uwp/api/windows.devices.display.core), [specialized displays](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/specialized-monitors)).
  - Indirect display drivers (IddCx) add monitors such as remote, virtual or USB-attached displays, and receive desktop frames from the OS ([IddCx](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/indirect-display-driver-model-overview)). They feed the desktop compositor rather than replace it.
  - So on Windows you can write a compositor for a dedicated display, but never for the desktop. That is why this course's compositor capstone runs on Linux.

The older [DWM overview](https://learn.microsoft.com/en-us/windows/win32/dwm/dwm-overview) page still describes the DWM service as one that can be switched on and off. The API page carries the Windows 8+ statement.

**Checkpoint:** use your PresentMon logs to explain which frames DWM composed, which reached the screen without composition, and why each switch happened. List what a Windows developer can and cannot change in this path. Then connect it to [`display-wake-research`](../display-wake-research/README.md): what does the Win+Ctrl+Shift+B graphics reset restart?

### 11: Wayland from the client side

**Prerequisites:** 08, 10. **Hours:** 10–16. **Runs on:** WSL 2 with WSLg. No Linux PC needed.

**Build:** a toolkit-free client on libwayland. It should:
- bind globals from the registry;
- create a `wl_shm` pool from a memfd;
- draw ARGB8888 with premultiplied alpha;
- show an `xdg_toplevel`;
- redraw only on frame callbacks;
- read keyboard and pointer input through `wl_seat`, with xkbcommon for the keymap.

Run it with `WAYLAND_DEBUG=1` and read the wire log.

**Learn as needed:**
- Objects, interfaces, requests and events.
- The registry, and double-buffered surface state that takes effect on commit.
- `wl_shm` formats. They reuse DRM fourcc codes except ARGB8888 and XRGB8888, the two formats every renderer should support ([protocol](https://wayland.freedesktop.org/docs/html/apa.html)).
- Premultiplied alpha. The `wl_buffer` text defines it on the encoded (electrical) values.
- Frame callbacks as the redraw throttle.
- The stable, staging and unstable tiers of wayland-protocols.
- [The Wayland Protocol](https://wayland-book.com/) book (a draft, CC BY-SA 4.0) works as a companion.

**Linux / NT:** WSLg is itself a Wayland compositor. It runs Weston with XWayland, forwards to Windows over RDP, and presets `WAYLAND_DISPLAY` for your distribution ([WSLg](https://github.com/microsoft/wslg), [Learn](https://learn.microsoft.com/en-us/windows/wsl/tutorials/gui-apps)). Its frames pass through system memory, so draw no timing conclusions here.

**Checkpoint:** explain every line of one frame in the `WAYLAND_DEBUG` log. Show that the client stops drawing when its frame callbacks stop. Map each message of your stage 09 protocol to the matching Wayland request.

### 12: DRM/KMS and atomic modesetting

**Prerequisites:** 04, 11. **Hours:** 14–24. **Runs on:** a Linux VM. Use QEMU's [virtio-gpu](https://www.qemu.org/docs/master/system/devices/virtio/virtio-gpu.html), which needs a guest kernel with `CONFIG_DRM_VIRTIO_GPU`, or VKMS. virtio-gpu is an atomic driver with a render node, and gives each output one primary and one cursor plane ([virtgpu_drv.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/virtio/virtgpu_drv.c), [virtgpu_display.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/virtio/virtgpu_display.c)).

**Build:** inspect the device with `drm_info` and with `modetest` from libdrm's test tools. Then write a C program that:
- opens the primary node as DRM master;
- lists the connectors, CRTCs, planes and their properties;
- allocates dumb buffers;
- lights up the screen with one atomic commit.

Use test-only commits to probe which configurations the driver accepts, and double-buffer with page-flip events. Read `kmscube` as a reference for KMS with GBM and EGL. Run it wherever the guest's Mesa can render it.

**Learn as needed:**
- The KMS pipeline: framebuffer → plane → CRTC → encoder → connector ([KMS overview](https://docs.kernel.org/gpu/drm-kms.html#overview)).
- Properties and blobs.
- Atomic commits and test-only commits ([atomic](https://docs.kernel.org/gpu/drm-kms.html#atomic-mode-setting)).
- Plane types. To atomic clients, `type` is only a hint, and each CRTC has its own primary plane ([plane abstraction](https://docs.kernel.org/gpu/drm-kms.html#plane-abstraction)).
- Format modifiers and the `IN_FORMATS` property ([format handling](https://docs.kernel.org/gpu/drm-kms.html#format-modifiers)).
- DRM master vs render nodes. Render nodes give unprivileged GPU access but no modesetting ([drm-uapi](https://docs.kernel.org/gpu/drm-uapi.html#render-nodes)).

VKMS is a software-only model of a KMS driver. Through configfs you can create instances with the planes, CRTCs and connectors you choose, and it simulates vblank with timers ([VKMS](https://docs.kernel.org/gpu/vkms.html#configuring-with-configfs)). It is good for practicing the API and useless for timing.

**Checkpoint:** draw the KMS graph of your machine from `drm_info` output. Explain why the program must run from a text console with no other compositor holding DRM master ([DRM master](https://docs.kernel.org/gpu/drm-uapi.html#primary-nodes-drm-master-and-authentication)). Show one test-only rejection and explain it.

### 13: Planes, page flips, vblank, VRR and hotplug

**Prerequisites:** 06, 12. **Hours:** 12–20. **Runs on:** a Linux VM, using virtio-gpu for flips and hotplug and VKMS for overlay planes.

**Build:**
1. **Flip events.** Log each flip-completion event's sequence number and timestamp against your submit time, then miss a vblank on purpose. DRM has used monotonic timestamps in these events since Linux 4.15 ([drm_vblank.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/drm_vblank.c)). In a VM the numbers follow the host's timers. Learn the API here and read real timing in the hardware validation.
2. **Planes.** Configure a VKMS instance with an overlay plane through configfs. Place a surface on it with test-only commits and capture the composed result through the writeback connector ([VKMS](https://docs.kernel.org/gpu/vkms.html#configuring-with-configfs)). Record which configurations VKMS rejects.
3. **Hotplug.** Listen for DRM uevents. They carry `HOTPLUG=1`, plus `CONNECTOR=<id>` for per-connector events ([drm_sysfs.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/drm_sysfs.c)). virtio-gpu turns display changes on the host, such as resizing or enabling a QEMU output, into hotplug events ([virtgpu_kms.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/virtio/virtgpu_kms.c)). Re-probe the connectors and pick a new mode.
4. **VRR and link status.** Read the properties and write the code paths. Set `VRR_ENABLED` on the CRTC only when the connector reports `vrr_capable`. When `link-status` turns `BAD`, choose a valid mode and commit it again. The VM has no VRR monitor and no cable to fail, so these paths are exercised in the hardware validation.

**Learn as needed:**
- Scanout and blanking, vblank counters, and page-flip events.
- How VRR stretches the vertical front porch until the next flip, and why large swings can cause stutter, or brightness flicker on some panels ([VRR properties](https://docs.kernel.org/gpu/drm-kms.html#variable-refresh-properties)).
- Link status ([connector properties](https://docs.kernel.org/gpu/drm-kms.html#standard-connector-properties)).
- Cursor size limits.

**Hardware check:**
- Real flip intervals.
- The planes the real GPU accepts. NVIDIA registers only primary and cursor planes ([README](https://download.nvidia.com/XFree86/Linux-x86_64/620.70.06/README/kms.html)), so overlay planes need an Intel or AMD GPU.
- VRR on a capable monitor. The ASUS VG289Q lists 40–60 Hz ([spec](https://www.asus.com/displays-desktops/monitors/tuf-gaming/tuf-gaming-vg289q/techspec/)).
- Pulling a real cable, including recovery from `link-status` `BAD`.

**Checkpoint:** while the program runs, disable and re-enable the QEMU output, and recover without restarting. Show an overlay-plane composition captured from VKMS writeback. Compare with `display-wake-research`, where Windows may treat a sleeping DisplayPort monitor as disconnected. Which KMS events would that produce on Linux?

### 14: Buffer sharing and synchronization

**Prerequisites:** 08 and 13, plus the graphics course's GPU stages. **Hours:** 16–26. **Runs on:** a Linux VM, plus Windows for the D3D comparison.

**Build:** allocate a buffer with GBM (or a dumb buffer) and render into it. In the VM the rendering is software-only, but virtio-gpu supports DRM syncobj with timelines ([virtgpu_drv.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/virtio/virtgpu_drv.c)), so all three runs below work there. Export it as a dma-buf and send it to a second process over `SCM_RIGHTS`. There, import it as a KMS framebuffer with its modifier and scan it out. Run it three ways:
1. With implicit sync.
2. With explicit sync: attach a sync_file through the plane's `IN_FENCE_FD` and receive one back from `OUT_FENCE_PTR`.
3. With DRM syncobj timelines.

Remove a fence on purpose and capture the corrupted frame. On Windows, share a D3D12 resource and fence between two processes with `CreateSharedHandle` and `DuplicateHandle` ([D3D12](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-createsharedhandle), [DXGI](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgiresource1-createsharedhandle)).

**Learn as needed:**
- GEM and PRIME ([PRIME](https://docs.kernel.org/gpu/drm-mm.html#prime-buffer-sharing)).
- dma-buf, dma-fence and dma-resv ([dma-buf](https://docs.kernel.org/driver-api/dma-buf.html)).
- sync_file, which carries fences between drivers and processes ([sync_file](https://docs.kernel.org/driver-api/sync_file.html)).
- `DMA_BUF_IOCTL_EXPORT_SYNC_FILE` and `DMA_BUF_IOCTL_IMPORT_SYNC_FILE`, which bridge explicit and implicit sync ([ioctls](https://docs.kernel.org/driver-api/dma-buf.html#dma-buffer-ioctls)).
- The KMS explicit fencing properties ([KMS](https://docs.kernel.org/gpu/drm-kms.html#explicit-fencing-properties)).
- DRM sync objects, binary and timeline ([syncobj](https://docs.kernel.org/gpu/drm-mm.html#drm-sync-objects)).
- Wayland's `linux-dmabuf-v1` (stable) and `linux-drm-syncobj-v1` (staging).
**Hardware check:** repeat with GPU rendering and real modifiers. On NVIDIA, explicit sync uses syncobj timeline points and needs kernel 6.8+ ([README](https://download.nvidia.com/XFree86/Linux-x86_64/620.70.06/README/wayland-issues.html)).

**Checkpoint:** draw one buffer's life: client render, fence, compositor import, scanout, release, reuse. Draw the same for the Windows shared handle and fence. Explain what breaks if a compositor releases a buffer before scanout has finished.

### 15: The input stack

**Prerequisites:** 08, 12. **Hours:** 10–16. **Runs on:** a Linux VM. The VM's virtual keyboard and mouse appear as evdev devices, and uinput covers the rest.

**Build:** read `/dev/input/eventX` directly and print the `EV_KEY`, `EV_REL` and `EV_ABS` events grouped into `SYN_REPORT` packets. Replace that with libinput, then add xkbcommon for keymaps and modifiers. Create a virtual device with uinput to script repeatable tests. Add pointer and keyboard focus to your stage 09 server, so that events reach only the focused client.

**Learn as needed:**
- The evdev event model ([input](https://docs.kernel.org/input/input.html), [event codes](https://docs.kernel.org/input/event-codes.html)).
- Exclusive grabs with `EVIOCGRAB` ([input.h](https://github.com/torvalds/linux/blob/master/include/uapi/linux/input.h)).
- uinput ([docs](https://docs.kernel.org/input/uinput.html)).
- libinput, which runs inside the compositor, reads the event devices, handles device quirks for touchpads, tablets and more, and is never seen by clients ([libinput](https://wayland.freedesktop.org/libinput/doc/latest/what-is-libinput.html)).
- xkbcommon for keymaps and keyboard state ([xkbcommon](https://xkbcommon.org/)).
- How Wayland delivers the keymap through `wl_keyboard`.

**Linux / NT:** with Raw Input, a Windows app registers for devices, receives `WM_INPUT`, and can tell two mice apart ([Learn](https://learn.microsoft.com/en-us/windows/win32/inputdev/about-raw-input)). Ordinary input arrives as window messages in a thread's queue.

**Checkpoint:** measure input-to-scanout latency from your logs, from the event timestamp to the flip that shows its effect. The VM number is only relative, and the hardware validation repeats it. Explain why the compositor, not each client, must own the input devices.

### 16: Seats and session management

**Prerequisites:** 12, 15. **Hours:** 6–10. **Runs on:** a Linux VM. If the host captures the Ctrl+Alt+F keys, switch VTs with `chvt`.

**Build:** run the stage 12 program as a normal user through libseat, with either the logind or the seatd backend. Handle VT switches. On pause, stop committing and release DRM master. On resume, take the new fds and restore state. Confirm that no input arrives while the session is inactive.

**Learn as needed:**
- logind's session API: `TakeControl`, `TakeDevice`, `PauseDevice`, `PauseDeviceComplete` and `ResumeDevice` ([logind](https://www.freedesktop.org/software/systemd/man/latest/org.freedesktop.login1.html)).
- libseat and its backends, as Weston uses them ([Weston](https://wayland.pages.freedesktop.org/weston/toc/running-weston.html)), and [seatd](https://git.sr.ht/~kennylevinsen/seatd).
- DRM master hand-off with `SET_MASTER` and `DROP_MASTER` ([drm-uapi](https://docs.kernel.org/gpu/drm-uapi.html#primary-nodes-drm-master-and-authentication)).
- Revocable input fds via `EVIOCREVOKE` ([input.h](https://github.com/torvalds/linux/blob/master/include/uapi/linux/input.h)).

**Linux / NT:** all Windows services run in session 0 and cannot interact with the user directly ([Learn](https://learn.microsoft.com/en-us/windows/win32/services/interactive-services)). Window stations and desktops are securable objects. They scope the clipboard, windows, menus and hooks ([Learn](https://learn.microsoft.com/en-us/windows/win32/winstation/about-window-stations-and-desktops)).

**Checkpoint:** switch VTs and back ten times. Afterwards there should be no leaked DRM master, no stuck input and no lost frames. Explain who grants device access in each design.

### 17: Isolation and security for client buffers

**Prerequisites:** 09, 14, 16. **Hours:** 8–14. **Runs on:** a Linux VM, WSL 2 for the shared-memory parts, Windows for comparison.

**Build:** attack your own server, then fix it. Write clients that:
- shrink their shm pool;
- send unsealed memfds;
- flood requests;
- send a dma-buf with the wrong size or modifier;
- ask for another client's buffer;
- submit a GPU job that never ends. Watch TDR on Windows now, and the Linux GPU reset in the hardware validation.

Then add proper screen capture through xdg-desktop-portal ScreenCast. There, `Start` typically asks the user and returns PipeWire streams ([portal](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.ScreenCast.html)). Learn what the privileged capture protocol `ext-image-copy-capture-v1` (staging) is for.

**Learn as needed:**
- Every request and buffer from a client is untrusted input.
- Sealing and `SIGBUS` handling.
- Validating dma-buf imports.
- GPU robustness and reset.
- `security-context-v1` (staging) for sandboxed clients.
- In the X11 and Wayland specs, find which requests would let one client read another client's pixels or inject input.

**Linux / NT:** UIPI blocks window messages from lower-integrity senders ([Learn](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-changewindowmessagefilterex)). `WDA_EXCLUDEFROMCAPTURE` (Windows 10 2004) hides a window from capture. It works only while DWM is composing, and Learn says it is not a security feature ([Learn](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowdisplayaffinity)).

**Checkpoint:** write a one-page threat model for your compositor: assets, untrusted inputs, mitigations and residual risks. Every attack client from the build runs in your test script without crashing the server.

### 18: Capstone, a minimal Wayland compositor on DRM/KMS

**Prerequisites:** 10–17. **Hours:** 60–120. **Runs on:** a Linux VM (QEMU with virtio-gpu) with software rendering. The hardware validation then runs it on a real display.

**Route (decided in review):** build your own backend from the stage 12–16 code (DRM/KMS, libinput, libseat, GBM) on top of libwayland-server. Write kernel-facing code in C; the compositor itself may be C++ to match the graphics course's core. First read [tinywl](https://gitlab.freedesktop.org/wlroots/wlroots/-/tree/master/tinywl), wlroots' minimal example compositor, as a map but not as a base. [wlroots](https://gitlab.freedesktop.org/wlroots/wlroots) is not used as the backend.

**Build, in milestones:**
1. **One client.** `wl_compositor`, `wl_shm` and `xdg_wm_base`. A full-screen toplevel is composed by the graphics module into a KMS framebuffer, and page-flip events drive the frame callbacks.
2. **Several windows.** Stacking, focus, and move and raise with a key binding. Keyboard and pointer through `wl_seat`, and a cursor on the cursor plane.
3. **dma-buf clients.** `linux-dmabuf-v1` import, damage tracking, and explicit sync with `linux-drm-syncobj-v1` where the driver supports it. In the VM, test with the buffer-sharing client from stage 14. GPU-rendered clients and GPU composition come with the hardware validation.
4. **Timing.** `presentation-time` feedback, and direct scanout of a full-screen client buffer when its format and modifier allow.
5. **Robustness.** Client crashes, hotplug, VT switches, GPU resets, and the stage 17 attack clients.

Real apps to reach, in order:
1. [Weston](https://wayland.pages.freedesktop.org/weston/)'s demo clients.
2. [`foot`](https://packages.debian.org/sid/foot), a minimal Wayland terminal.
3. A Vulkan demo such as `vkcube`. In the VM this needs a software Vulkan driver from the guest's Mesa; otherwise it moves to the hardware validation.
4. A GTK 4 or Qt 6 application. Toolkits use more of xdg-shell (popups, configure sequences) and more protocols, so expect to keep adding globals until the app starts.

Xwayland is optional.

**Checkpoint:** record a session:
- boot to a text console and start the compositor as a normal user;
- open a terminal, a toolkit app and a Vulkan demo;
- move windows, switch VT and back, disable and re-enable an output, kill a client.

Write the design note. It traces one client frame from `wl_surface.commit` to scanout and back to the frame callback, with measured timestamps, and includes the threat model from stage 17. In the VM the timestamps are relative.

## Hardware validation (Linux PC)

**Prerequisites:** stage 18 working in the VM. **Hours:** 15–30. **Runs on:** Linux with direct control of a real display. That is a dual-boot install on the RTX 5070 Ti desktop or a separate PC (README open question 1).

Run the compositor and the stage programs on real hardware, and collect what the VM could not show:

1. **Flip timing:** frame intervals against vblank, missed deadlines, and how the stage 06 scheduling choices affect them.
2. **Planes:** which planes the real GPU accepts. NVIDIA registers only primary and cursor planes ([README](https://download.nvidia.com/XFree86/Linux-x86_64/620.70.06/README/kms.html)), so overlay planes need an Intel or AMD GPU.
3. **VRR:** on a VRR-capable monitor (the ASUS VG289Q lists 40–60 Hz), frame intervals with and without `VRR_ENABLED`.
4. **Hotplug:** pull a real cable, recover, and handle `link-status` going `BAD`.
5. **GPU buffers:** stage 14 and capstone milestone 3 with GPU rendering, GPU composition and real modifiers. On NVIDIA, this needs the open kernel modules, and explicit sync needs kernel 6.8+ ([README](https://download.nvidia.com/XFree86/Linux-x86_64/620.70.06/README/wayland-issues.html)).
6. **Latency and resets:** input-to-photon latency from stage 15, and a GPU reset from stage 17.
7. **Capstone again:** the stage 18 checkpoint, with timestamps that now mean something.

**Checkpoint:** the design note gains a table comparing VM and hardware numbers for the same runs, with an explanation of each difference.

## Interface with the graphics course

The compositor calls the graphics course's composition module once per output frame. That module is T4 (compositing) of the 2D, text and compositing rendering track in the graphics course's [Part II draft](../computer-graphics-practice/SYLLABUS-PART2.md). Text comes from T3, and color handling from C5 and T5. Both documents are drafts, so treat this as a proposal for both sides to agree on. T4 already takes surfaces with position, size, opacity, transform and a rounded clip. This interface adds what a system compositor needs.

**In:** a list of surfaces. Each surface has:
- a buffer: CPU memory for `wl_shm`, or a GPU image for dma-buf;
- its DRM fourcc format and modifier;
- size, position, scale and transform;
- opacity and clip;
- a damage region and a stacking order;
- an acquire fence, for GPU buffers.

**Out:** a frame written into a target buffer (a KMS framebuffer or a GBM buffer), the region that changed, and a release fence.

**Rules both sides must agree on:**
- Wayland buffers carry alpha premultiplied on the encoded values, while T4 blends premultiplied colors in linear light. So each Wayland surface must be un-premultiplied, decoded and premultiplied again in linear light before blending, and the result encoded for the output. Decide whether the compositor or T4 does this conversion, and state it in the design note. T4's checkpoint already shows where encoded and linear blending differ.
- The module needs a backend that runs on Linux. The graphics course's Part II lets you choose Vulkan or Direct3D 12, and only Vulkan, or a CPU path, can run here. In the VM, Vulkan means a software driver unless the guest has 3D acceleration. A CPU compositing path, in the spirit of T1's CPU rasterizer, keeps the VM route usable.
- ARGB8888 and XRGB8888 must work. Every other format and modifier pair is optional and must be negotiated.
- KMS's plane "pixel blend mode" property distinguishes premultiplied and coverage blending. When the compositor hands a surface straight to a hardware plane, the plane must match what the module would have computed.
- Text (window titles, debug overlays) comes from T3.
- The module must not block. It receives a deadline (the next vblank minus a margin) and reports how long it took.

Until T4 exists, stage 09 uses a stub with the same interface.

## Optional side quests

- **rv32sim privileged mode.** Add machine-code decoding, CSRs, trap entry and return, and Sv32 paging to the [`c-compiler-foundations`](../c-compiler-foundations/README.md) simulator. Then boot a tiny kernel of your own on it. This ties the two folders together and takes roughly 30–60 hours.
- **NT driver in a VM.** A KMDF driver in a Windows VM with test signing.
- **HDR and color.** Current kernel docs document the KMS per-plane color pipeline ([colorop](https://docs.kernel.org/gpu/drm-kms.html#colorop-abstraction)), and NVIDIA lists support for it from Linux 6.19. Combine it with Wayland's `color-management-v1` (staging) and the graphics course's C5 and T5 stages.
- **Xwayland** for X11 applications.
- **NVK.** On the same card, compare Mesa's [NVK](https://docs.mesa3d.org/drivers/nvk.html), which lists consumer Blackwell support, with NVIDIA's driver.

## Suggested study sessions

Split each stage into short cycles:

1. **Predict:** write down the expected trace, value or timing before changing code.
2. **Implement:** add one capability, and keep a known-good build to compare against.
3. **Inspect:** use GDB, `strace`, `WAYLAND_DEBUG`, `drm_info`, PresentMon or a timestamp log to see what actually happened.
4. **Explain:** write what happened and why, with the documentation page that defines the behavior.
5. **Break it:** inject one recoverable fault and record how it shows up.

## Sources

Checked on 2026-11-15. Versions, statuses and page contents change, so recheck before building. Pages marked *not re-read* were linked but refused automated requests on the check date.

**Teaching kernel and emulation**
- MIT 6.1810: [course home](https://pdos.csail.mit.edu/6.1810/) (Fall 2026 under construction), [2025 schedule](https://pdos.csail.mit.edu/6.1810/2025/schedule.html) (lab order), [2025 tools](https://pdos.csail.mit.edu/6.1810/2025/tools.html) (WSL 2 with Ubuntu 24.04, packages, QEMU 7.2+, GDB 8.3+), [xv6 book, RISC-V rev5](https://pdos.csail.mit.edu/6.1810/2025/xv6/book-riscv-rev5.pdf).
- [xv6-riscv repository](https://github.com/mit-pdos/xv6-riscv): the README, plus the Makefile's `QEMUOPTS`, `CPUS := 3` and `-march=rv64gc`.
- QEMU: [Windows downloads](https://www.qemu.org/download/#windows), [accelerators including WHPX](https://www.qemu.org/docs/master/system/introduction.html#virtualisation-accelerators), [virtio-gpu](https://www.qemu.org/docs/master/system/devices/virtio/virtio-gpu.html).
- WSL: [WSL 1 vs WSL 2](https://learn.microsoft.com/en-us/windows/wsl/compare-versions), [.wslconfig settings](https://learn.microsoft.com/en-us/windows/wsl/wsl-config) (`nestedVirtualization`, GPU para-virtualization), [GUI apps](https://learn.microsoft.com/en-us/windows/wsl/tutorials/gui-apps), [WSLg architecture](https://github.com/microsoft/wslg).

**Linux kernel documentation and sources**
- Scheduling: [EEVDF](https://docs.kernel.org/scheduler/sched-eevdf.html), [SCHED_DEADLINE](https://docs.kernel.org/scheduler/sched-deadline.html), [sched_ext](https://docs.kernel.org/scheduler/sched-ext.html).
- Core: [generic IRQ and threaded handlers](https://docs.kernel.org/core-api/genericirq.html), [driver model](https://docs.kernel.org/driver-api/driver-model/overview.html), [lockdep](https://docs.kernel.org/locking/lockdep-design.html), [RCU](https://docs.kernel.org/RCU/whatisRCU.html).
- Input: [input subsystem and evdev](https://docs.kernel.org/input/input.html), [event codes](https://docs.kernel.org/input/event-codes.html), [uinput](https://docs.kernel.org/input/uinput.html), [uapi input.h](https://github.com/torvalds/linux/blob/master/include/uapi/linux/input.h) (`EVIOCGRAB`, `EVIOCREVOKE`).
- KMS: [drm-kms](https://docs.kernel.org/gpu/drm-kms.html). Sections used: overview, atomic mode setting, plane abstraction, standard plane properties, explicit fencing, variable refresh, standard connector properties, format modifiers and colorop abstraction.
- DRM uAPI: [drm-uapi](https://docs.kernel.org/gpu/drm-uapi.html) (DRM master, render nodes), [drm-mm](https://docs.kernel.org/gpu/drm-mm.html) (PRIME, sync objects).
- Kernel sources behind docs too long to fetch: [drm_sysfs.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/drm_sysfs.c) (hotplug uevents), [drm_vblank.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/drm_vblank.c) (vblank handling, monotonic timestamps).
- Buffers and fences: [dma-buf, dma-fence, dma-resv and the sync-file ioctls](https://docs.kernel.org/driver-api/dma-buf.html), [sync_file](https://docs.kernel.org/driver-api/sync_file.html), [VKMS](https://docs.kernel.org/gpu/vkms.html).

**Linux man pages (man7.org)**
- [memfd_create(2)](https://man7.org/linux/man-pages/man2/memfd_create.2.html), [unix(7)](https://man7.org/linux/man-pages/man7/unix.7.html), [mmap(2)](https://man7.org/linux/man-pages/man2/mmap.2.html), [clone(2)](https://man7.org/linux/man-pages/man2/clone.2.html), [futex(2)](https://man7.org/linux/man-pages/man2/futex.2.html).
- [epoll(7)](https://man7.org/linux/man-pages/man7/epoll.7.html), [eventfd(2)](https://man7.org/linux/man-pages/man2/eventfd.2.html), [timerfd_create(2)](https://man7.org/linux/man-pages/man2/timerfd_create.2.html), [signalfd(2)](https://man7.org/linux/man-pages/man2/signalfd.2.html), [io_uring(7)](https://man7.org/linux/man-pages/man7/io_uring.7.html).

**Wayland and freedesktop.org**
- Wayland: [architecture](https://wayland.freedesktop.org/architecture.html), [protocol specification](https://wayland.freedesktop.org/docs/html/apa.html) (`wl_shm.format`, premultiplied alpha in `wl_shm` and `wl_buffer`, `wl_surface.frame`), [The Wayland Protocol book](https://wayland-book.com/) (a draft, CC BY-SA 4.0).
- [wayland-protocols](https://gitlab.freedesktop.org/wayland/wayland-protocols) (*not re-read*). Statuses were taken from the [wayland.app index](https://wayland.app/protocols/), which is generated from the same XML:
  - stable: xdg-shell, presentation-time, viewporter, linux-dmabuf-v1;
  - staging: linux-drm-syncobj-v1, security-context-v1, ext-image-copy-capture-v1, color-management-v1, fifo-v1, commit-timing-v1, tearing-control-v1;
  - unstable: xdg-decoration, linux-explicit-synchronization.
- Input and seats: [libinput](https://wayland.freedesktop.org/libinput/doc/latest/what-is-libinput.html), [xkbcommon](https://xkbcommon.org/), [logind D-Bus API](https://www.freedesktop.org/software/systemd/man/latest/org.freedesktop.login1.html), [seatd](https://git.sr.ht/~kennylevinsen/seatd) (*not re-read*).
- Compositors: [Weston, running and back-ends](https://wayland.pages.freedesktop.org/weston/toc/running-weston.html) (libseat with seatd or logind), [wlroots](https://gitlab.freedesktop.org/wlroots/wlroots) and [tinywl](https://gitlab.freedesktop.org/wlroots/wlroots/-/tree/master/tinywl) (*not re-read*).
- [xdg-desktop-portal ScreenCast](https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.ScreenCast.html).
- Tools, confirmed through Debian package pages: [kmscube](https://packages.debian.org/sid/kmscube), [drm-info](https://packages.debian.org/sid/drm-info), [libdrm-tests file list](https://packages.debian.org/sid/amd64/libdrm-tests/filelist) (ships `modetest`), [foot](https://packages.debian.org/sid/foot), [weston](https://packages.debian.org/sid/weston).

**Microsoft Learn and Microsoft**
- DWM and presentation: [DwmEnableComposition](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmenablecomposition), [DWM overview](https://learn.microsoft.com/en-us/windows/win32/dwm/dwm-overview) (older text), [WDDM architecture](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/windows-vista-and-later-display-driver-model-architecture), [multiplane overlays](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/multiplane-overlay-support), [DXGI flip model](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model), [DirectX blog on flip model](https://devblogs.microsoft.com/directx/dxgi-flip-model/), [PresentMon](https://github.com/GameTechDev/PresentMon).
- Displays outside the desktop: [Windows.Devices.Display.Core](https://learn.microsoft.com/en-us/uwp/api/windows.devices.display.core), [specialized displays](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/specialized-monitors), [IddCx](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/indirect-display-driver-model-overview), [TDR](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/timeout-detection-and-recovery).
- Sharing, capture and input: [DXGI CreateSharedHandle](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/nf-dxgi1_2-idxgiresource1-createsharedhandle), [D3D12 CreateSharedHandle](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12device-createsharedhandle), [SetWindowDisplayAffinity](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setwindowdisplayaffinity), [UIPI via ChangeWindowMessageFilterEx](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-changewindowmessagefilterex), [window stations and desktops](https://learn.microsoft.com/en-us/windows/win32/winstation/about-window-stations-and-desktops), [Raw Input](https://learn.microsoft.com/en-us/windows/win32/inputdev/about-raw-input).
- Kernel and drivers: [IRQL](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/managing-hardware-priorities), [DPC objects](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-dpc-objects), [Nt and Zw routines](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/using-nt-and-zw-versions-of-the-native-system-services-routines), [NtQueryInformationProcess](https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntqueryinformationprocess), [WDM](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-wdm), [WDF](https://learn.microsoft.com/en-us/windows-hardware/drivers/wdf/), [device stacks](https://learn.microsoft.com/en-us/windows-hardware/drivers/gettingstarted/device-nodes-and-device-stacks), [TESTSIGNING](https://learn.microsoft.com/en-us/windows-hardware/drivers/install/the-testsigning-boot-configuration-option).
- Processes and memory: [processes and threads](https://learn.microsoft.com/en-us/windows/win32/procthread/about-processes-and-threads), [scheduling priorities](https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities), [priority boosts](https://learn.microsoft.com/en-us/windows/win32/procthread/priority-boosts), [file mapping](https://learn.microsoft.com/en-us/windows/win32/memory/file-mapping), [named shared memory](https://learn.microsoft.com/en-us/windows/win32/memory/creating-named-shared-memory), [reserve and commit](https://learn.microsoft.com/en-us/windows/win32/memory/reserving-and-committing-memory).
- IPC, messages and services: [DuplicateHandle](https://learn.microsoft.com/en-us/windows/win32/api/handleapi/nf-handleapi-duplicatehandle), [I/O completion ports](https://learn.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports), [messages and message queues](https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues), [MsgWaitForMultipleObjectsEx](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-msgwaitformultipleobjectsex), [interactive services and session 0](https://learn.microsoft.com/en-us/windows/win32/services/interactive-services).
- Synchronization: [spin locks](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-spin-locks), [dispatcher objects](https://learn.microsoft.com/en-us/windows-hardware/drivers/kernel/introduction-to-kernel-dispatcher-objects), [SRW locks](https://learn.microsoft.com/en-us/windows/win32/sync/slim-reader-writer--srw--locks), [WaitOnAddress](https://learn.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-waitonaddress).
- Tools: [Windows Performance Toolkit](https://learn.microsoft.com/en-us/windows-hardware/test/wpt/), [Process Explorer](https://learn.microsoft.com/en-us/sysinternals/downloads/process-explorer).

**Virtual machines**
- Hyper-V: [Discrete Device Assignment](https://learn.microsoft.com/en-us/windows-server/virtualization/hyper-v/plan/plan-for-deploying-devices-using-discrete-device-assignment) (Windows Server framing, server-class hardware, Windows guests listed) and [enabling Hyper-V](https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/quick-start/enable-hyper-v) (Windows 11 Pro or Enterprise; the warning about other hypervisors).
- virtio-gpu guest driver: [virtgpu_drv.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/virtio/virtgpu_drv.c) (atomic, render node, syncobj with timelines), [virtgpu_display.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/virtio/virtgpu_display.c) (primary and cursor planes), [virtgpu_kms.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/virtio/virtgpu_kms.c) (host display events become hotplug).
- vmwgfx: [vmwgfx_drv.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/vmwgfx/vmwgfx_drv.c) and [vmwgfx_stdu.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/vmwgfx/vmwgfx_stdu.c) (atomic, render node, primary and cursor planes, no syncobj). The driver-features line and plane list came from a summarized part of each file.
- hyperv_drm: [hyperv_drm_drv.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/hyperv/hyperv_drm_drv.c) and [hyperv_drm_modeset.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/hyperv/hyperv_drm_modeset.c) (one primary plane, no render node).
- [VMware Fusion and Workstation free for all users](https://blogs.vmware.com/cloud-foundation/2024/11/11/vmware-fusion-and-workstation-are-now-free-for-all-users/) (2024-11-11).

**NVIDIA, Mesa and monitors**
- NVIDIA Linux driver 620.70.06 README: [KMS](https://download.nvidia.com/XFree86/Linux-x86_64/620.70.06/README/kms.html) (on by default; primary and cursor planes only; known issues), [open kernel modules](https://download.nvidia.com/XFree86/Linux-x86_64/620.70.06/README/kernel_open.html) (Blackwell only on open modules), [GBM](https://download.nvidia.com/XFree86/Linux-x86_64/620.70.06/README/gbm.html), [Wayland known issues](https://download.nvidia.com/XFree86/Linux-x86_64/620.70.06/README/wayland-issues.html) (explicit sync on kernel 6.8+, COLOR_PIPELINE on 6.19+, the simpledrm and `fbdev=1` note). The KMS chapter also says GBM submission to KMS is unsupported, which contradicts the GBM chapter, so verify it on the machine.
- [NVIDIA open GPU kernel modules](https://github.com/NVIDIA/open-gpu-kernel-modules) (Turing and later).
- [Mesa NVK](https://docs.mesa3d.org/drivers/nvk.html) (Kepler through Ada, plus consumer Blackwell).
- [ASUS VG289Q specifications](https://www.asus.com/displays-desktops/monitors/tuf-gaming/tuf-gaming-vg289q/techspec/) (Adaptive-Sync, 40–60 Hz).
