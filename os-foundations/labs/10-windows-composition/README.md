# Lab 10: Window system architecture, measured on Windows

**English** | [繁體中文](README.zh-TW.md) | [Build plan, stage 10](../../SYLLABUS.md#10-window-system-architecture-measured-on-windows)

> **Draft for review, 2026-11-16. What was and was not executed:**
> - **`flipstats.exe` (exercises A–D): built and run** on 2026-11-16, natively and unelevated, with gcc 16.2.0 from WinLibs and its MinGW-w64 14.0.0 headers and import libraries (no Windows SDK): `info` once; the windowed and the overlapped configuration with vsync on and off, twice each; the windowed configuration with a maximum frame latency of 1, twice. Output: [`windows/example-output.txt`](windows/example-output.txt). No account name, machine name or SID appears in it.
> - **Where it ran is not where this stage is meant to run.** The machine has an Intel Core i7-11700 with Intel UHD Graphics 750 (driver 31.0.101.5334) and no other GPU, runs Windows 11 Pro build 26300.9711, and the program ran inside a **Remote Desktop session**. That session's only display is the remote one, which every API reported as 1440 × 900 at 32 Hz. Every frame was composed for that remote display, and the vblank fields of the frame statistics stayed 0. Nothing here measures the RTX 5070 Ti, a physical monitor, Independent Flip or an overlay plane. Run everything again at the console of the machine you study.
> - **Borderless full screen (exercise E): not executed.** It covers the whole monitor of whoever is using the machine. Its code differs from the windowed path only in the window it creates, but it has never run.
> - **The vblank half of the frame-statistics analysis** (present-to-vblank latency, glitch counts) never ran either, because the vblank fields stayed 0. Those lines of the program are untested.
> - **PresentMon (exercise F): not executed** and not downloaded. The command lines follow its console README as read on 2026-11-16, and the `PresentMode` meanings are taken from it.
> - **Not executed:** any ETW or WPR trace, Win+Ctrl+Shift+B (exercise G), TDR or any GPU hang (described only), and any change to display settings.
> - **Documentation:** the Learn pages, the DirectX blog post and the PresentMon README linked below were read on 2026-11-16.

## Goal

Part II starts on the system you cannot replace. When a Windows program presents a frame, either DWM composes it into the desktop, or the display hardware scans the program's buffer out directly while DWM steps aside. This lab builds a small Direct3D 11 program with a flip-model swap chain, logs everything the ordinary APIs will say about where its frames went, and then names what those APIs cannot say, which is what PresentMon's ETW capture answers. You run it in three configurations: in a window, in a window partly covered by another window, and borderless over the whole monitor.

```
 your process                          dwm.exe (one per session)            display hardware
 D3D11 + DXGI, flip model
 Present(buffer N) ───────────────────▶ composed: reads buffer N (shared,  ─▶ primary plane scans out
   buffers are shared with DWM;          no copy) into its own frame           DWM's frame
   no copy into a redirection surface                                          PresentMon: "Composed: Flip"

 window covers the monitor, nothing ──── independent flip: DWM stops  ──────▶ primary plane scans out
 on top, size and format fit             composing; buffer N goes             buffer N itself
                                         to the plane directly                "Hardware: Independent Flip"

 driver grants an overlay plane ──────── DWM composes everything else ──────▶ overlay plane: buffer N;
 (multiplane overlay, MPO)               on the primary plane                 primary plane: DWM's frame
                                                                              "Hardware Composed: Independent Flip"
```

Stage 13 meets the same objects from the Linux side: planes, page flips, vblank and VRR. On Windows DWM decides which of the three paths a frame takes. On Linux your compositor does.

## Files

| File | What it is | Status |
| --- | --- | --- |
| [`windows/flipstats.c`](windows/flipstats.c) | a D3D11 program in C: an inventory mode (`info`) and three window configurations, each with vsync on or off, `FLIP_DISCARD` or `FLIP_SEQUENTIAL`, a buffer count and a frame latency; logs `QueryPerformanceCounter`, `GetLastPresentCount`, `GetFrameStatistics`, `GetFrameStatisticsMedia`, `DwmGetCompositionTimingInfo` and `WaitForVBlank` | built and run here (`info`, `windowed`, `overlap`); `borderless` not run |
| [`windows/example-output.txt`](windows/example-output.txt) | recorded output, eleven runs in a Remote Desktop session | recorded 2026-11-16 |
| PresentMon CSV files | your captures from exercise F | not recorded here |

## Setup

- **Compiler:** gcc from WinLibs (MinGW-w64), as in the earlier labs. Its headers include `d3d11.h`, `d3d11_1.h`, `dxgi1_2.h` to `dxgi1_6.h` and `dwmapi.h`, and its libraries include `libd3d11.a`, `libdxgi.a` and `libdwmapi.a`, so no Windows SDK is needed. The program draws with clears only, so it needs no shader compiler either. (WinLibs also ships `d3d12.h`, `libd3d12.a` and `libd3dcompiler.a`, if you prefer D3D12; the presentation path is DXGI's in both.)

  ```powershell
  cd windows
  gcc -O2 -Wall -o flipstats.exe flipstats.c -ld3d11 -ldxgi -ldwmapi -luser32 -lgdi32 -ladvapi32
  .\flipstats.exe info
  ```

- **Where to run it:** at the console of the machine you study (the RTX 5070 Ti desktop in this course), signed in locally, not over Remote Desktop. The program prints `GetSystemMetrics(SM_REMOTESESSION)`; it must be 0. Close or move other windows away from the bottom-right corner and from the monitor you test on.
- **PresentMon** for exercise F, from its [GitHub releases](https://github.com/GameTechDev/PresentMon/releases). On 2026-11-16 the newest release was v2.6.0, and its console application is the single file `PresentMon-2.6.0-x64.exe` (957 KB). Check the releases page for a newer one. It records through an ETW session; its README offers `--restart_as_admin` for when it is not elevated, so run it from an elevated prompt.

## Read first

- Learn: [DXGI flip model](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model) (flip vs bitblt, frame statistics in windowed mode, the glitch-detection steps), [IDXGISwapChain::Present](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present) (sync intervals for the flip model, VRR needs tearing), [DXGI_FRAME_STATISTICS](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ns-dxgi-dxgi_frame_statistics), [GetFrameStatistics](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-getframestatistics), [DwmGetCompositionTimingInfo](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmgetcompositiontiminginfo).
- Shawn Hargreaves, [For best performance, use DXGI flip model](https://devblogs.microsoft.com/directx/dxgi-flip-model/) (DirectX blog, 2018-04-09): DirectFlip, Independent Flip, MPO, and "reverse composition" with `FLIP_DISCARD`.
- Learn, driver side: [WDDM architecture](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/windows-vista-and-later-display-driver-model-architecture), [MPO](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/multiplane-overlay-support), [TDR](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/timeout-detection-and-recovery).
- PresentMon's [console application README](https://github.com/GameTechDev/PresentMon/blob/main/README-ConsoleApplication.md): the options, the CSV columns and the `PresentMode` values.
- The syllabus's [stage 10](../../SYLLABUS.md#10-window-system-architecture-measured-on-windows) for X11, Wayland and why DWM cannot be replaced.

## The program

`flipstats.exe` creates a D3D11 device on the adapter that owns the window's monitor and a swap chain with `CreateSwapChainForHwnd`: `DXGI_FORMAT_B8G8R8A8_UNORM`, two buffers, `DXGI_SWAP_EFFECT_FLIP_DISCARD` (`--sequential` for `FLIP_SEQUENTIAL`), and `DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING` when `IDXGIFactory5::CheckFeatureSupport` reports tearing support. Each frame it clears the buffer to a new colour, draws a white bar that moves 4 pixels per frame (tearing shows as a broken bar), and calls `Present(1, 0)`, or with `--vsync 0` `Present(0, DXGI_PRESENT_ALLOW_TEARING)`. It declares per-monitor DPI awareness, so that Windows does not scale its window on a scaled monitor (which would force composition), and disables Alt+Enter with `DXGI_MWA_NO_ALT_ENTER`.

| Configuration | Window | Safety |
| --- | --- | --- |
| `windowed` | 320 × 240 client area near the bottom-right corner of the work area | `WS_EX_NOACTIVATE`, `SW_SHOWNOACTIVATE`: never the foreground window, no taskbar button; not topmost |
| `overlap` | the same, plus a 160 × 120 opaque popup that it owns, over the lower-right part of the client area | the same; an owned window always stays above its owner |
| `borderless` | `WS_POPUP` covering the primary monitor exactly, shown normally and brought to the foreground | covers your screen for the whole run; Esc ends it early |

Every configuration ends by itself (default 4 s, at most 30 s), and a watchdog thread ends the process 5 s after the run should have finished.

## The evidence, and what each piece can prove

Nothing an application can call tells it, frame by frame and with documentation behind it, whether DWM composed that frame. Here is what each source in the log does say:

| Source | What it reports | What it can show | What it cannot show | In the recorded runs |
| --- | --- | --- | --- | --- |
| `QueryPerformanceCounter` around `Present` | when you submitted, how long `Present` blocked | back-pressure: a full present queue or vsync pacing makes `Present` wait | anything after the submit | blocked about one 32 Hz period with vsync on, about 0.1 ms with vsync off |
| `GetLastPresentCount` | the id DXGI gave your last present | joins your timestamps to the statistics | nothing on its own | ids 1, 2, 3, … |
| `GetFrameStatistics` | `PresentCount`: the newest present DXGI counts as shown; `PresentRefreshCount`: the vblank at which it was shown; `SyncRefreshCount` and `SyncQPCTime`: a vblank and its time | which presents reached the screen, and when, in vblanks; glitches, using Learn's [glitch steps](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model#avoiding-detecting-and-recovering-from-glitches) | whether DWM composed the frame or the hardware flipped it; Learn warns the statistics "are not reliable in many multiple monitor scenarios" | `PresentCount` advanced; the three vblank fields stayed 0 |
| `IDXGISwapChainMedia::GetFrameStatisticsMedia` | `CompositionMode`: `COMPOSED`, `OVERLAY`, `NONE` | a hint from DXGI itself about composition | Learn documents it for media apps deciding whether to keep a decode swap chain, not for ordinary swap chains, and does not say whether `OVERLAY` covers Independent Flip | `COMPOSED` for every present (`NONE` once, before the first) |
| `DwmGetCompositionTimingInfo(NULL, …)` | DWM's refresh rate and its counters of refreshes and composed frames | how fast DWM itself runs, for the whole session | anything about one window: since Windows 8.1 the window handle must be `NULL` | 32 Hz; the counters moved 8–10 per second while DXGI reported 32 new frames per second |
| `IDXGIOutput::WaitForVBlank` | returns at the output's next vblank | the output's cadence as DXGI sees it | which plane showed what | 31.0–32.0 Hz mean, 31.2–31.4 ms median |
| `SupportsOverlays`, `CheckOverlaySupport`, `CheckHardwareCompositionSupport`, tearing support | what the output and driver could do | whether MPO or hardware composition is possible at all | whether it happened | no overlays, no hardware composition flags, tearing supported |
| PresentMon (ETW) | `PresentMode`, `AllowsTearing`, display timing per present | which path each present took, and when it reached the screen | photons, which need a camera or a latency tester | not run |

So the program can show you pacing, queueing and which presents were shown. Only a trace says how they were shown.

## Exercises

Write each prediction in `NOTES.md` before you run anything.

### A. What the APIs say about the machine (run here)

```powershell
.\flipstats.exe info
```

It prints the Windows build, whether this is a Remote Desktop session, every DXGI adapter (description, LUID, vendor and device id, user-mode driver version from `CheckInterfaceSupport`) and its outputs (desktop rectangle, the `EnumDisplaySettings` refresh rate, overlay and hardware-composition support), the active display paths from `QueryDisplayConfig` (exact refresh rate as a fraction, connection type), DWM's timing, and how far DWM's counters move in 2 s while the program shows nothing.

**Predict** for your machine: how many adapters DXGI lists (count the software one), which one owns your monitor, and the refresh rate as a fraction.

Observed here, in the Remote Desktop session:

- Three DXGI adapters: "Intel(R) UHD Graphics 750" twice, with different LUIDs, and the software "Microsoft Basic Render Driver". The only output, `\\.\DISPLAY353`, 1440 × 900 at 32 Hz, hangs off the first Intel adapter. `QueryDisplayConfig` puts that display's path on the *second* LUID, which DXGI lists without outputs, and reports the connection type as "other" with no monitor name. `Win32_VideoController` lists a "Microsoft Remote Display Adapter" at 1440 × 900 and 32 Hz, and the Intel GPU at 1920 × 1080 and 60 Hz: that is the console's display, which this session cannot see. How the remote display adapter is built was not investigated.
- No overlays (`SupportsOverlays` FALSE, `CheckOverlaySupport` flags 0), no hardware composition (`CheckHardwareCompositionSupport` flags 0), tearing supported.
- DWM: refresh 32 Hz (`qpcRefreshPeriod` 31.250 ms), and in 2 s with nothing of ours on screen its counters advanced by one. That fits DWM composing only when something changes, not at every vblank.
- One `dwm.exe` runs per session (`Get-Process dwm` showed one in session 1 and one in session 2).

### B. A windowed flip-model swap chain, vsync on and off (run here)

```powershell
.\flipstats.exe windowed 4              # Present(1, 0)
.\flipstats.exe windowed 4 --vsync 0    # Present(0, DXGI_PRESENT_ALLOW_TEARING)
```

**Predict:** with vsync on, how long does `Present` block, and how long after a `Present` call does that present first appear in `GetFrameStatistics`? With vsync off, how many presents per second, and how many of them are ever shown?

Observed, two runs each (every run: 320 × 240, not the foreground window, no other window above it):

| Configuration | Presents per second | `Present` median | Presents never reported as the newest shown | `Present` to the first read reporting it, median (p99) | `CompositionMode` | DWM `cFrame` per second |
| --- | --- | --- | --- | --- | --- | --- |
| windowed, vsync on | 32.6 / 32.6 | 30.93 / 30.96 ms | 0 of 131 / 0 of 131 | 187.2 (218.8) / 187.2 (218.4) ms | `COMPOSED` 131 / 131 | 8.1 / 8.0 |
| windowed, vsync off | 5,864 / 5,686 | 0.110 / 0.117 ms | 23,329 of 23,458 / 22,615 of 22,744 | 34.7 (40.2) / 35.1 (40.1) ms | `COMPOSED` 129 / 129 | 9.9 / 9.9 |

What these runs show, and what they do not:

- **The output paces the program.** With `Present(1, 0)`, `Present` blocked for about one period of the 32 Hz output (median 30.9 ms; `WaitForVBlank` medians 31.2–31.4 ms). The program renders only as fast as frames leave the queue.
- **The queue is long here.** A present first appeared in `GetFrameStatistics` about 187 ms after its `Present` call: six periods of 31.25 ms. The statistics were read once per `Present`, so each value is late by up to one period. Exercise D removes two of those six periods. Where the other four go cannot be seen from these APIs.
- **With vsync off, almost every frame was thrown away.** `Present` returned in about 0.1 ms, about 5,800 times a second, and only about 32 presents per second were ever the newest one shown: 99.4 % of the frames were rendered for nothing. That is Learn's rule for a flip-model sync interval of 0 ([Present](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present)): the previous frame's remaining time is cancelled, and a frame is discarded if a newer one is queued. The latency dropped to about one period (35 ms), because nothing waited in a queue.
- **Tearing was allowed, and could not happen.** `DXGI_PRESENT_ALLOW_TEARING` was accepted, but DWM composed every frame (`COMPOSED`), and a composed frame is whole. Tearing needs the buffer on a plane (exercise E).
- **The presents-per-second fell during each vsync-off run**, for example from 8,717 to 3,679 per second in run 1. The cause was not investigated (clock or thermal limits, the remote display path).
- **DWM's counters disagree with DXGI's.** DXGI reported 32 new frames per second; DWM's `cFrame` and `cRefresh` advanced 8 times a second with vsync on and 10 with vsync off. Which of the two counts what was not determined. That question needs a trace, not more API calls.
- **Not shown:** whether PresentMon would call these presents `Composed: Flip`, any latency to the screen's light, and anything about the RTX 5070 Ti.

### C. Another window on top (run here)

```powershell
.\flipstats.exe overlap 4
.\flipstats.exe overlap 4 --vsync 0
```

The program creates a second, opaque, 160 × 120 window of its own over the lower-right part of the client area. Its log line `windows above ours that intersect its client area: 1 of this program, 0 others` confirms it, while counting only (no titles or classes of other windows are read).

**Predict:** on your console, which `PresentMode` will this window get, compared with exercise B and with exercise E?

Observed, two runs each:

| Configuration | Presents per second | `Present` median | Presents never reported as the newest shown | `Present` to the first read reporting it, median (p99) | `CompositionMode` | DWM `cFrame` per second |
| --- | --- | --- | --- | --- | --- | --- |
| overlap, vsync on | 32.6 / 32.8 | 30.96 / 30.97 ms | 0 of 131 / 0 of 132 | 187.2 (218.7) / 187.3 (188.3) ms | `COMPOSED` 131 / 132 | 8.1 / 8.1 |
| overlap, vsync off | 5,973 / 5,891 | 0.109 / 0.106 ms | 23,765 of 23,894 / 23,436 of 23,565 | 34.7 (39.8) / 35.0 (47.1) ms | `COMPOSED` 129 / 129 | 9.9 / 9.9 |

The overlap changed nothing here, and that is the expected result in a session where every frame is composed anyway. The comparison only means something where the window could have left composition, which is a full-screen window on a local display. There, the blog post describes three ways DWM can handle content on top of a directly flipped swap chain: fall back to composition, "reverse compose" the content on top into the app's buffer, or use MPO. Run exercise E with and without something on top, and find out which one your machine does.

### D. The present queue: maximum frame latency 1 (run here)

```powershell
.\flipstats.exe windowed 4 --latency 1     # IDXGIDevice1::SetMaximumFrameLatency(1)
```

The device reported a maximum frame latency of 3 by default. With 1:

| Configuration | Presents per second | `Present` median | `Present` to the first read reporting it, median (p99) |
| --- | --- | --- | --- |
| windowed, vsync on, latency 3 (exercise B) | 32.6 / 32.6 | 30.93 / 30.96 ms | 187.2 (218.8) / 187.2 (218.4) ms |
| windowed, vsync on, latency 1 | 32.4 / 32.3 | 31.17 / 31.18 ms | 124.9 (126.2) / 125.0 (126.3) ms |

The median fell by 62.3 ms, two periods of 31.25 ms. That fits two frames that had been waiting in the queue the default latency of 3 allows. `Present` still blocked for one period, so the frame rate did not change. In the second run, 3 of 130 presents were never seen as the newest one. With one read per `Present`, two presents shown within one read interval look like that, so this does not prove they were dropped. **Explain** why lowering the latency did not lower the frame rate, and what a waitable swap chain (`DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT`, not used here) would change.

### E. Borderless full screen (not executed here)

```powershell
.\flipstats.exe borderless 10
.\flipstats.exe borderless 10 --vsync 0
```

This covers your primary monitor for 10 s and takes the foreground; Esc ends it early. It was not run for this draft, because it would have covered the screen of the person using the machine.

What to look for, at your console with nothing else open on that monitor:

- The blog post's condition for DirectFlip: the swap chain's buffers match the screen's dimensions, and the window's client area covers the screen. Check the `window:` line: the client size must equal the monitor's mode, at your monitor's real DPI.
- `GetFrameStatistics` on a physical display should fill in `PresentRefreshCount`, `SyncRefreshCount` and `SyncQPCTime`. Then the program prints the vblank period it derives from them, the steps where a frame stayed on screen longer than its sync interval, and the time from `Present` to the vblank at which DXGI reports the frame shown. That code has never run (see the status block): check its numbers against `WaitForVBlank` and the display path's refresh rate before you trust it.
- `CompositionMode`: record whether it changes from `COMPOSED`, and compare it with PresentMon's `PresentMode` for the same run (exercise F). Neither Learn nor this lab says what `OVERLAY` means for an ordinary swap chain.
- With `--vsync 0` and Independent Flip, the white bar should tear, and `presents never reported as the newest one shown` will still be most of them.

### F. Capture with PresentMon (not executed here)

PresentMon was not downloaded for this draft, and none of these commands were run. Start the capture first, in an elevated PowerShell, and the program within the 3 s delay, in a normal one:

```powershell
# elevated: wait 3 s, record 8 s, then exit
.\PresentMon-2.6.0-x64.exe --process_name flipstats.exe --delay 3 --timed 8 --terminate_after_timed --output_file windowed-vsync1.csv
# normal prompt, at once:
.\flipstats.exe windowed 12 > windowed-vsync1-flipstats.txt
```

Repeat for the six runs, `windowed`, `overlap` and `borderless`, each with `--vsync 1` and `--vsync 0`, giving each CSV its own name. Keep the program's log of the same run beside each CSV. The options used, as the README describes them: `--process_name` records only processes with that executable name, `--delay` waits before recording, `--timed` stops recording after that many seconds, and `--terminate_after_timed` makes PresentMon exit when the timed capture ends. `--v1_metrics` writes PresentMon 1.x columns instead of the default set; check the header row of your CSV for the columns you got.

Read the `PresentMode` column. The README defines these values:

| `PresentMode` | Meaning, from the README |
| --- | --- |
| `Composed: Flip` | windowed, flip-model swap chain, surfaces shared with DWM to be composed |
| `Hardware: Independent Flip` | the app does not own the screen, but its surface is still swapped onto it every frame |
| `Hardware Composed: Independent Flip` | flip-model swap chain that has been granted a hardware overlay plane |
| `Hardware: Legacy Flip`, `Hardware: Legacy Copy to front buffer` | the app owns the screen (legacy full-screen exclusive) |
| `Composed: Copy with GPU GDI`, `Composed: Copy with CPU GDI` | bitblt-model presentation through a GDI-shared or DirectX redirection surface |

Count the modes and find the switches with PowerShell alone:

```powershell
Import-Csv windowed-vsync1.csv | Group-Object PresentMode | Select-Object Count, Name
$i = 0; $prev = ''
foreach ($r in Import-Csv borderless-vsync0.csv) {
    if ($r.PresentMode -ne $prev) { "{0,5}  {1}  AllowsTearing={2}" -f $i, $r.PresentMode, $r.AllowsTearing; $prev = $r.PresentMode }
    $i++
}
```

Also look at `AllowsTearing`, `SyncInterval`, `PresentFlags`, and the display columns (`DisplayedTime`, `DisplayLatency`, or with `--v1_metrics` `MsBetweenDisplayChange` and `MsUntilDisplayed`). Compare `PresentMode` with the program's `CompositionMode` for the same run, and the display timing with the program's `Present`-to-report times. Expect the first frames of a borderless run to be composed before a switch, and record after how many frames it happened.

### G. Make it switch, and press Win+Ctrl+Shift+B (not executed here)

During a borderless capture (`--vsync 0`, a longer `--timed`), make something appear over the program: press a volume key so the volume flyout appears, open the Start menu, or press Alt+Tab. Find each switch in the CSV and match it to what you did.

Then the question from [the display wake case](../../../pc-troubleshooting/cases/2026-10-14-display-wake/README.md): what does the Win+Ctrl+Shift+B graphics reset restart? Microsoft's support page (cited there) describes it as asking Windows to reset the graphics driver. Run `.\flipstats.exe windowed 20` at your console, press Win+Ctrl+Shift+B during the run, and record:

1. Did `Present` return an error? The program stops and prints it, with `GetDeviceRemovedReason` for `DXGI_ERROR_DEVICE_REMOVED` or `DXGI_ERROR_DEVICE_RESET`.
2. If the run went on: did `GetFrameStatistics` return `DXGI_ERROR_FRAME_STATISTICS_DISJOINT` again, and did DWM's counters restart (compare `before:` and `after:`)?
3. Did the window keep updating, and did the screen flicker?

Compare with a TDR as Learn describes it: the GPU scheduler in `Dxgkrnl` finds a task that cannot be completed or preempted within the timeout (two seconds by default), the miniport's `DxgkDdiResetFromTimeout` resets the GPU, the video memory manager purges all allocations, and applications must release and re-create their Direct3D devices. Learn also says the OS bug-checks on the sixth hang when five or more hangs and recoveries occur within one minute. **Do not cause a TDR on this or any machine you work on.** Stage 17 submits a never-ending GPU job on purpose; the TDR registry keys (`TdrDelay` and others) are for driver testing and are not touched in this course.

### H. Draw the three presentation paths

Draw X11, Wayland and Windows side by side, one frame each, from the client's render to scanout. For each, mark who owns the display hardware (the X server, the compositor through KMS, `Dxgkrnl` and DWM), where composition happens, where a buffer is copied or shared, and where a client can reach the screen without composition (an unredirected full-screen X client, direct scanout of a client buffer by a Wayland compositor, Independent Flip). Use the syllabus's [stage 10](../../SYLLABUS.md#10-window-system-architecture-measured-on-windows) for X11 and Wayland, and your PresentMon results for Windows.

## Learn as needed

- **WDDM layers.** The Direct3D runtime and the vendor's user-mode driver (UMD) run inside your process. They reach the kernel through `gdi32` thunks into `Dxgkrnl`, which contains the GPU scheduler (VidSch) and the video memory manager (VidMm, in `dxgmms2.sys` for WDDM 2.0 and later). The vendor's kernel-mode miniport driver (KMD) drives the hardware ([WDDM architecture](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/windows-vista-and-later-display-driver-model-architecture)). DWM runs in user mode, one process per session (exercise A).
- **Flip model.** In the bitblt model the runtime copies your back buffer into a redirection surface at every present, and DWM composes from that copy. In the flip model "all back buffers are shared with the Desktop Window Manager", which composes from them directly ([flip model](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-flip-model)). The same page asks for 2–16 buffers and one of three formats, and says MSAA is not supported.
- **Direct Flip, Independent Flip, MPO.** When a flip-model swap chain's buffers fill the screen, DWM can scan them out instead of its own (DirectFlip), possibly with the scanout hardware scaling them or on an overlay plane (MPO). Once a swap chain is direct-flipped, DWM can stop composing until something else changes, and the app's frames go to the screen "independently" (Independent Flip) ([blog](https://devblogs.microsoft.com/directx/dxgi-flip-model/)).
- **Multiplane overlays.** Introduced with WDDM 1.3 in Windows 8.1, they let the display hardware combine several planes (a video, an app's window, the desktop) into one image, instead of blending them in software ([MPO](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/multiplane-overlay-support)). Whether a frame got one is what `Hardware Composed: Independent Flip` reports.
- **Sync interval and tearing.** For the flip model, a sync interval of 0 cancels the rest of the previous frame's time and discards a frame that a newer one replaces; 1–4 holds a frame for at least that many vblanks. Variable refresh rate displays require tearing to be enabled ([Present](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/nf-dxgi-idxgiswapchain-present)). Stage 13 meets VRR as the KMS property `VRR_ENABLED`.
- **The present queue.** `IDXGIDevice1::SetMaximumFrameLatency` bounds how many frames the CPU may queue ahead (3 by default here); when the queue is full, `Present` blocks (exercise D).
- **TDR.** Detection by the GPU scheduler, a two-second default timeout, reset through the miniport, and applications that must re-create their devices ([TDR](https://learn.microsoft.com/en-us/windows-hardware/drivers/display/timeout-detection-and-recovery)). Exercise G compares it with Win+Ctrl+Shift+B.
- **Why DWM cannot be replaced.** Since Windows 8 composition cannot be turned off programmatically ([DwmEnableComposition](https://learn.microsoft.com/en-us/windows/win32/api/dwmapi/nf-dwmapi-dwmenablecomposition)). The only documented path for your own compositor, `Windows.Devices.Display.Core`, covers only displays that DWM ignores: head-mounted and specialized displays, which leave the desktop. Indirect display drivers add monitors and receive frames from the desktop compositor, rather than replacing it. The syllabus has the sources. A Windows program can choose how it presents; it cannot choose to be the one that composes the desktop.

## Linux / NT comparison

| Windows | Linux (stage 13 onwards) |
| --- | --- |
| DWM composes the desktop, and decides when a swap chain may bypass it | the Wayland compositor composes, and is the one that decides on direct scanout |
| Independent Flip: a full-screen flip-model buffer scanned out without composition | direct scanout: the compositor puts a client's dma-buf on a plane (capstone milestone 4) |
| MPO: `Hardware Composed: Independent Flip` | KMS overlay planes, tried with test-only atomic commits |
| `GetFrameStatistics`: present count and vblank count | page-flip events: a sequence number and a monotonic timestamp |
| `IDXGIOutput::WaitForVBlank` | `drmWaitVBlank` |
| `DXGI_PRESENT_ALLOW_TEARING` and VRR | `DRM_MODE_PAGE_FLIP_ASYNC`, `tearing-control-v1`, `VRR_ENABLED` |
| TDR resets the GPU and removes devices | the GPU driver's reset; the compositor must survive it (stage 17) |

The difference that drives this course: on Linux you can write the compositor, and stage 18 does.

## Checkpoints

Answer these in `NOTES.md`.

1. From your PresentMon logs: which frames DWM composed, which reached the screen without composition, and why each switch happened, with frame numbers from the CSV and the action that caused each.
2. For each source in [the evidence table](#the-evidence-and-what-each-piece-can-prove), one sentence on what your run showed and one on what it could not show. Did `CompositionMode` agree with `PresentMode` in every run?
3. List what a Windows developer can and cannot change in this path. Can: the swap effect, buffer count, format, sync interval, the tearing flag, the frame latency, the window's style, size and DPI awareness. Cannot: whether DWM composes a given frame, whether an overlay plane is granted, DWM itself, the vblank. Add what your runs taught you.
4. Your drawing from exercise H, with one sentence per system on who decides that a frame may skip composition.
5. What does Win+Ctrl+Shift+B restart? Answer from exercise G. Separate what Microsoft documents from what you observed and what you infer, and compare it with a TDR and with the KMS hotplug events of stage 13.
6. Why a desktop compositor of your own cannot exist on Windows, in three documented facts, and what that means for where this course's capstone runs.

## Common failure modes

- **`undefined reference to IID_IDXGIFactory2`** (or another `IID_`). Include `<initguid.h>` before the DXGI headers in exactly one file, as `flipstats.c` does, or link `-ldxguid`.
- **`SM_REMOTESESSION = 1`, 32 Hz, every frame `COMPOSED`, the vblank fields 0.** You are in Remote Desktop, like the recorded runs. Sign in at the console.
- **Borderless never leaves composition.** Something is on top (see `windows above ours … others`), the client size is not the monitor's mode, the process is not DPI-aware so Windows scales it (the program sets per-monitor awareness; check the `DPI` value), or the driver does not offer it on this output. Check `CheckHardwareCompositionSupport` and PresentMon rather than guessing.
- **`Present` returns `DXGI_ERROR_INVALID_CALL` with `--vsync 0`.** `DXGI_PRESENT_ALLOW_TEARING` needs a swap chain created with `DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING`. The program sets both only when the system reports tearing support.
- **Alt+Enter switches to exclusive full screen.** Not in this program: `MakeWindowAssociation(..., DXGI_MWA_NO_ALT_ENTER)`.
- **PresentMon records nothing.** It is not elevated, the process name is wrong, or the program ended before the delay was over.
- **The numbers look like one run's accident.** Run each configuration at least twice, and report the spread, as the tables here do.

## Where this shows up later

- Stage 13 replaces `GetFrameStatistics` with page-flip events, and `CheckOverlaySupport` with test-only commits on VKMS planes. The hardware validation repeats this lab's timing questions on a real display.
- Stage 14 shares a D3D12 resource and a fence between two processes: a buffer handed across a process boundary, as your swap chain's buffers are handed to DWM.
- Stage 17 submits a GPU job that never ends and watches TDR. Its `WDA_EXCLUDEFROMCAPTURE` works only while DWM composes, the composition this lab measures.
- Stage 18's milestone 4 is the Linux version of Independent Flip: direct scanout of a full-screen client buffer.

## Next

Stage 11 of the [build plan](../../SYLLABUS.md#11-wayland-from-the-client-side) writes a Wayland client without a toolkit, in WSLg.
