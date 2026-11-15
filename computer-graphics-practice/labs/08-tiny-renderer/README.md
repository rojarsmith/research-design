# Lab 08 — Final project: a tiny renderer demo

**English** | [繁體中文](README.zh-TW.md)

## Goal

Finish Part I with a small renderer that you can run, explain and defend. The brief: a navigable scene with at least two meshes, a movable camera, depth, one textured lit material, shadows or a documented substitute, and a feature toggle or debug view. You hand in the source, screenshots or a short recording, and a 1–2 page design note.

This lab gives you three things: the brief with a checklist, a reference demo that meets it on the page, and an outline for the design note. The demo is a known-good comparison, not the answer: your project is code you wrote and can explain.

```
CPU, once        meshes, textures -> GPU buffers           shader sources -> programs
CPU, per frame   keys and drag -> camera    time -> model matrices    sliders -> light matrices
pass 1 shadow    6 draws from the light  -> depth texture (the shadow map)
pass 2 scene     6 draws from the camera -> MSAA color + depth: textured, lit, shadowed
pass 3 resolve   4 samples -> 1 per pixel (blitFramebuffer)
pass 4 present   scaled image -> canvas: bilinear in linear light, sRGB encode once
```

## Run

Open `../../index.html` and choose **08 · Final demo**. Click the canvas so it has keyboard focus, then use W A S D to move, Q and E to go down and up, and Shift to move faster; drag to look around. "Show" switches between the final image and a debug view for each stage. The readouts list the passes of the current frame, trace one vertex through every space, and show draw calls, memory, and GPU time per pass when the browser exposes timer queries. The page needs WebGL 2 but no server or network connection.

## The reference demo against the brief

| Requirement | In the demo | Where in `app.js` |
| --- | --- | --- |
| A navigable scene with at least two meshes | A tiled ground, two crates, a sphere, a pillar and a spinning cube: four meshes, six objects | `MESHES`, `SCENE` |
| A movable camera | A fly camera: W A S D, Q / E, Shift, drag to look | `camera`, `cameraAxes`, `updateCamera` |
| Depth | A depth test in both passes, and a view that turns window depth back into view distance | `shadowPass`, `scenePass`, `uView == 4` |
| One textured, lit material | Tile and crate textures (`SRGB8_ALPHA8`, mipmapped), lit with Lambert and Blinn–Phong in linear light | `makeTileTexture`, `makeCrateTexture`, `SCENE_FRAGMENT_SHADER` |
| Shadows or a documented substitute | A shadow map with 3 × 3 bilinear PCF and slope-scaled bias | `lightFrame`, `shadowPass`, `shadowFactor` |
| A feature toggle or debug view | Five toggles, seven views, a render scale and a shadow-map size | the controls under the canvas |

## Read the implementation

`app.js` is laid out in the order a frame needs it:

1. **Settings** are read from the page once per frame (`readSettings`), so every pass sees the same values.
2. **The scene is data.** `MESHES` builds geometry with `../common/mesh.js`. `MATERIALS` holds colors (sRGB, decoded when uploaded), texture names, UV scale and specular parameters. `SCENE` pairs a mesh with a material and a model matrix that may depend on time. Adding an object means adding one line.
3. **The camera** is a position, a yaw and a pitch. `cameraAxes` turns yaw and pitch into a forward and a right vector, and the view matrix is `lookAt(eye, eye + forward, up)`. Keys count only while the canvas has focus, so the page still scrolls normally.
4. **GPU resources** are created once in `setup` through `../common/webgl.js`: four programs, four meshes, two textures, the shadow map, a raw-depth sampler and the GPU timer. `sceneTargets` rebuilds the scene's render targets only when the render scale or MSAA changes.
5. **Passes.** `shadowPass` and `scenePass` each draw every object. `webgl.resolve` averages the MSAA samples. `presentPass` filters the scene image to the canvas size in linear light and encodes it once. At render scale 2× the bilinear sample point sits exactly between four texels, so it is a 2 × 2 average (SSAA); at 0.5× it is an upscale.
6. **The frame.** `buildFrame` gathers what the passes need: settings, matrices, the light, and every object's model matrix at this moment. `render` runs the passes inside GPU timer queries and then fills in the readouts. `showTrace` repeats the vertex shader's math on the CPU for one corner of the spinning cube, which gives you numbers for the design note.

Every idea here is one you have met: the normal matrix and Blinn–Phong from Lab 06; the shadow lookup, bias, render targets, MSAA and the linear-light resolve from Lab 07; and the vertex trace from Lab 03.

## Conventions

Unchanged from [`../../CONVENTIONS.md`](../../CONVENTIONS.md): a right-handed world with +y up, column vectors and column-major storage, NDC +y up, the clip volume −w ≤ z ≤ w with window depth (z_ndc + 1) / 2, a depth test that keeps the smaller value, counter-clockwise front faces with back faces culled in both passes, linear-light shading with sRGB textures decoded before filtering, and Lab 07's shadow-map lookup. The pixel row in the trace counts from the top, as in the CPU labs; WebGL's window y counts from the bottom.

