# Part II build plan — native GPU rendering (draft)

**English** | [繁體中文](SYLLABUS-PART2.zh-TW.md)

> **Status: draft for review.** This plan has not been reviewed or approved yet, and none of its stages is implemented. Stage names, order, and hour estimates may change after review.

Part I (labs 01–08 in [`SYLLABUS.md`](SYLLABUS.md)) builds a renderer in the browser: a CPU rasterizer first, then WebGL 2. Part II takes the same ideas to native code with Vulkan (or Direct3D 12) on the GeForce RTX 5070 Ti and aims at one of two end goals:

- **Engine track:** the renderer at the core of a game engine.
- **2D, text and compositing rendering track:** the drawing and compositing layers under a desktop user interface: vector graphics, text, and a GPU compositor.

Both tracks start with a **shared core** on presentation, synchronization, compute, measurement, and color. Do the shared core, then one track. The other track is optional.

## Where this course runs

**The whole graphics course, Parts I and II, can be completed on Windows with the RTX 5070 Ti.** No Linux machine is needed for any stage or capstone in this plan. Linux is needed only for the separate OS research described at the end.

Tools you install yourself:

- A C++ toolchain: MSVC (Visual Studio) or clang, plus CMake.
- The LunarG Vulkan SDK (validation layers, shader compilers, `vulkaninfo`); version 1.4.363.1 was current for Windows when this draft was written. Choose Direct3D 12 instead if you prefer; then you need the Windows SDK and optionally the DirectX 12 Agility SDK.
- NVIDIA Nsight Graphics and RenderDoc for frame capture, and PresentMon for frame-time and latency measurements. You run all captures on the RTX 5070 Ti yourself.

Record the API you chose (Vulkan or Direct3D 12) in your notes at the start, and keep it for the whole of Part II. Every stage below names the Vulkan concept first and the Direct3D 12 counterpart in parentheses.

## Before you start

- Labs 01–06 of Part I finished, with your notes. Labs 07–08 (shadows, final scene) are recommended.
- Comfortable with C or C++ and a debugger. C++ beyond that (ownership, RAII, build systems) is part of stage C0.
- [`CONVENTIONS.md`](CONVENTIONS.md) read and understood. Part II changes the NDC y direction, the depth range, and the front-face setting; every port starts from that checklist.

## How this part is paced and assessed

Every stage keeps Part I's loop: **predict → implement → inspect → explain**, with a written checkpoint. In addition:

- Every stage ends with **zero validation-layer errors** (or zero D3D12 debug-layer errors) in normal use.
- Every performance claim needs **evidence**: a GPU timestamp, a PresentMon log, or an Nsight Graphics / RenderDoc capture. Utilization percentages alone do not count.
- Hours are rough estimates for a student who already finished Part I. Expect the first native stages to take longer than planned.

---

## Shared core

### C0 — A native renderer skeleton

**Prerequisites:** Part I labs 01–06. **Time:** 30–50 h.

**Build:** a CMake project that opens a window, creates a Vulkan 1.3 or 1.4 instance and device with validation layers enabled, and clears the screen through a swapchain every frame. (Direct3D 12: device, command queue, flip-model swap chain, debug layer.)

**Learn as needed:** C++ ownership and RAII for GPU objects, error handling for API results, CMake targets, debugger use, how validation layers report misuse.

**Checkpoint:** list every object you create, who owns it, and when it is destroyed. Show a clean shutdown with no validation messages. Break one rule on purpose (for example, destroy a buffer that is still in use) and explain the validation message.

### C1 — Draw with explicit synchronization

**Prerequisites:** C0. **Time:** 50–80 h.

**Build:** port the Lab 05–06 cube with lighting and a texture. Allocate buffers and images (with Vulkan Memory Allocator, or D3D12 Memory Allocator), upload through a staging buffer, use dynamic rendering, push constants or descriptor sets for per-frame data, and run two or three **frames in flight**.

