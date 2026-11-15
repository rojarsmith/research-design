# Lab 05 — Hand geometry to the GPU

## Goal

Port the Lab 03–04 cube to WebGL 2. Upload positions and indices once, write a vertex shader and a fragment shader, animate one matrix uniform, and let the GPU do clipping, the divide by `w`, culling, rasterization, interpolation, and the depth test. Then compare the GPU's pixels with a CPU reference that uses the same buffers and the same matrices, and explain every difference.

```
setup, once:   compile shaders → link program → upload vertex + index buffers → describe attributes (VAO)
every frame:   set state → upload uniforms → drawElements → readPixels (only to compare)
```

## Run

Open `../../index.html` and choose **05 · WebGL 2**. The left canvas is WebGL 2; the right canvas shows the CPU reference, an amplified difference image, or a mask of mismatching pixels. The page works without a server or network connection, but it needs a browser with WebGL 2 and hardware acceleration enabled.

The "GPU and API reported by the browser" readout names the adapter that actually runs your shaders. On Windows, browsers usually translate WebGL to Direct3D 11 through ANGLE, and on a machine with both integrated and discrete graphics the browser may run on the integrated one. If the readout does not show your GeForce card, set the browser to "High performance" under Windows Settings → System → Display → Graphics, restart the browser, and check again. Record what you see in your notes.

## Read the implementation

1. **The mesh as the GPU sees it.** 24 vertices (4 per face, so each face has its own UVs) and 36 16-bit indices. Each vertex is 8 floats (32 bytes), interleaved as position, color, UV. The color is the position mapped from `[−1, 1]` to `[0, 1]`, so the cube shows the RGB color cube. The winding is Lab 03's: counter-clockwise from outside.
2. **Shaders.** `VERTEX_SHADER` multiplies each position by one `mat4` uniform and passes color and UV on as `out` variables ("varyings"). `FRAGMENT_SHADER` receives the interpolated values and outputs a vertex color, a checkerboard computed from the UV, or a gray level computed from `gl_FragCoord.z`.
3. **Setup, once.** `buildProgram` compiles, links, and looks up uniform locations; compile and link errors throw with the driver's log. `uploadMesh` creates a vertex array object (VAO), copies the vertex and index data into GPU buffers with `bufferData`, and describes the interleaved layout with `vertexAttribPointer(location, size, type, normalize, stride, offset)`.
4. **Every frame.** `gpuRender` sets the viewport, clear values, depth test, and culling state, uploads the matrix with `uniformMatrix4fv(location, false, data)`, and issues one `drawElements` call. `false` means "do not transpose": the column-major arrays from Lab 03 are already in the layout WebGL expects.
5. **Readback.** `readGpuImage` copies the GPU image back with `readPixels`, which makes the CPU wait for the GPU to finish, and flips the rows because WebGL's window origin is the bottom-left corner.
6. **The CPU reference.** `cpuVertexShader` and `cpuFragmentShader` are line-by-line JavaScript copies of the two shaders. `cpuRender` runs the vertex shader once per vertex, assembles triangles from the same index buffer, culls counter-clockwise-is-front exactly like `gl.frontFace(gl.CCW)`, and rasterizes with the Lab 04 depth test.
7. **The live editor** recompiles only the GPU fragment shader. If compilation fails, the log shows the error and the previous program keeps running.

8. **Color math.** The vertex colors are sRGB-encoded values. With "Color math → Linear light, sRGB output" (the default and the course rule), the vertex shader decodes them to linear light, the GPU interpolates linear values, and the fragment shader encodes once with `linearToSrgb`. "Interpolate sRGB numbers" skips both steps, as the original version of this lab did. The CPU reference mirrors both modes with `../common/color.js`. The depth view writes its gray directly: it is a measurement, not light.

The CPU reference deliberately interpolates varyings with plain screen-space barycentric weights, as Labs 01–04 did. The GPU interpolates perspective-correctly. That is the largest difference you will see, and fixing it is exercise B.

## Conventions