## Your project

Choose one path and say which one in your design note:

- **Extend this demo.** Keep its structure and complete at least three of the exercises below; at least one of them must add or change a pass or a render target.
- **Build from your own labs.** Start from your Lab 06 or Lab 07 code and meet the same brief. Use this demo only to compare images and timings.

Either way, everything you hand in must be code you can explain line by line, and it must run from a page with no build step, like the labs.

### Deliverables

1. **Source code** in your own folder, runnable by opening one HTML page.
2. **Screenshots or a 30–90 s screen recording:** the final image, every debug view you added, one artifact before and after your fix, and one quality comparison at the same zoom.
3. **A design note of 1–2 pages** that follows the outline below.

### Design-note outline

1. **Pipeline diagram.** Every pass with its inputs, its target (size, format, sample count), its state and its shaders. Mark which stages are programmable and which are fixed-function.
2. **Conventions.** Handedness, matrix storage and multiplication order, NDC y, depth range and test, front faces and culling, and where color is decoded and encoded. Note every place where you differ from `CONVENTIONS.md`.
3. **One vertex, traced.** Follow one vertex from object space through world, view, clip, NDC and window coordinates to the pixel it lands on and the value displayed there, with numbers. Add its shadow-map coordinates if you have shadows.
4. **One bug you diagnosed.** The symptom, your first hypothesis, the debug view or measurement that confirmed or refuted it, and the fix.
5. **One quality/performance trade-off.** The two options, how you measured (GPU timer queries, or a controlled frame-time comparison), the numbers, and your choice.
6. **Limits and next steps.** What your renderer cannot do yet, and what you would change first.

## Exercises

Extensions to choose from. Each names the lab it builds on.

- **A — Spot light:** replace the directional light with a spot light: a position, a cone, `perspective()` for the shadow pass, the divide by w in the lookup, and distance falloff (Lab 07 E, Lab 06 A).
- **B — Normal mapping:** add a tangent attribute to the crate, draw a normal map on a canvas, and perturb the normal in tangent space (Lab 06 G).
- **C — A better material:** add Schlick Fresnel and energy-normalized specular (Lab 06 B and C), or a metallic–roughness model as a preview of Part II stage E3.
- **D — Frustum culling:** give every object a bounding sphere, skip draws outside the camera's view frustum, and show how many were skipped. Measure JavaScript and GPU time with 200 objects.
- **E — Instancing:** draw 500 crates with `drawElementsInstanced` and a per-instance model matrix, and compare JavaScript time and GPU time with 500 separate draws.
- **F — Stable shadows:** fit the light box to the camera's view and snap it to whole texels, or split it into two cascades (Lab 07 C).
- **G — HDR and tone mapping:** render into an `RGBA16F` target (it needs the `EXT_color_buffer_float` extension), raise the light intensity above 1, and tone-map in the present pass before the sRGB encode. Compare with plain clamping.
- **H — Picking:** render object IDs into an integer target, read the pixel under the mouse with `readPixels`, and show that object's name, model matrix and material. Why is this readback acceptable on a click but not every frame?

## Self-review

Before you hand it in, check that:

- A classmate can open the page, move the camera and change a control within a minute, without reading your source.
- Every pass, target and shader in your pipeline diagram exists in the code, and the code has none that the diagram lacks.
- Each debug view says what it shows, and at least one of them helped you find a bug.
- The port checklist at the end of `CONVENTIONS.md` passes for your scene.
- Your trade-off claim rests on a measurement that someone else could repeat with your settings.
- The browser console shows no errors, and `gl.getError()` reported none while you developed.

## Checkpoints

Answer these in your lab notebook:

1. Using the trace readout, follow the spinning cube's corner from object space to a canvas pixel and into the shadow map. Which steps are matrix products, which one is a division, and which steps does fixed-function hardware perform when the GPU processes the same vertex?
2. List the demo's passes with their targets, sizes and sample counts, and say what reads each target afterward. If your GPU budget were halved, which pass would you cut or shrink first, and what would the image lose?
3. Which debug view would you open first to find (a) a wrong normal matrix, (b) a texture uploaded upside down, (c) shadow acne, (d) a near plane set much too close? Break each on purpose and check your answer.
4. With render scale 0.5×, 1× and 2× and MSAA on and off, which combination gives the best image per millisecond on your GPU? Show the measurements.
5. Name one thing your own renderer does differently from this demo, and justify it.

## Where this shows up later

- **Engines:** Part II starts again from a native skeleton (C0), and this design note grows into the engine capstone's frame-time budget report: per-pass GPU times at a stated resolution, backed by a capture.
- **Compositors:** a compositor runs the same loop of passes, render targets and one final encode for the display, over windows instead of meshes. The 2D, text and compositing track's capstone asks for a measurement report in the same spirit.

## Next

Part I ends here. The draft [`../../SYLLABUS-PART2.md`](../../SYLLABUS-PART2.md) continues with native Vulkan or Direct3D 12 on the RTX 5070 Ti. Start with the port checklist in `CONVENTIONS.md` and stage C0.