**Learn as needed:** command buffers and queues, pipeline objects, descriptor sets (root signatures and descriptor heaps), image layouts, `synchronization2` barriers (resource or enhanced barriers), fences and semaphores, the [0, 1] depth range and y flip from `CONVENTIONS.md`.

**Checkpoint:** explain every barrier in one frame: what it waits for, what it makes visible, and why. Remove one barrier on purpose, catch it with the synchronization validation layer, and describe what could go wrong on real hardware.

### C2 — Presentation and frame pacing (application side)

**Prerequisites:** C1. **Time:** 20–30 h.

**Build:** a settings panel that switches the present mode (FIFO, MAILBOX, IMMEDIATE where supported; Direct3D 12: flip-model sync intervals and tearing support), the number of frames in flight, and an artificial CPU or GPU load. Log frame times and present-to-display latency.

**Learn as needed:** swapchain images and present modes (only FIFO is guaranteed to exist), vsync from the application's point of view, tearing, frame queues and latency, waiting for a present before starting the next frame (present-wait or a frame-latency waitable object), and PresentMon metrics such as `DisplayLatency` and `MsClickToPhotonLatency`.

**Checkpoint:** plot frame time and latency for three configurations under the same load. Explain the trade-off between smoothness, tearing, and latency, and choose a default for a game and for a desktop tool.

### C3 — Compute shaders and the GPU's execution model

**Prerequisites:** C1. **Time:** 40–60 h.

**Build:** compute kernels for a blur, a histogram, and a prefix sum on images or buffers, each in a naïve and an optimized version (shared memory, subgroup operations, better memory access).

**Learn as needed:** workgroups and dispatches, subgroups (waves / warps), shared memory, occupancy and latency hiding, memory coalescing, atomics, compute-to-graphics synchronization.

**Checkpoint:** for each kernel, show timestamp measurements of both versions and an Nsight Graphics GPU Trace that explains the difference with evidence (for example, memory throughput or occupancy), not guesses.

### C4 — Measure the GPU

**Prerequisites:** C1, C3. **Time:** 20–30 h.

**Build:** GPU timestamp queries around every pass, an on-screen frame-time graph, and a written capture walkthrough of one frame in Nsight Graphics and one in RenderDoc.

**Learn as needed:** timestamp queries (`vkCmdWriteTimestamp2` and `timestampPeriod`; Direct3D 12 timestamp query heaps and `GetTimestampFrequency`), CPU versus GPU time, Nsight Graphics' Graphics Capture versus GPU Trace (the older Frame Debugger is legacy for Direct3D 12 and Vulkan), reading pipeline state and resource contents from a capture, bottleneck hypotheses (vertex, fragment, bandwidth, CPU submission, synchronization).

**Checkpoint:** give a frame-time budget for your current renderer (for example, 16.6 ms at 60 Hz split by pass) and defend one bottleneck claim with a capture and a controlled experiment.

### C5 — Color science: linear light, sRGB, HDR, color management

**Prerequisites:** C1, Part I Lab 06. **Time:** 30–50 h.

**Build:** a color test scene with gray ramps, a checkerboard, saturated primaries, and bright highlights. Render it to an sRGB swapchain and to an HDR swapchain: extended-range linear (scRGB, FP16) or PQ-encoded (HDR10). Add a simple tone mapper and an SDR-white-level control.

