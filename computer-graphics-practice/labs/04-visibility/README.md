# Lab 04 — Decide what is visible

## Goal

Fix the problems Lab 03 left on purpose: back faces painted over front faces, triangles that vanish whole at the near plane, and no answer to "which surface is in front at this pixel?". Add three tools, each with an off switch, and be able to explain what each one fixes and what it costs.

```
clip space --(near-plane clipping)--> ÷ w --(back-face culling)--> rasterize --(depth test)--> pixel
```

## Run

Open `../../index.html` and choose **04 · Visibility**. Pick a scene, then switch Visibility, Culling, Near plane, and Depth format one at a time. Hover the canvas to read the depth buffer. The page works without a server or network connection.

## Conventions

This lab relies on the course conventions in [`../../CONVENTIONS.md`](../../CONVENTIONS.md): the OpenGL / WebGL clip volume, window depth `d = (z_ndc + 1) / 2` with the near plane at 0, a depth buffer cleared to 1 with a "less than" test, and counter-clockwise front faces measured in NDC with +y up. Vulkan and Direct3D 12 clip depth to `0 ≤ z ≤ w`, and Vulkan's NDC +y points down. Both change these tests, so read the notes there before you port this lab. The faces have flat colors, so no color mixing happens here. Shared math comes from `../common/math.js`.

## Scenes

- **Cube + slicing plane** — a single-sided square cuts diagonally through the spinning cube. No order of whole triangles draws this correctly, because the square is in front of some cube faces at some pixels and behind them at others.
- **Z-fighting pair** — a large square with a smaller one floating `Separation` units above it. Use it for depth-precision experiments.
- **Floor through the camera** — four huge floor tiles (two triangles each) that reach under and behind the camera, plus the cube. Use it for near-plane experiments; move the camera inside the cube for more.

## Read the implementation

1. **Clipping, before the divide.** `nearStage` tests each vertex against the near plane in clip space, `z + w ≥ 0`. A triangle that crosses the plane goes through `clipPolygon`, a Sutherland–Hodgman clipper that keeps inside vertices and inserts an intersection point on each crossing edge. A clipped triangle becomes a polygon of 3 or 4 vertices, which is split into a triangle fan. The "Skip the whole triangle" mode drops any triangle that crosses, as Lab 03 did. The "None" mode divides by `w` anyway.
2. **The divide and culling.** `toScreen` divides by `w` and maps to pixels and to window depth `d = (z_ndc + 1) / 2`. `ndcArea` computes the signed area in NDC, where +y is up. Counter-clockwise (positive) is front-facing, matching the cube's winding from Lab 03 and OpenGL's default.
3. **Visibility.** "None" rasterizes triangles in submission order. "Painter's sort" sorts all surviving triangles by their average distance (`w`) and draws far to near. "Depth buffer" keeps one stored depth per pixel, cleared to 1, and writes a sample only if its depth is less than the stored value.
4. **Depth interpolation.** Window depth is interpolated with the same screen-space barycentric weights as Lab 01. That is correct for NDC z, which is an affine function of screen x and y on a planar triangle. It would not be correct for view-space z.
5. **Depth formats.** `quantize` rounds each depth to the chosen format before testing and storing it: 32-bit float, or 24, 16, or 12-bit fixed point.
6. **Debug views.** "Stored depth" shows the raw buffer as gray. "Depth as view distance" converts it back to distance with `viewDistance(d) = n·f / (f − d·(f − n))` and stretches the visible range. "Overdraw" counts covered samples per pixel: dark blue 1, teal 2, yellow 3, red 4 or more.
7. **The readouts** count triangles at each stage and samples at the depth test. "Depth under the mouse" shows the stored value, the distance it encodes, and the distance between two neighbouring depth codes at that point.

Deliberately left for you, marked with `TODO(exercise …)` in `app.js`: the depth comparison is fixed to "less than", there is no reversed-Z option, only the near plane is clipped, and there is no depth bias.

## Exercises

- **A — Depth function:** make the comparison selectable: LESS, LEQUAL, GREATER, and ALWAYS. Predict each picture first. Which choices need a different clear value to make sense? In the Z-fighting pair at separation 0, why do LESS and LEQUAL pick different winners?
- **B — Reversed-Z:** add a toggle that stores `1 − d`, clears to 0, and passes the test when the new value is greater. Compare the 32-bit float format with and without it at Near = 0.01 in the Z-fighting pair. Why does this help floats but not fixed-point formats? (Look at where float32 has the most representable values.)
- **C — More clip planes:** extend `nearStage` to clip against the far plane `w − z ≥ 0` as well. Which part of the picture changes, and why did the depth test already hide most of it? Then add the four side planes `w ± x ≥ 0` and `w ± y ≥ 0`. They do not change the image. What do they change? Measure it with the frame time and the samples-covered readout, and look up "guard band".
- **D — Defeat painter's algorithm:** build a scene of three long, thin triangles that overlap in a cycle (A over B, B over C, C over A). Show that no draw order is correct, whatever sort key you choose. Show that the depth buffer handles it.
- **E — Cull before the divide:** move the culling test before `nearStage`, using the clip-space vertices: the sign of the determinant of the 3×3 matrix with rows `(x, y, w)` of the three vertices tells you the winding. Why is this test still correct when a vertex has `w < 0`, while the NDC area test is not? Use the "None" near-plane mode as evidence.
- **F — Polygon offset:** add a depth bias for the top square only: `d − (constant · one depth code + slope · max(|∂d/∂x|, |∂d/∂y|))`. Find the smallest constant that removes the stripes in 16-bit, then tilt the camera closer to the plane. Why is a slope term needed? Lab 07 needs the same idea for shadow acne.
- **G — Early depth rejection:** with the depth buffer on, sort triangles front to back instead of back to front. Compare samples covered, failed, and written. Why do GPUs try to run the depth test before the fragment shader whenever they can?

## Checkpoints

Answer these in your lab notebook:

1. Using a sketch of the slicing plane, explain why sorting whole triangles fails while a per-sample depth test succeeds. What extra information does the depth buffer keep that a sort key cannot?
2. Why is window depth safe to interpolate linearly in screen space, while view-space z is not?
3. Why must near-plane clipping happen before the divide by `w`? Describe what "None" draws and why.
4. Where does a perspective projection spend its depth precision? Use the depth-step readout at two distances, and explain why raising Near helps much more than lowering Far.
5. Describe one setup that still z-fights with a 24-bit or float depth buffer, and two different fixes.
6. Back-face culling depends on winding. Which convention from Lab 03 does it rely on? What happens to a mirrored object (negative determinant in the model matrix, as in Lab 02), and how do real renderers handle it?

## Where this shows up later

- **Engines:** depth pre-passes, hierarchical-Z occlusion culling, reversed-Z depth buffers, and clipping all extend this lab. GPU-driven culling in Part II tests whole objects before any triangle reaches the rasterizer.
- **Compositors:** windows are drawn back to front like painter's sort, because transparent layers must blend in order. Fully covered opaque windows are skipped (occlusion culling), and only changed regions are redrawn (damage tracking).

## Next

Lab 05 moves the same cube, the same matrices, and the same depth and culling rules to the GPU with WebGL 2, and compares the GPU's pixels with this CPU renderer.
