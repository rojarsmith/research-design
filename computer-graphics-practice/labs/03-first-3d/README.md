# Lab 03 — The first 3D image

## Goal

Render a spinning cube through a camera with a perspective projection, still on the CPU. The only new ideas are 4×4 matrices, a camera, a projection, and the perspective divide. Be able to follow one vertex through every coordinate space and explain each number.

```
object --M--> world --V--> view --P--> clip --(÷ w)--> NDC --(viewport)--> pixel
```

## Run

Open `../../index.html` and choose **03 · 3D camera**. Drag the canvas or use the sliders to orbit the camera. Switch the projection, the draw mode, and the view, and pick a vertex to trace. The page works without a server or network connection.

## Conventions used in this code

Every renderer has to choose these. The course-wide list, with how each item changes in Vulkan and Direct3D 12, is in [`../../CONVENTIONS.md`](../../CONVENTIONS.md). Copy the items below into your notes and keep them current as you change the code.

- **Column vectors:** `p' = M · p`, so `P · V · M · p` applies M first.
- **Column-major storage:** a matrix is a flat array of 16 numbers, `m[col * 4 + row]`. `fromRows` lets you *write* a matrix row by row and stores it column-major. WebGL expects this layout, so Lab 05 uploads these arrays unchanged.
- **Shared math:** the vector and matrix helpers live in [`../common/math.js`](../common/math.js), a classic script that puts them on `window.cg.math` (ES modules would not load when `index.html` is opened from disk). Labs 03–07 share it, so a change there affects all of them; copy a function into this lab's `app.js` if you want to experiment locally.
- **Right-handed world, +y up.** The camera looks down its own **−z** axis.
- **OpenGL / WebGL clip volume:** a point is inside when `−w ≤ x, y, z ≤ w`. After dividing by `w`, NDC is the cube `[−1, 1]³`. (Direct3D, Metal, and Vulkan use `0 ≤ z ≤ w` instead.)
- **Viewport:** NDC x maps to `[0, 480]`; NDC y is flipped into pixel rows, as in Lab 02.

## Read the implementation

1. In `../common/math.js`, `cameraBasis(eye, target, up)` builds the camera's `right`, `up`, and `forward` axes and `viewMatrix(basis)` builds the view matrix; `mainCamera()` in this lab calls both. Its first three rows are the camera's right, up, and back axes in world coordinates; its last column moves the eye to the origin. The view matrix is the inverse of "place the camera in the world".
2. `perspective(fovY, aspect, near, far)` (also in `../common/math.js`) maps the frustum into the clip volume. Its bottom row `[0, 0, −1, 0]` copies `−z_view`, the distance in front of the camera, into `w`.
3. `orthographic(...)` maps a box instead. Its bottom row is `[0, 0, 0, 1]`, so `w = 1` and the divide changes nothing. The box is sized so the orbit target appears the same size as with perspective.
4. `drawScene` multiplies `P · V · M` once, transforms the 8 shared cube positions, divides by `w`, maps to pixels, and then fills or outlines the 12 indexed triangles.
5. The vertex-trace table recomputes one vertex step by step, using the same matrices, and checks the clip-volume test for each axis.
6. The observer view renders the scene from a second camera, beside and above the first. It draws the main camera's eye and its axes: red = right (+x), green = up (+y), blue = back (+z). The camera looks the opposite way from the blue arrow.

What is deliberately missing, because later labs or your exercises add it:

- **Visibility.** There is no depth buffer and no back-face culling, so filled triangles are drawn in array order. In Filled mode you will see back faces painted over front faces. Lab 04 fixes this.
- **Clipping.** A triangle with any vertex in front of the near plane is skipped whole. The far plane is not tested at all. Lab 04 clips triangles properly. (The grid and debug lines are cut at the near plane by `drawSegment3D` so they do not explode when a line passes behind the camera.)
- **The frustum.** `frustumCorners()` returns an empty list, so the observer view shows the camera but not its view volume. That is exercise D.

## Exercises

- **A — Trace by hand:** turn Spin off and press Reset, so the model matrix is the identity. Pick vertex 6 `(1, 1, 1)` and compute its view, clip, NDC, and pixel coordinates with a calculator, using the V and P readouts. Match every row of the trace table. Which single number turns clip into NDC?
- **B — Painter's algorithm:** in `drawScene`, sort `order` so the triangle farthest from the camera is filled first (use the average view-space z of its three vertices). Does the spinning cube look right in Filled mode now? Keep this code: Lab 04 shows a scene where it fails.
- **C — Viewport as a matrix:** replace `ndcToPixel` with a 4×4 viewport matrix, as in Lab 02, and print `viewport · P · V · M`. Why can you fold the viewport into one matrix with the others, but not the divide by `w`?
- **D — Draw the frustum:** implement `frustumCorners(camera)`. At distance `d` along `forward`, the half-height is `d · tan(fovY / 2)` and the half-width is the half-height times `aspect`. The observer view draws the 12 edges as soon as the function returns 8 points. Then make it correct for the orthographic projection too, where the volume is a box. Bonus: compute the same corners by transforming the 8 NDC cube corners with the inverse of `P · V`, and compare.
- **E — Field of view versus distance:** compare FOV 25° at distance 12 with FOV 80° at distance 3.2. The cube is about the same size; what changes? Implement a "dolly zoom" toggle that changes FOV and keeps `distance · tan(fovY / 2)` constant while the camera moves.
- **F — A second mesh:** add an octahedron or a square pyramid as a position list and an index list, plus a select to switch meshes. Check that every face is counter-clockwise when seen from outside. Which face would you get wrong first?
- **G — Parent and child in 3D:** add a small moon cube that orbits the main cube: `M_moon = M_cube · rotationY(t) · translation(3, 0, 0) · scale(0.3)`. `translation` and `scaling` already exist in `window.cg.math`; read how they are written with `fromRows` before you use them.

## Checkpoints

Answer these in your lab notebook:

1. Write the chain of spaces and matrices from object to pixel, and state every convention this code uses (vector convention, storage order, handedness, camera direction, clip volume).
2. Why does `w_clip` equal the distance in front of the camera for this perspective projection? Use the trace table to explain why farther points land closer to the center of the screen. What happens instead with the orthographic projection?
3. What do the near and far values change in this lab, and what do they not change yet? Use experiment 03 / C as evidence.
4. Why is the view matrix the inverse of the camera's placement in the world? Why are its first three rows the camera's axes?
5. In Lab 02, barycentric weights in pixel space matched object space. Why is that no longer true after the perspective divide? (You will see the consequence in Lab 05.)

## Where this shows up later

- **Engines:** the same object → world → view → clip chain runs in every vertex shader. Production engines usually switch to a [0, 1] depth range and reversed-Z (Lab 04, Part II), and Vulkan flips NDC y; see `CONVENTIONS.md`.
- **Compositors:** a 2D compositor uses the orthographic case: one projection that maps pixel rectangles straight to NDC, with no perspective divide doing any work.

## Next

Lab 04 adds the missing visibility rules: a depth buffer, back-face culling based on the winding you just set up, and clipping against the near plane.
