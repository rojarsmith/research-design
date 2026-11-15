# Computer Graphics Practice

**English** | [繁體中文](README.zh-TW.md) | [Open the interactive course](index.html)

A build-first computer graphics course for a computer science student who already programs. Start with the math you need, make pixels on the CPU, then move the same ideas into shaders and a small real-time renderer.

## Start here

1. Open [`index.html`](index.html) in a modern browser. It works offline and has an English / Traditional Chinese switch.
2. Start with **01 — Draw a triangle**. The demo implements a CPU framebuffer, edge tests, and barycentric color interpolation.
3. Change the vertex controls, read the experiment prompts, then implement the listed variations in `labs/01-triangle/app.js`.
4. When you can answer a lab's checkpoints, pick the next lab from the lab bar. Each lab folder has its own README with exercises and checkpoints.
5. Keep a `NOTES.md` of observations, screenshots, and your answers to each checkpoint.

No graphics card setup, engine, package manager, or prior graphics course is required for the first labs. The course introduces WebGL 2 after the software renderer so that the CPU work gives GPU concepts context. The browser page is a learning aid, not a substitute for implementing each exercise.

## Course route

| Stage | Build | Core ideas |
| --- | --- | --- |
| 0. Pixels | Plot pixels and draw a triangle on a CPU framebuffer | Coordinates, sampling, rasterization, barycentric coordinates |
| 1. 2D transforms | Move, rotate, and scale shapes | Vectors, matrices, composition, homogeneous coordinates |
| 2. 3D transforms | Render a rotating mesh with a camera | Model / view / projection, clip space, perspective divide |
| 3. Visibility | Add a depth buffer and back-face culling | Depth testing, winding, clipping, precision |
| 4. GPU pipeline | Port the mesh to WebGL 2 and write shaders | Vertex processing, interpolation, fragments, buffers, uniforms |
| 5. Materials and light | Add normals, Lambert / Blinn-Phong, textures | Coordinate spaces, sampling, color spaces, BRDF intuition |
| 6. Shadows and quality | Add a shadow map and anti-aliasing experiments | Multiple passes, bias, aliasing, trade-offs |
| 7. Final project | Build a compact interactive scene and explain its pipeline | Profiling, debugging, design choices, reproducible demos |

The detailed milestone-by-milestone build plan is in [`SYLLABUS.md`](SYLLABUS.md). Labs 01–06 are implemented in this directory; later milestones are the planned course sequence to build as you progress.

**Part II (draft, awaiting review):** [`SYLLABUS-PART2.md`](SYLLABUS-PART2.md) outlines what comes after the browser: native Vulkan or Direct3D 12 on the RTX 5070 Ti, a shared core, and two tracks, a game-engine renderer or 2D, text and compositing rendering. It is a proposal only; none of it is implemented. Parts I and II can both be completed on Windows.

Hardware is part of the course too: a thread woven through the labs covers pipeline stages, GPU parallel execution, memory and CPU/GPU data movement, and evidence-based performance diagnosis. The browser labs provide a low-friction start. The NVIDIA path adds a native OpenGL renderer and Nsight Graphics frame analysis on your RTX 5070 Ti; CUDA image processing and Vulkan ray tracing are optional extensions. It stays at the level useful for graphics programming; circuit design is out of scope.

The RTX 5070 Ti supports Vulkan 1.4 and OpenGL 4.6 according to NVIDIA's [GeForce RTX 5070 family specifications](https://www.nvidia.com/en-us/geforce/graphics-cards/50-series/rtx-5070-family/). NVIDIA lists RTX 50-series cards and OpenGL / Vulkan among [Nsight Graphics' supported hardware and APIs](https://developer.nvidia.com/nsight-graphics/get-started); consult the current page for installation requirements.

Suggested pace: 2–4 hours per stage. Spend most of each session changing code and inspecting output; use the concept notes to explain what you observe.

Conventions such as handedness, matrix storage, the NDC y direction, the depth range, front-face winding, and where color math happens are listed once in [`CONVENTIONS.md`](CONVENTIONS.md), with the changes each one needs in Vulkan and Direct3D 12.

## Working loop

For every lab: **predict → implement → inspect → explain**. Change one thing at a time. Record a concrete observation (for example, which side of a triangle is included at a shared edge) and connect it to the rule in your implementation. Do not move on until you can explain the pixels, not just run the demo.

## Files

- [`index.html`](index.html) — bilingual course overview and interactive labs; choose a lab from the lab bar. It works when opened directly from disk.
- [`CONVENTIONS.md`](CONVENTIONS.md) — the course's coordinate, matrix, depth, winding, and color conventions, and how each changes in Vulkan and Direct3D 12.
- [`labs/common/math.js`](labs/common/math.js), [`labs/common/color.js`](labs/common/color.js) — shared vector/matrix/camera/projection helpers and sRGB conversions for Labs 02–06, loaded as classic scripts on `window.cg` so no server is needed.
- [`SYLLABUS-PART2.md`](SYLLABUS-PART2.md) — **draft** Part II plan (native GPU rendering, engine track, 2D/text/compositing track), awaiting review.
- [`labs/01-triangle/app.js`](labs/01-triangle/app.js) — deliberately readable CPU triangle rasterizer to extend.
- [`labs/01-triangle/README.md`](labs/01-triangle/README.md) — experiment brief and implementation checkpoints.
- [`labs/02-transforms/app.js`](labs/02-transforms/app.js) — 3×3 homogeneous transforms, a viewport matrix, and a wireframe debug overlay on top of the Lab 01 rasterizer.
- [`labs/02-transforms/README.md`](labs/02-transforms/README.md) — transform-order and color-mixing experiments, exercises A–H, and checkpoints.
- [`labs/03-first-3d/app.js`](labs/03-first-3d/app.js) — 4×4 matrices, an orbit camera, perspective and orthographic projection, a vertex trace, and an observer view, still on the CPU.
- [`labs/03-first-3d/README.md`](labs/03-first-3d/README.md) — coordinate conventions, exercises A–G, and checkpoints.
- [`labs/04-visibility/app.js`](labs/04-visibility/app.js) — near-plane clipping, back-face culling, a depth buffer with selectable precision, painter's sort for comparison, and depth / overdraw debug views.
- [`labs/04-visibility/README.md`](labs/04-visibility/README.md) — visibility scenes, exercises A–G, and checkpoints.
- [`labs/05-webgl/app.js`](labs/05-webgl/app.js) — the same cube in WebGL 2 (buffers, a vertex array, shaders, uniforms), a CPU reference with JavaScript copies of the shaders, a pixel-difference view, and a live fragment-shader editor.
- [`labs/05-webgl/README.md`](labs/05-webgl/README.md) — CPU/GPU comparison, exercises A–H (including the optional native OpenGL / Nsight path), and checkpoints.
- [`labs/06-materials/app.js`](labs/06-materials/app.js) — normals and the normal matrix, Lambert and Blinn–Phong lighting per vertex or per fragment, texture filtering with mipmaps, and linear-light versus sRGB color math in WebGL 2.
- [`labs/06-materials/README.md`](labs/06-materials/README.md) — lighting and color-space experiments, exercises A–H, and checkpoints.

## Completion target

Finish with a renderer you wrote yourself, an interactive final scene, and a short technical note that traces one vertex from object space through rasterization to a displayed pixel. You should be able to justify your coordinate conventions, depth behavior, lighting model, and at least one quality/performance trade-off.