**Learn as needed:** transfer functions versus primaries, sRGB formats and where the hardware decodes and encodes, floating-point render targets, scRGB (linear, Rec. 709 primaries, where 1.0 means 80 nits on an HDR display but the display's reference white on an SDR one) and HDR10 (PQ, SMPTE ST 2084, with Rec. 2020 primaries), tone mapping, and how Windows Advanced Color and Auto Color Management handle SDR and HDR displays.

**Checkpoint:** trace one pixel value from a texture through lighting, tone mapping, and encoding to the display signal, naming the color space at every step. Show one deliberate mistake (double encoding or blending in encoded space) and its visible result.

---

## Engine track

Goal: the rendering core of a game engine: data-driven passes, physically based materials, shadows, post-processing, GPU-driven culling, and ray tracing, with measured performance.

### E1 — Render graph and resource lifetimes

**Prerequisites:** shared core. **Time:** 40–60 h.

**Build:** a small render graph: each pass declares what it reads and writes, and the graph orders the passes, inserts barriers, and creates or reuses (aliases) transient images.

**Learn as needed:** frame graphs, resource lifetimes, automatic barrier placement, memory aliasing, culling unused passes.

**Checkpoint:** show the graph for one frame, the barriers it generated, and the memory saved by aliasing. Add a pass without editing any barrier by hand.

### E2 — Material and shader system

**Prerequisites:** E1. **Time:** 40–60 h.

**Build:** shaders compiled offline to SPIR-V (or DXIL), reflection to build descriptor layouts, a material description format, shader permutations for a few features, and a pipeline cache.

**Learn as needed:** GLSL or HLSL to SPIR-V, shader reflection, specialization constants, pipeline creation cost and caching, hot reload.

**Checkpoint:** add a new material feature end to end, report how many pipelines exist and how long they take to build, and show hot reload working.

### E3 — Physically based materials and image-based lighting

**Prerequisites:** E2, C5. **Time:** 50–80 h.

**Build:** the metallic–roughness material model used by glTF, with direct lights and image-based lighting from an environment map (prefiltered specular, irradiance, and a BRDF lookup table).

**Learn as needed:** microfacet BRDFs (GGX, Smith, Schlick Fresnel), energy conservation, the split-sum approximation, cube maps and mip-based roughness, linear-light HDR rendering.

**Checkpoint:** render a roughness/metalness sphere grid and compare it with a reference viewer. Explain one visible difference.

### E4 — Cascaded shadow maps

**Prerequisites:** E1, Part I Lab 07. **Time:** 30–50 h.

**Build:** directional-light shadows with cascades, PCF filtering, cascade blending, and stabilization against shimmering.

**Learn as needed:** cascade splits, texel snapping, depth bias and slope bias, shadow-map resolution versus coverage.

**Checkpoint:** visualize the cascades, measure the cost per cascade, and show one artifact (acne, peter-panning, or shimmering) with its fix.

### E5 — Post-processing: tone mapping, bloom, and temporal anti-aliasing

**Prerequisites:** E3. **Time:** 40–60 h.

**Build:** an HDR post chain: bloom, exposure and tone mapping, and temporal anti-aliasing (TAA) with jittered projection, motion vectors, history reprojection, and history rejection.

**Learn as needed:** HDR render targets, mip-chain bloom, sub-pixel jitter, motion vectors, neighborhood clamping, ghosting versus blur trade-offs.

**Checkpoint:** compare no AA, TAA, and TAA with a broken motion vector. Explain each artifact you see, with zoomed captures.

### E6 — GPU-driven culling and indirect drawing

**Prerequisites:** E1, C3. **Time:** 50–80 h.

**Build:** compute-shader frustum and occlusion culling (hierarchical Z) that writes indirect draw commands, then one multi-draw-indirect-count call (Direct3D 12: `ExecuteIndirect`) for thousands of objects.

**Learn as needed:** indirect draw arguments, draw-count buffers, Hi-Z pyramids, two-phase occlusion culling, CPU versus GPU submission cost. Mesh shaders are an optional extension.

**Checkpoint:** show CPU time and GPU time for 10,000 objects with CPU culling versus GPU culling, and explain where each version spends its time.

### E7 — Hardware ray tracing

**Prerequisites:** E3, C3. **Time:** 50–80 h.

**Build:** bottom- and top-level acceleration structures for your scene, then ray-traced shadows or ambient occlusion with ray queries from a compute or fragment shader (Direct3D 12: DXR inline ray tracing). A full ray-tracing pipeline is optional.

**Learn as needed:** acceleration structure build and update, instancing, ray queries versus ray-tracing pipelines, denoising basics, cost per ray. Use Nsight Graphics to inspect ray tracing work: RenderDoc captures it but cannot debug it.

**Checkpoint:** compare shadow maps from E4 with ray-traced shadows in image quality and measured cost, and justify which one you would ship.

### E8 — Multithreaded recording, asset pipeline, and debug UI

**Prerequisites:** E1, E2. **Time:** 40–60 h.

**Build:** command recording on several threads, a glTF 2.0 loader, an offline texture step that produces block-compressed (BCn) textures in KTX2 or DDS containers, and an in-engine debug UI (for example, Dear ImGui) for toggles and GPU timings.

**Learn as needed:** command pools per thread, secondary command buffers or bundles, job systems, glTF scenes and materials, BC1/BC5/BC7/BC6H formats and when to use each, the cost of loading versus streaming.

**Checkpoint:** show load-time and frame-time numbers before and after compression and threading, and the debug UI controlling at least three renderer features.

### Engine capstone — a mini engine renderer

**Prerequisites:** E1–E8. **Time:** 80–150 h.

**Build:** a renderer written from scratch on your Part II code that loads a glTF scene and renders it with PBR materials, shadows, post-processing and TAA, GPU-driven culling, and at least one ray-traced effect. Include a debug view per major pass.

**Deliver:** source code, a short video, and a **frame-time budget report**: per-pass GPU times at a stated resolution and present mode, measured on the RTX 5070 Ti, with one Nsight Graphics capture that supports your largest optimization claim.

---

## 2D, text and compositing rendering track

Goal: the rendering layers under a desktop UI: vector paths, text, and a compositor that combines surfaces correctly and efficiently. Keep each module's interface clean: **surfaces in, composited frame out**. A planned OS research effort will reuse these modules (see the last section).

### T1 — Vector paths on the CPU

**Prerequisites:** shared core (C5 at least), Part I Lab 01. **Time:** 50–80 h.

**Build:** a CPU path rasterizer: lines and quadratic/cubic Bézier curves, flattening, nonzero and even-odd fill rules, exact-area coverage anti-aliasing, and stroking with joins, caps, and dashes.

**Learn as needed:** Bézier math and adaptive subdivision, signed-area accumulation, fill rules, stroke outlines and their failure cases, why coverage must be blended in linear light.

**Checkpoint:** render a test sheet of paths and strokes and compare it with a reference renderer at 400 % zoom. Explain every difference.

### T2 — Vector paths on the GPU

**Prerequisites:** T1, C3. **Time:** 60–100 h.

**Build:** move path rendering to the GPU with compute shaders: bin path segments into tiles, compute coverage per tile, and composite. Study the Vello family of open-source renderers and compare them with your CPU version: the compute-centric research renderer (alpha), and the newer "sparse strips" renderers, `vello_cpu` and `vello_gpu`.

**Learn as needed:** tiling and binning, prefix sums on the GPU, sparse versus dense coverage, conflation artifacts, memory budgets for unbounded scenes.

**Checkpoint:** measure CPU versus GPU time for a scene with thousands of paths, and show one case where each approach wins.

### T3 — Text rendering

**Prerequisites:** T1, C5. **Time:** 60–100 h.

**Build:** a text pipeline: shape text with HarfBuzz, rasterize glyphs (FreeType or your T1 rasterizer), cache them in a glyph atlas, lay out paragraphs with line breaking, and draw them on the GPU. Compare grayscale anti-aliasing, hinting, and subpixel positioning, and try subpixel (LCD) rendering. Optional comparisons: a multi-channel signed-distance-field atlas (msdfgen), and HarfBuzz's newer raster and GPU libraries, which grew out of a project that started as shaping only.

**Learn as needed:** what shaping does (ligatures, kerning, complex scripts), bidirectional text and line breaking at an overview level, glyph metrics, hinting (FreeType does it; HarfBuzz does not), gamma and contrast of text, why subpixel AA breaks under transparency and rotation, and why Microsoft now recommends grayscale antialiasing for most Windows text.

**Checkpoint:** render the same paragraph in a Latin script, Traditional Chinese, and one complex script, at 100 % and 150 % scale. Explain the rendering choices and one artifact.

### T4 — Compositing

**Prerequisites:** T3, C2. **Time:** 50–80 h.

**Build:** a compositor library that takes a list of surfaces (position, size, opacity, transform, clip with rounded corners) and produces one frame. Use premultiplied alpha and blend in linear light, track damage so only changed regions are redrawn, skip surfaces hidden behind opaque ones, and render correctly at fractional scale factors.

**Learn as needed:** premultiplied alpha and Porter–Duff operators, layer trees, damage regions, occlusion culling for opaque regions, HiDPI and per-monitor DPI awareness (v2) on Windows, pixel snapping at fractional scales.

**Platform note:** damage tracking always saves rendering work, because you redraw only damaged regions into the composited image. Telling the *presentation engine* which regions changed is a separate step. Direct3D 12 can do it through DXGI's `Present1` with dirty rectangles, which needs the `FLIP_SEQUENTIAL` swap effect rather than `FLIP_DISCARD`. Vulkan's `VK_KHR_incremental_present` extension is not reported by the RTX 5070 Ti's Windows driver in the public database we checked, so a Vulkan version presents full frames. Check `vulkaninfo` on your own machine and record what you find.

**Checkpoint:** show damage regions on screen, the number of pixels redrawn per frame, and a scene where blending in encoded sRGB visibly differs from blending in linear light. State whether your presentation is partial or full-frame, and why.

### T5 — Color management and HDR for desktop content

**Prerequisites:** T4, C5. **Time:** 30–50 h.

**Build:** composite SDR surfaces (sRGB) and HDR surfaces into an HDR output, with a controllable SDR white level, and keep SDR content looking unchanged.

**Learn as needed:** mixing SDR and HDR, reference white (on Windows, scale SDR content by the display's SDR white level divided by 80 nits when composing in scRGB), tone mapping for UI, color spaces of surfaces, ICC profiles at an overview level.

**Checkpoint:** explain how one sRGB white pixel ends up as a display signal in your HDR output, with numbers at each step.

### 2D/text/compositing capstone — a single-process GPU compositor

**Prerequisites:** T1–T5. **Time:** 80–150 h.

**Build:** one Windows application, using Vulkan or Direct3D 12 on the RTX 5070 Ti, that composites several surfaces, such as simulated windows with vector graphics and text drawn by your T1–T3 modules. It uses damage tracking, occlusion culling, and correct color handling, including an HDR mode. Record which API you chose and why.

**Deliver:** source code, a short video, and a measurement report: frame time, pixels redrawn per frame, and input-to-display latency measured with PresentMon (for example `DisplayLatency` and `MsClickToPhotonLatency`) and your own timestamps, for an idle desktop, one animating window, and a worst case. Say whether you used partial presentation.

---

## Beyond Part II: planned OS research

Window-system architecture, kernel display interfaces, hardware display planes, cross-process buffer sharing, input stacks, and a real system compositor are **not** part of this course. They continue in a separate OS research effort planned for this repository. Its folder does not exist yet. That research needs Linux; nothing in Parts I and II does.

The 2D, text and compositing modules from this track are meant to be reused there. Keep their interfaces narrow (surfaces in, composited frame out) so that a later system compositor can drive them without changes.

## What completing Part II means

- **Done:** you built the capstone of your track from scratch, with measurements that support your design choices, and you can read and explain the matching parts of open-source projects (for example, Godot's renderer for the engine track, or Skia and Vello for the 2D and text track). A stronger goal is getting a patch accepted in one such project.
- **Not claimed:** senior or production-level expertise. That still needs time on a real team, with real content and real hardware variety. This plan prepares you for that work; it does not replace it.

## Rough hours

| Part | Stages | Hours |
| --- | --- | --- |
| Shared core | C0–C5 | 190–300 |
| Engine track | E1–E8 and capstone | 420–680 |
| 2D, text and compositing track | T1–T5 and capstone | 330–560 |
| **Shared core plus one track** | | **about 520–980** |

These are estimates for planning, not promises. Track your real hours per stage and adjust.

## Sources

Checked against the official pages on 2026-11-15. Versions move quickly; check the current ones before you install anything.

**APIs, conventions, and tools (shared core)**

- Vulkan specification: [viewport transform](https://docs.vulkan.org/spec/latest/chapters/vertexpostproc.html#vertexpostproc-viewport), [clipping](https://docs.vulkan.org/spec/latest/chapters/vertexpostproc.html#vertexpostproc-clipping), [polygon orientation](https://docs.vulkan.org/spec/latest/chapters/primsrast.html#primsrast-polygons-basic), [what each version promoted](https://docs.vulkan.org/spec/latest/appendices/versions.html#versions-1.4-promotions); [Khronos Vulkan 1.4 announcement](https://www.khronos.org/news/press/khronos-streamlines-development-and-deployment-of-gpu-accelerated-applications-with-vulkan-1.4) (December 2024).
- [LunarG Vulkan SDK](https://www.lunarg.com/products/vulkan-sdk/) and [getting started on Windows](https://vulkan.lunarg.com/doc/view/latest/windows/getting_started.html); [Vulkan Memory Allocator](https://gpuopen.com/vulkan-memory-allocator/) and [D3D12 Memory Allocator](https://gpuopen.com/d3d12-memory-allocator/).
- Presentation: [Vulkan present modes](https://docs.vulkan.org/refpages/latest/refpages/source/VkPresentModeKHR.html) (only FIFO is guaranteed), [`VK_KHR_present_wait2`](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_present_wait2.html); DXGI [swap effects](https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ne-dxgi-dxgi_swap_effect), [tearing support](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/variable-refresh-rate-displays), [frame-latency waitable object](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_3/nf-dxgi1_3-idxgiswapchain2-getframelatencywaitableobject).
- Direct3D 12: [rasterizer state and front-face default](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_rasterizer_desc), [Agility SDK](https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/), [enhanced barriers](https://microsoft.github.io/DirectX-Specs/d3d/D3D12EnhancedBarriers.html), [GPU timing](https://learn.microsoft.com/en-us/windows/win32/direct3d12/timing).
- Measurement: [Vulkan `timestampPeriod`](https://docs.vulkan.org/refpages/latest/refpages/source/VkPhysicalDeviceLimits.html); [Nsight Graphics](https://developer.nvidia.com/nsight-graphics/get-started) (Graphics Capture and GPU Trace for Direct3D 12 and Vulkan; supports RTX 50-series); [RenderDoc](https://renderdoc.org/) and its [ray tracing limits](https://renderdoc.org/docs/behind_scenes/raytracing.html) (captured, but not debuggable); [PresentMon metrics](https://raw.githubusercontent.com/GameTechDev/PresentMon/main/README-ConsoleApplication.md).
- Hardware: [GeForce RTX 5070 family specifications](https://www.nvidia.com/en-us/geforce/graphics-cards/50-series/rtx-5070-family/) (Vulkan 1.4, OpenGL 4.6, DirectX 12 Ultimate).
- Color: [sRGB, IEC 61966-2-1](https://webstore.iec.ch/en/publication/6169) (formulas also in [CSS Color 4](https://www.w3.org/TR/css-color-4/)); [Advanced Color and HDR in DirectX](https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range); [Auto Color Management](https://devblogs.microsoft.com/directx/auto-color-management); Vulkan [swapchain color spaces](https://docs.vulkan.org/refpages/latest/refpages/source/VkColorSpaceKHR.html) and [HDR metadata](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_hdr_metadata.html).

**Engine track**

- Render graphs: Yuriy O'Donnell, [FrameGraph: Extensible Rendering Architecture in Frostbite](https://www.gdcvault.com/play/1024612/FrameGraph-Extensible-Rendering-Architecture-in) (GDC 2017).
- PBR and IBL: Brian Karis, [Real Shading in Unreal Engine 4](https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf) (SIGGRAPH 2013); [Physically Based Rendering in Filament](https://google.github.io/filament/Filament.html).
- Shadows and AA: [Cascaded Shadow Maps](https://learn.microsoft.com/en-us/windows/win32/dxtecharts/cascaded-shadow-maps); Brian Karis, [High-Quality Temporal Supersampling](https://advances.realtimerendering.com/s2014/) (SIGGRAPH 2014).
- GPU-driven rendering: [`vkCmdDrawIndexedIndirectCount`](https://docs.vulkan.org/refpages/latest/refpages/source/vkCmdDrawIndexedIndirectCount.html) (core since 1.2), [`ExecuteIndirect`](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-executeindirect), [`VK_EXT_mesh_shader`](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_mesh_shader.html) (optional).
- Ray tracing: [Vulkan ray tracing final specification](https://www.khronos.org/blog/vulkan-ray-tracing-final-specification-release); [DXR 1.1 and inline ray tracing](https://devblogs.microsoft.com/directx/dxr-1-1/).
- Assets and tools: [glTF](https://www.khronos.org/gltf/) (also ISO/IEC 12113:2022); [KTX 2.0](https://www.khronos.org/ktx/) and [`ktx create`](https://github.khronos.org/KTX-Software/ktxtools/ktx_create.html); [texconv](https://github.com/microsoft/DirectXTex/wiki/Texconv); [block compression formats](https://learn.microsoft.com/en-us/windows/win32/direct3d11/texture-block-compression-in-direct3d-11); [Dear ImGui backends](https://github.com/ocornut/imgui/blob/master/docs/BACKENDS.md); [Godot Engine source](https://github.com/godotengine/godot).

**2D, text and compositing track**

- Vector rendering: [Vello](https://github.com/linebender/vello) and its [architecture notes](https://github.com/linebender/vello/blob/main/ARCHITECTURE.md); Raph Levien, [Fast 2D rendering on GPU](https://raphlinus.github.io/rust/graphics/gpu/2020/06/13/fast-2d-rendering.html); [Inside the fastest font renderer in the world](https://medium.com/@raphlinus/inside-the-fastest-font-renderer-in-the-world-75ae5270c445) (signed-area accumulation); Diego Nehab, [Converting Stroked Primitives to Filled Primitives](https://w3.impa.br/~diego/projects/Neh20/index.html) (SIGGRAPH 2020); [Skia](https://skia.org/).
- Text: HarfBuzz, [What is text shaping?](https://harfbuzz.github.io/what-is-harfbuzz.html#what-is-text-shaping) and [What HarfBuzz doesn't do](https://harfbuzz.github.io/what-harfbuzz-doesnt-do.html); [FreeType](https://freetype.org/); [DirectWrite](https://learn.microsoft.com/en-us/windows/win32/directwrite/introducing-directwrite); [msdfgen](https://github.com/Chlumsky/msdfgen); [UAX #9 bidirectional text](https://www.unicode.org/reports/tr9/); [UAX #14 line breaking](https://www.unicode.org/reports/tr14/); [OpenType specification](https://learn.microsoft.com/en-us/typography/opentype/spec/).
- Compositing: Porter and Duff, [Compositing Digital Images](https://history.siggraph.org/?p=107468) (SIGGRAPH 1984); [`DXGI_ALPHA_MODE`](https://learn.microsoft.com/en-us/windows/win32/api/dxgi1_2/ne-dxgi1_2-dxgi_alpha_mode); [DXGI dirty rectangles](https://learn.microsoft.com/en-us/windows/win32/direct3ddxgi/dxgi-1-2-presentation-improvements); [`VK_KHR_incremental_present` support data](https://vulkan.gpuinfo.org/displayextensiondetail.php?extension=VK_KHR_incremental_present); [DPI awareness contexts](https://learn.microsoft.com/en-us/windows/win32/hidpi/dpi-awareness-context) and [`WM_DPICHANGED`](https://learn.microsoft.com/en-us/windows/win32/hidpi/wm-dpichanged).
