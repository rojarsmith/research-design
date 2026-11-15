# Lab 02 — 2D transforms and wireframe debugging

## Goal

Move shapes with matrices instead of editing vertex coordinates, and keep three coordinate spaces apart: **object** space (where the shape is authored), **world** space (where the scene lives), and **pixel** space (where the rasterizer works). The rasterizer from Lab 01 does not change. Everything new happens before it.

```
object --(model matrix M)--> world --(viewport matrix V)--> pixel --(Lab 01 rasterizer)--> samples
```

## Run

Open `../../index.html` and choose **02 · 2D transforms**. Move the sliders, change the multiplication order, and toggle the debug layers. The page works without a server or network connection.

## Read the implementation

1. Points are homogeneous column vectors `[x, y, 1]`. Directions are `[x, y, 0]`. `apply(m, p)` computes `M · p`.
2. Matrices are stored as rows (`m[row][col]`) so the code matches the math on paper. `translate`, `rotate`, and `scale` each build one 3×3 matrix.
3. `modelMatrix()` multiplies the three matrices in the order chosen on the page. With column vectors, `M = T · R · S` applies **S first**, then R, then T. The "Applied to a point" readout spells this out.
4. `viewport` maps world units to pixels: 40 pixels per unit, origin at the canvas center, and a **negative** y scale because pixel rows grow downward while world +y points up.
5. `draw()` transforms every vertex to world space, then to pixel space, and calls the unchanged `fillTriangle`. The debug overlay is drawn into the same CPU framebuffer:
   - **Wireframe** — every triangle edge, including the diagonals that split each rectangle.
   - **Vertices** — a marker on every transformed vertex.
   - **Local axes** — the images of the object's x axis (red) and y axis (green), drawn from the object's origin (orange). The axes are directions (`w = 0`), so translation moves their start point but does not change their direction.
   - **Object-space ghost** — the same F drawn with `M = identity`, so you can see where it started.
   - **World grid** — one-unit grid lines and the world axes.
6. The readout shows the composed matrix, its determinant, and the F's total signed area in each space. Signed area is positive for counter-clockwise vertex order in a +y-up frame.

Two things are deliberately not done for you. `invert()` returns `null`, so the mouse readout shows `TODO (C)` for object coordinates. `drawLine()` uses a floating-point DDA rather than Bresenham's algorithm.

## Exercises

- **A — Order on paper:** for the F's top-right corner `(2, 3)`, compute `T(3, 0) · R(90°) · p` and `R(90°) · T(3, 0) · p` by hand. Set the same values on the page (Translate Y = 0, Scale = 1) and check the white vertex markers. Explain the difference in one sentence about which space each rotation happens in.
- **B — Rotate about a pivot:** write `rotateAbout(radians, px, py)` that returns `T(p) · R · T(−p)`. Use it to spin the F around its own center `(1, 1.5)` instead of its corner. Which local-axes overlay point stays fixed now?
- **C — Inverse:** implement `invert(m)` for affine matrices (invert the 2×2 linear part, then fix the translation). The mouse readout will start showing object coordinates. Verify that `multiply(m, invert(m))` is the identity within rounding, and that `invert(viewport)` matches the hand-written `pixelToWorld`. Then highlight the triangle under the mouse by testing the object-space point against the untransformed triangles with the edge function.
- **D — Bresenham:** replace the DDA in `drawLine` with an integer-only Bresenham line. Count how many pixels differ from the DDA output for the wireframe. Which line slopes produce differences, and why?
- **E — Row vectors:** rewrite `apply` and the matrix builders for the row-vector convention `p' = p · M`. What happens to each matrix, and how must the multiplication order change so the picture stays identical? Keep a note of which convention each later API uses.
- **F — Winding test:** skip triangles whose pixel-space signed area is positive. Which slider settings make the F disappear? Explain using the signs of `det(M)` and `det(V)`. This is the 2D version of back-face culling in Lab 04.
- **G — Parent and child:** add a small flag triangle to the top bar of the F. Give it its own matrix `C` and draw it with `M · C`, so it follows the F when you move the sliders but can also rotate on its own. This is a two-node scene graph.

## Checkpoints

Answer these in your lab notebook:

1. Why does adding a third coordinate equal to 1 let translation become a matrix product? What does `w = 0` mean, and where does this code rely on it?
2. With column vectors, which matrix in `T · R · S` touches the point first? How does the answer change with row vectors?
3. Why does a negative determinant flip the signed area? Why does the viewport matrix have a negative determinant here, and why does the F still look correct on screen?
4. The barycentric weights `fillTriangle` computes in pixel space are the same weights the sample has in object space. Which property of affine transforms makes this true? (Lab 03's perspective divide breaks it.)
5. If the canvas changed to 960 × 648 pixels, which values in this program would change and which would not? Answer in terms of object, world, and pixel space.

## Next

Lab 03 replaces 3×3 matrices with 4×4 matrices, adds a camera and a perspective projection, and keeps the same rule: one named space per stage, one matrix per arrow.