This lab relies on [`../../CONVENTIONS.md`](../../CONVENTIONS.md): column-major matrices uploaded with `transpose = false`, counter-clockwise front faces (`gl.frontFace(gl.CCW)`), NDC +y up, window origin at the bottom-left (hence the row flip after `readPixels`), and the clip volume `−w ≤ z ≤ w`. A Vulkan or Direct3D 12 port of this lab changes the depth range and the y direction, and has to choose its front-face setting explicitly. The matrix helpers come from `../common/math.js`.

## Exercises

- **A — Setup versus per frame:** list every `gl.*` call in `app.js` in the order it runs and mark each one "once" or "every frame". How many bytes cross from the CPU to the GPU each frame, and how many cross back for the comparison? Which of these would you remove in a real application?
- **B — Perspective-correct interpolation:** in `cpuRender`, interpolate `varying / w` and `1 / w` with the screen-space weights, then divide. With the checker shader and the difference view, the mismatches inside faces should disappear and leave at most a few pixels on cell or triangle edges. Explain why depth did not need this correction in Lab 04.
- **C — Vertex layout:** add a fourth attribute (a face normal, which Lab 06 needs) to the interleaved buffer: update the data, the stride, and the `vertexAttribPointer` calls, then visualize it in the fragment shader as a color. Alternatively, split the buffer into one buffer per attribute. Which layout keeps each vertex's data together in memory?
- **D — Move work between stages:** upload the model, view, and projection matrices separately and multiply them in the vertex shader. How many matrix products now run per frame, and where? When would you choose each option?
- **E — Flat versus smooth:** declare the color varying `flat` in both shaders. Which vertex of each triangle provides the value (look up "provoking vertex")? Implement the same rule in the CPU reference until the mismatch count returns to zero.
- **F — Coverage rule:** after exercise B, find mismatches on triangle edges, if any remain for some camera angle. Implement the top-left fill rule from Lab 01 exercise D in the CPU reference and check whether they disappear.
- **G — Antialiasing preview:** create the context with `antialias: true` and look at the difference mask along silhouette edges. Write down what changed and keep the screenshots for Lab 07.
- **H — NVIDIA native path (optional):** rebuild this lab as a small C++ program with OpenGL 4.6 (for example GLFW plus a loader such as glad). Reuse the same vertex data, indices, matrices, and shader bodies with `#version 460 core`. Confirm with `glGetString(GL_RENDERER)` that it runs on the RTX 5070 Ti, then capture one frame in Nsight Graphics and find the draw call, the vertex input layout (stride 32, offsets 0, 12, 24), the matrix uniform, and the depth and cull state. See the NVIDIA GPU path in `../../SYLLABUS.md`.

## Checkpoints

Answer these in your lab notebook:

1. Which calculations on this page run once per frame, which once per vertex, and which once per covered sample? Give approximate counts for the default view from the "Shader runs per frame" readout. Why is the vertex count a range?
2. Which data is uploaded once and which every frame? Why does keeping the mesh in a GPU buffer matter more as meshes grow?
3. Why can WebGL take the Lab 03 matrix arrays unchanged with `transpose = false`? What would you see if you passed `true`?
4. List every kind of difference between the CPU and GPU images and its cause: interpolation, coverage rule, rounding to 8 bits, depth format. Which ones would a real renderer care about?
5. Why does `readPixels` take measurable time, and why is timing `drawElements` with `performance.now()` not a measurement of GPU work? What would you use instead?

## Where this shows up later

- **Engines:** vertex layouts, uniform data, and pipeline state become Vulkan vertex input descriptions, descriptor sets or push constants, and pipeline objects. Readback is reserved for screenshots, GPU timing, and debugging, because it stalls the CPU.
- **Compositors:** each window is a textured quad drawn by exactly this kind of draw call. A compositor never reads frames back; it samples client buffers directly on the GPU.

## Next

Lab 06 adds normals, Lambert and Blinn–Phong lighting, and a sampled texture, and starts doing real work per fragment.
