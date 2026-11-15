# Operating Systems Foundations: From Kernel to Compositor

**English** | [繁體中文](README.zh-TW.md) | [Build plan](SYLLABUS.md)

> **Draft for review, 2026-11-15.** This is a plan, not a finished course, and no labs exist yet. The stage order, hour estimates and tool choices are proposals. Technical claims were checked that day against the official pages listed under [Sources](SYLLABUS.md#sources). Recheck versions before building anything.

This is a build-first operating systems course with one goal: being able to build a desktop graphics system, meaning a window system and compositor that runs real applications. Part I builds the OS foundation: you change a small teaching kernel, and you write Linux and Windows programs against the real interfaces. Part II takes those mechanisms to the display. It covers how buffers move between client processes, the kernel and the display hardware, and it ends with a minimal Wayland compositor on Linux DRM/KMS.

## How the work splits with the graphics course

The desktop-graphics topic is split by layer:

| Layer | Question | Where |
| --- | --- | --- |
| Pixels | How each pixel of a window or of the final frame is computed: 2D drawing, text, blending, color | [`computer-graphics-practice/`](../computer-graphics-practice/README.md), in its planned Part II track "2D、文字與合成渲染" (2D, text and compositing). Doable on Windows. |
| Buffers and time | Who owns a buffer, how it reaches another process and the screen, and when it is shown | This course, Part II. Needs Linux on real hardware from stage 13. |
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
| 12 DRM/KMS | Light up a screen with an atomic commit | KMS objects, properties, DRM master, render nodes | Linux VM, then Linux PC |
| 13 Planes and timing | Flip timing, planes, VRR, hotplug recovery | vblank, page flip, front porch, uevents, link status | Linux PC |
| 14 Buffer sharing and sync | A GPU buffer passed across two processes to scanout | dma-buf, modifiers, dma-fence, sync_file, syncobj | Linux PC, Windows |
| 15 Input | evdev → libinput → xkbcommon, with focus routing | Event packets, device quirks, keymaps | Linux PC |
| 16 Seats and sessions | Run unprivileged and survive VT switches | logind/libseat, DRM master hand-off, revocation | Linux PC |
| 17 Isolation and security | Attack and defend your own server | Untrusted buffers, capture policy, GPU resets | Linux PC, Windows |
| 18 Capstone | A minimal Wayland compositor running real apps | Everything above, end to end | Linux PC |

[`SYLLABUS.md`](SYLLABUS.md) has the milestones, prerequisites and checkpoints for each stage.

## Time and prerequisites

Plan on roughly 260–450 focused hours. Part I takes about 110–190 and Part II about 150–260, of which the capstone is 60–120. At 6–8 hours a week that is eight months to a year and a half. These are planning estimates for someone who already writes C, not promises. Progress is measured by passed checkpoints, not by weeks.

You start with C (pointers, structs, bit operations, linking), a Linux shell, git and make, and enough RISC-V assembly to read compiler output. That level is stage 1 of the [`c-compiler-foundations` learning path](../c-compiler-foundations/research/04-integration-path.zh-TW.md). Part II also needs the graphics course through stage 05 (GPU pipeline), plus the compositing modules of its planned 2D, text and compositing track.

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
| DRM/KMS API basics | n/a | No | Yes, with virtio-gpu or VKMS | Yes |
| Real planes, vblank timing, VRR, hotplug, GPU buffer sharing, input devices, seats | n/a | No | No real timing or hardware planes | Yes |
| Capstone compositor | n/a | No | Development against VKMS only | Yes |

**Part II needs Linux with direct control of a display from stage 13 on.** That means either a dual-boot install on the RTX 5070 Ti desktop or a separate PC. WSL 2 cannot stand in. Its GPU access is para-virtualized for rendering. WSLg shows Linux windows through a Weston compositor that forwards them to Windows over RDP. Nothing inside WSL drives your monitors through KMS. Stages 11 and 12 can start on Windows (WSLg, then a QEMU VM), and stage 10 is about Windows itself.

For the VM: QEMU publishes Windows builds (installers by Stefan Weil, or MSYS2 packages) and can use the Windows Hypervisor Platform (WHPX) accelerator. Windows 11 also turns on nested virtualization for WSL 2 by default (`nestedVirtualization` in `.wslconfig`), which gives a second route to a Linux VM.

## The Linux PC

The RTX 5070 Ti can do this course on Linux, with limits. These come from NVIDIA's driver README (620.70.06, checked 2026-11-15):

- Blackwell GPUs are supported only by NVIDIA's open kernel modules.
- nvidia-drm KMS is on by default.
- The driver ships a GBM backend for GBM-based Wayland compositors.
- Explicit sync uses DRM syncobj timeline points and needs kernel 6.8 or newer.
- The KMS driver registers only primary and cursor planes, no overlay planes.
- The known issues also cover VRR behavior, and a black-screen interaction with simpledrm unless `fbdev=1` is set. Read them before stage 13.
- Mesa's NVK Vulkan driver lists consumer Blackwell (RTX 5000 series) support. That is an upstream alternative to compare later.

So the overlay-plane experiments in stage 13 need either VKMS (for API practice) or a different GPU. An Intel or AMD GPU works, and integrated graphics is enough. First confirm with `drm_info` that it exposes overlay planes. A small separate PC with such a GPU would also spare the main desktop a dual boot. This is open question 1 below.

Monitors: the ASUS VG289Q lists Adaptive-Sync with a 40–60 Hz vertical range, so it can serve the VRR experiment. VRR support on the Acer ET322QK was not verified.

## Working loop

This is the graphics course's loop with one addition from `mcu-foundations`: **predict → implement → inspect → explain → break it on purpose.** Keep a `NOTES.md`. For each experiment, record every tool version, the expected result, the measured result and the broken case. A Linux or NT comparison counts only if it cites the documentation page or shows a trace.

## Related folders

- [`computer-graphics-practice/`](../computer-graphics-practice/README.md) is the prerequisite for Part II. Its stages 00–05 cover framebuffers, pixel formats and the GPU pipeline. Its planned Part II track "2D、文字與合成渲染" supplies the composition module that stages 09 and 18 call. That Part II is still being drafted on another branch, so stage names may change.
- [`c-compiler-foundations/`](../c-compiler-foundations/README.md) has the contract stack (language, ABI, ISA). Stages 01–02 extend it downward: trap frames and `swtch` only make sense with the RISC-V calling convention in mind. Its `rv32sim` is the starting point for the optional privileged-mode side quest.
- [`mcu-foundations/`](../mcu-foundations/README.md) (Traditional Chinese only) covers reset to `main`, interrupts, DMA buffer ownership and ISR rules in one address space. This course asks the same questions with memory protection and many processes.
- [`mcu-embedded-hmi-research/`](../mcu-embedded-hmi-research/README.md) covers embedded graphics where one application owns the display and there is no compositor (LVGL, TouchGFX, smart displays). Part II is the multi-client case.
- [`display-wake-research/`](../display-wake-research/README.md) covers the Windows display path on the same RTX 5070 Ti desktop. It includes a sleeping DisplayPort monitor being treated as disconnected, and the Win+Ctrl+Shift+B graphics driver reset. Stages 10 and 13 meet the same events from the OS side: WDDM recovery and KMS hotplug.

## Files

- [`SYLLABUS.md`](SYLLABUS.md) / [`SYLLABUS.zh-TW.md`](SYLLABUS.zh-TW.md): stage-by-stage build plan, the interface with the graphics course, side quests and sources.
- No labs yet. The planned layout is `labs/NN-name/`, one folder per stage, each with a README of exercises and checkpoints, following the graphics course.

## Completion target

You end with a compositor you wrote yourself. It boots from a text console as a normal user and runs a terminal, a GTK or Qt application and a Vulkan demo side by side. It survives a VT switch, a monitor hotplug and a crashing client. You also write a design note that traces one client frame from `wl_surface.commit` to scanout and back to the frame callback, with measured timestamps.

Part I alone ends when you can explain, with traces, what happens from a system call to a context switch to a page fault. You explain it in xv6 and again in Linux and NT terms.

## Scope and limits

- Out of scope: GPU kernel drivers, Mesa drivers, UI toolkits, Xwayland internals, Windows kernel drivers (optional, VM only), HDR and color management (an optional extension).
- On Windows you can study and measure DWM but not replace it. Stage 10 of the syllabus explains why.
- WSL 2 runs inside a VM, so latency measured there is only relative. Stage 13 measures again on real hardware.
- The sources were checked on 2026-11-15. gitlab.freedesktop.org and git.sr.ht refused automated requests during the check. wlroots, tinywl and seatd are linked, but their pages were not re-read. Wayland protocol statuses came from the wayland.app index, which is generated from the same XML files.
- Hours are estimates.

## Open questions for review

1. Linux machine for stages 13–18: dual boot on the RTX 5070 Ti desktop, or a separate PC with Intel or AMD graphics? Does the desktop CPU have integrated graphics?
2. Distribution and kernel: Ubuntu 24.04 matches MIT's tools page. NVIDIA explicit sync needs kernel 6.8+, and nvidia-drm's KMS color pipeline needs 6.19+. Should WSL 2 and the Linux PC run the same distribution, or may they differ?
3. Capstone route: your own backend built from stages 12–16 (recommended, since those stages already write it), or wlroots for the backend with your own scene and composition?
4. Language: C for Part I and kernel-facing code. For the compositor, C or C++, depending on the graphics course's Part II core (C++/Vulkan)?
5. NT depth: comparison only (as planned), or add a KMDF driver lab in a Windows VM?
6. Keep or drop the `rv32sim` privileged-mode side quest?
7. Does the Acer ET322QK support VRR? This only matters for stage 13.
