# Operating Systems Foundations: From Kernel to Compositor

**English** | [繁體中文](README.zh-TW.md) | [Build plan](SYLLABUS.md)

> **Draft for review, 2026-11-15.** This is a plan, not a finished course. Draft labs exist for stages 00 and 01 only, under [`labs/`](labs/). Their Windows programs were built and run when they were written; their xv6 and Linux steps have not been run yet. The stage order, hour estimates and tool choices are proposals. Technical claims were checked that day against the official pages listed under [Sources](SYLLABUS.md#sources). Recheck versions before building anything.

This is a build-first operating systems course with one goal: being able to build a desktop graphics system, meaning a window system and compositor that runs real applications. Part I builds the OS foundation: you change a small teaching kernel, and you write Linux and Windows programs against the real interfaces. Part II takes those mechanisms to the display. It covers how buffers move between client processes, the kernel and the display hardware, and it ends with a minimal Wayland compositor on Linux DRM/KMS.

## How the work splits with the graphics course

The desktop-graphics topic is split by layer:

| Layer | Question | Where |
| --- | --- | --- |
| Pixels | How each pixel of a window or of the final frame is computed: 2D drawing, text, blending, color | [`computer-graphics-practice/`](../computer-graphics-practice/README.md), in the 2D, text and compositing rendering track of its [Part II draft](../computer-graphics-practice/SYLLABUS-PART2.md). Doable on Windows. |
| Buffers and time | Who owns a buffer, how it reaches another process and the screen, and when it is shown | This course, Part II. Runs in a Linux VM; a final set of hardware checks needs a Linux PC. |
| Mechanisms | Processes, memory, traps, IPC, synchronization, scheduling, drivers | This course, Part I. Runs on Windows 11 through WSL 2 and QEMU. |

The capstone joins the two: surfaces in, composited frame out. This course supplies client buffers, fences and display timing. The graphics course supplies the module that turns surfaces into a frame. The plan includes the [interface between them](SYLLABUS.md#interface-with-the-graphics-course).

## Course route

| Stage | Build | Core ideas | Runs on |
| --- | --- | --- | --- |
| **Part I: OS foundations** | | | |
| 00 Workbench | Boot xv6 in QEMU and step through it in GDB | Firmware-free boot, privilege modes | WSL 2 |
| 01 System calls | Add a system call and trace one call across the boundary | Traps, trap frames, untrusted user pointers | WSL 2, Windows |
| 02 Processes and threads | A fork/exec/pipe pipeline; user-level threads | Process state, context switch, calling convention | WSL 2, Windows |
| 03 Virtual memory | Copy-on-write fork; shared and private mappings | Page tables, page faults, mmap, SIGBUS | WSL 2, Windows |
| 04 Interrupts and drivers | Follow a device interrupt; a small Linux character-device module | Interrupt context, deferred work, device model | WSL 2, Linux VM |
| 05 Synchronization | Reduce lock contention; a futex-based mutex | Atomics, spinlocks vs sleep locks, lost wakeups | WSL 2, Windows |
| 06 Scheduling and latency | A new xv6 scheduler; a wakeup-latency histogram | Preemption, fairness vs latency, real-time classes | WSL 2, Windows |
| 07 File systems | Extend the xv6 file system; file-backed mmap | Logging, inodes, fds vs open file descriptions | WSL 2 |
| 08 IPC and event loops | Pass fds and sealed shared memory; one epoll loop | Capabilities, sealing, readiness vs completion | WSL 2, Windows |
| 09 Part I project | A headless display server with its own protocol | Untrusted clients, back-pressure, frame pacing | WSL 2 |
| **Part II: Window and display system** | | | |
| 10 Window system architecture | Measure the DWM presentation path with PresentMon | X11, Wayland, DWM/WDDM; why DWM can't be replaced | Windows |
| 11 Wayland clients | A toolkit-free Wayland client | Objects, globals, double-buffered state, frame callbacks | WSL 2 (WSLg) |
| 12 DRM/KMS | Light up a screen with an atomic commit | KMS objects, properties, DRM master, render nodes | Linux VM |
| 13 Planes and timing | Flip events, planes, hotplug recovery | vblank, page flip, front porch, uevents, link status | Linux VM |
| 14 Buffer sharing and sync | A buffer passed across two processes to scanout | dma-buf, modifiers, dma-fence, sync_file, syncobj | Linux VM, Windows |
| 15 Input | evdev → libinput → xkbcommon, with focus routing | Event packets, device quirks, keymaps | Linux VM |
| 16 Seats and sessions | Run unprivileged and survive VT switches | logind/libseat, DRM master hand-off, revocation | Linux VM |
| 17 Isolation and security | Attack and defend your own server | Untrusted buffers, capture policy, GPU resets | Linux VM, Windows |
| 18 Capstone | A minimal Wayland compositor running real apps | Everything above, end to end | Linux VM |
| Hardware validation | Rerun the timing, plane, VRR, hotplug and GPU checks on a real display | What a VM cannot show | Linux PC |

[`SYLLABUS.md`](SYLLABUS.md) has the milestones, prerequisites and checkpoints for each stage.

## Time and prerequisites

Plan on roughly 270–480 focused hours. Part I takes about 110–190 and Part II about 160–290. Part II's share includes the 60–120 hour capstone and 15–30 hours of hardware validation. At 6–8 hours a week that is eight months to a year and a half. These are planning estimates for someone who already writes C, not promises. Progress is measured by passed checkpoints, not by weeks.

You start with C (pointers, structs, bit operations, linking), a Linux shell, git and make, and enough RISC-V assembly to read compiler output. That level is stage 1 of the [`c-compiler-foundations` learning path](../c-compiler-foundations/research/04-integration-path.zh-TW.md). Part II also needs the graphics course's Part I through lab 05 (GPU pipeline). The capstone additionally needs stage T4 (compositing) of the graphics course's Part II 2D, text and compositing track. T4 builds on T1, T3 and the shared core, which adds up to several hundred hours on its own. That track can run in parallel with this course's Part I.

## Lab vehicle: why xv6-riscv

[xv6-riscv](https://github.com/mit-pdos/xv6-riscv) is MIT's re-implementation of Unix Version 6 for a modern multiprocessor RISC-V, written in ANSI C. It is the kernel of MIT 6.1810. The 2025 public lab sequence is util, syscall, pgtbl, traps, cow, net, lock, fs and mmap ([schedule](https://pdos.csail.mit.edu/6.1810/2025/schedule.html)). The Fall 2026 site was still under construction on 2026-11-15. Why it fits:

- It is small enough to read in full. It still has real processes, Sv39 paging, traps, a PLIC, locks on a multiprocessor, a scheduler, a logging file system, pipes and virtio devices.
- It boots in QEMU's `virt` machine with `-bios none`. Every instruction from reset to the shell prompt is in the source tree.
- Its RISC-V picks up from [`c-compiler-foundations/`](../c-compiler-foundations/README.md). That folder targets RV32IM. xv6 is 64-bit (`-march=rv64gc`) and adds the privileged architecture: CSRs, privilege modes, traps and paging.
- MIT's labs and the xv6 book are a tested exercise sequence. This course adds Linux and NT comparison work and a path to the display.

Alternatives considered:

| Option | Verdict |
| --- | --- |
| Extend `rv32sim` from `c-compiler-foundations/` | Not as the main vehicle. It executes assembly text, not machine code, and has no CSRs, privilege modes, traps or MMU. Making it boot a kernel is a project of its own, so it stays an optional side quest. |
| A toy x86-64 kernel from scratch | Spends more hours on boot and legacy PC details before reaching the ideas. Good as a follow-up after xv6. |
| Modify Linux itself | Too large to be the kernel you change. Linux is used throughout from user space (WSL 2) and for one kernel module in a VM. |
| Windows NT | No source access. NT is compared through documented APIs and tracing tools (WPR/WPA, Process Explorer). Writing an NT driver is optional and VM-only. |

## What runs where

| Work | Windows 11 | WSL 2 | Linux VM (QEMU) | Linux on real hardware |
| --- | --- | --- | --- | --- |
| xv6 labs | n/a | Yes. MIT's Windows setup is WSL 2 with Ubuntu 24.04; WSL 1 does not work | Not needed | Yes |
| Linux system-call, memory and IPC experiments | n/a | Yes. WSL 2 runs a real Linux kernel in a lightweight VM | Yes | Yes |
| Your own Linux kernel module | n/a | Not planned | Yes | Yes |
| NT comparisons (Win32, ETW traces, Process Explorer) | Yes | n/a | n/a | n/a |
| Wayland client programming | n/a | Yes, through WSLg | Yes | Yes |
| KMS atomic commits, flip events, overlay planes (VKMS), hotplug from host display changes (virtio-gpu) | n/a | No | Yes | Yes |
| Buffer sharing, explicit sync, input devices, seats and VT switches | n/a | No | Yes, with software rendering | Yes |
| Capstone compositor running real apps | n/a | No | Yes, with software rendering | Yes |
| Real flip timing, VRR, planes on a real GPU, cable hotplug and link status, GPU-accelerated composition, input-to-photon latency | n/a | No | No | Yes |

**A Linux VM can carry Part II up to and including the capstone. Only the final hardware validation needs Linux with direct control of a real display.** That validation still means a dual-boot install on the RTX 5070 Ti desktop or a separate PC, but it can wait until the end.

What a VM gives you, checked against the guest drivers' source on 2026-11-15:

- **virtio-gpu (QEMU) is the recommended VM display.** It is an atomic KMS driver with a render node and DRM syncobj, timelines included ([virtgpu_drv.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/virtio/virtgpu_drv.c)). It exposes one primary and one cursor plane per output ([virtgpu_display.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/virtio/virtgpu_display.c)). Display changes on the host reach the guest as hotplug events ([virtgpu_kms.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/virtio/virtgpu_kms.c)).
- **VKMS fills virtio-gpu's gaps.** Through configfs it offers overlay planes and a writeback connector. It has no runtime hotplug yet ([VKMS](https://docs.kernel.org/gpu/vkms.html)).
- **VMware Workstation is a fallback.** It has been free for all users since November 2024 ([VMware](https://blogs.vmware.com/cloud-foundation/2024/11/11/vmware-fusion-and-workstation-are-now-free-for-all-users/)). Its guest driver, vmwgfx, is atomic with a render node and has primary and cursor planes, but no syncobj ([vmwgfx_drv.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/vmwgfx/vmwgfx_drv.c)).
- **Hyper-V is too thin.** Its hyperv_drm driver has a single primary plane and no render node ([hyperv_drm_drv.c](https://github.com/torvalds/linux/blob/master/drivers/gpu/drm/hyperv/hyperv_drm_drv.c)).
- **Expect software rendering in the guest.** 3D acceleration for a QEMU guest on a Windows host is not documented: QEMU's virgl and Venus requirements cover Linux hosts only ([virtio-gpu](https://www.qemu.org/docs/master/system/devices/virtio/virtio-gpu.html)).

What a VM cannot give you:

- real scanout timing and VRR;
- the planes of a real GPU;
- cable hotplug and link training;
- GPU-accelerated composition on the RTX 5070 Ti;
- input-to-photon latency.

The GPU can't be handed to a Linux VM either. Hyper-V's Discrete Device Assignment is documented for Windows Server on server-class hardware, with Windows guests only ([Learn](https://learn.microsoft.com/en-us/windows-server/virtualization/hyper-v/plan/plan-for-deploying-devices-using-discrete-device-assignment)).

WSL 2 is not a route for Part II's display work. Its GPU access is para-virtualized for rendering, and WSLg shows Linux windows through a Weston compositor that forwards them to Windows over RDP. Stage 10 is about Windows itself, and stage 11 runs in WSLg.

Running QEMU: it publishes Windows builds (installers by Stefan Weil, or MSYS2 packages) and can use the Windows Hypervisor Platform (WHPX) accelerator. Windows 11 also turns on nested virtualization for WSL 2 by default (`nestedVirtualization` in `.wslconfig`), which gives a second route to a Linux VM. Learn warns that VMware Workstation and VirtualBox can be unreliable while the Hyper-V hypervisor is running ([Learn](https://learn.microsoft.com/en-us/virtualization/hyper-v-on-windows/quick-start/enable-hyper-v)). Keep that in mind if WSL 2 stays enabled.

## Real hardware for the final validation

When you reach the hardware validation, the RTX 5070 Ti can do it on Linux, with limits. These come from NVIDIA's driver README (620.70.06, checked 2026-11-15):

- Blackwell GPUs are supported only by NVIDIA's open kernel modules.
- nvidia-drm KMS is on by default.
- The driver ships a GBM backend for GBM-based Wayland compositors.
- Explicit sync uses DRM syncobj timeline points and needs kernel 6.8 or newer.
- The KMS driver registers only primary and cursor planes, no overlay planes.
- The known issues also cover VRR behavior, and a black-screen interaction with simpledrm unless `fbdev=1` is set. Read them before stage 13.
- Mesa's NVK Vulkan driver lists consumer Blackwell (RTX 5000 series) support. That is an upstream alternative to compare later.

So on real hardware, overlay planes need a different GPU. An Intel or AMD GPU works, and integrated graphics is enough. First confirm with `drm_info` that it exposes overlay planes. In the VM route, VKMS covers overlay planes for API practice. A small separate PC with such a GPU would also spare the main desktop a dual boot. Which machine to use is open question 1 below, and it can wait until the capstone works in the VM.

Monitors: the ASUS VG289Q lists Adaptive-Sync with a 40–60 Hz vertical range, so it can serve the VRR check. VRR support on the Acer ET322QK was not verified.

## Working loop

This is the graphics course's loop with one addition from `mcu-foundations`: **predict → implement → inspect → explain → break it on purpose.** Keep a `NOTES.md`. For each experiment, record every tool version, the expected result, the measured result and the broken case. A Linux or NT comparison counts only if it cites the documentation page or shows a trace.

## Related folders

- [`computer-graphics-practice/`](../computer-graphics-practice/README.md) is the prerequisite for Part II. Its stages 00–05 cover framebuffers, pixel formats and the GPU pipeline. Its [Part II draft](../computer-graphics-practice/SYLLABUS-PART2.md) has a 2D, text and compositing rendering track, whose T3 (text) and T4 (compositing) supply the modules that stages 09 and 18 call. That Part II is still a draft for review, so stage names may change.
- [`c-compiler-foundations/`](../c-compiler-foundations/README.md) has the contract stack (language, ABI, ISA). Stages 01–02 extend it downward: trap frames and `swtch` only make sense with the RISC-V calling convention in mind. Its `rv32sim` is the starting point for the optional privileged-mode side quest.
- [`mcu-foundations/`](../mcu-foundations/README.md) (Traditional Chinese only) covers reset to `main`, interrupts, DMA buffer ownership and ISR rules in one address space. This course asks the same questions with memory protection and many processes.
- [`mcu-embedded-hmi-research/`](../mcu-embedded-hmi-research/README.md) covers embedded graphics where one application owns the display and there is no compositor (LVGL, TouchGFX, smart displays). Part II is the multi-client case.
- [`display-wake-research/`](../display-wake-research/README.md) covers the Windows display path on the same RTX 5070 Ti desktop. It includes a sleeping DisplayPort monitor being treated as disconnected, and the Win+Ctrl+Shift+B graphics driver reset. Stages 10 and 13 meet the same events from the OS side: WDDM recovery and KMS hotplug.

## Files

- [`SYLLABUS.md`](SYLLABUS.md) / [`SYLLABUS.zh-TW.md`](SYLLABUS.zh-TW.md): stage-by-stage build plan, the interface with the graphics course, side quests and sources.
- [`labs/`](labs/): one folder per stage, `labs/NN-name/`, each with a README of exercises, predictions, checkpoints and failure modes, following the graphics course. Each README states which of its steps were executed and which were not. Drafts so far:
  - [`labs/00-workbench/`](labs/00-workbench/README.md): boot xv6 in QEMU and step from the reset vector to `main()` in GDB. Not executed yet.
  - [`labs/01-system-calls/`](labs/01-system-calls/README.md): follow one system call across the boundary, add one, and pass bad pointers to xv6, Linux and NT. The Windows programs were built and run, with recorded output. The xv6 and Linux parts were not executed.
  - Stages 02–18 have no lab folder yet.

## Completion target

You end with a compositor you wrote yourself. It boots from a text console as a normal user and runs a terminal, a GTK or Qt application and a Vulkan demo side by side. It survives a VT switch, a monitor hotplug and a crashing client. You also write a design note that traces one client frame from `wl_surface.commit` to scanout and back to the frame callback, with measured timestamps.

All of that works in a VM. The hardware validation then repeats it on a real display, where the timestamps mean something and VRR and real planes exist. Only the second proves frame timing.

Part I alone ends when you can explain, with traces, what happens from a system call to a context switch to a page fault. You explain it in xv6 and again in Linux and NT terms.

## Scope and limits

- Out of scope: GPU kernel drivers, Mesa drivers, UI toolkits, Xwayland internals, Windows kernel drivers (optional, VM only), HDR and color management (an optional extension).
- On Windows you can study and measure DWM but not replace it. Stage 10 of the syllabus explains why.
- WSL 2 and the Linux VM both run under a hypervisor, so latency measured there is only relative. The hardware validation measures again on a real display.
- The sources were checked on 2026-11-15. gitlab.freedesktop.org and git.sr.ht refused automated requests during the check. wlroots, tinywl and seatd are linked, but their pages were not re-read. Wayland protocol statuses came from the wayland.app index, which is generated from the same XML files.
- Hours are estimates.

## Review decisions (2026-11-15)

- **Where Part II runs:** a Linux VM by default (QEMU with virtio-gpu, plus VKMS). Real hardware is a final validation step.
- **Capstone route:** your own backend, built from the stage 12–16 code on libwayland-server. tinywl is read as a map, and wlroots is not used as the backend.
- **Language:** C for Part I and kernel-facing code. The compositor may use C++ to match the graphics course's Part II core.
- **Optional side quests:** all stay. That is the rv32sim privileged mode, a KMDF driver in a Windows VM, HDR and color, Xwayland, and NVK.

## Open questions for review

1. Hardware for the final validation: dual boot on the RTX 5070 Ti desktop, or a separate PC with Intel or AMD graphics? Does the desktop CPU have integrated graphics? This can wait until the capstone works in the VM.
2. Which VM host: QEMU on Windows with WHPX, or QEMU inside WSL 2? Both are documented routes, and neither documents 3D acceleration for the guest. QEMU on Windows is the suggested start, because its window is native rather than forwarded through WSLg.
3. Distribution and kernel: Ubuntu 24.04 matches MIT's tools page. NVIDIA explicit sync needs kernel 6.8+, and nvidia-drm's KMS color pipeline needs 6.19+. Should WSL 2, the VM and the Linux PC all run the same distribution?
4. Does the Acer ET322QK support VRR? This only matters for the hardware validation.
5. Graphics API for the reused modules: the graphics course's Part II lets you choose Vulkan or Direct3D 12 and keep it. Only Vulkan, or a CPU path, can run inside this course's Linux compositor. Should the graphics course choose Vulkan for that reason?
