# Conventions

**English** | [繁體中文](CONVENTIONS.zh-TW.md)

Every renderer has to choose a handful of conventions. Most bugs in a port from one graphics API to another come from one of them silently changing. This page is the single list for the course: what Part I (the CPU labs and WebGL 2) uses, which labs rely on each item, and what changes in Vulkan or Direct3D 12 in Part II. Each lab's README repeats the items it depends on.

## Summary

| Convention | Part I (CPU labs, WebGL 2) | Labs | Vulkan | Direct3D 12 |
| --- | --- | --- | --- | --- |
| Handedness | Right-handed world and view space, +y up; the camera looks down its own −z | 03–08 | Not fixed by the API; your matrices decide | Not fixed by the API; DirectXMath has both left- and right-handed helpers |
| Vectors | Column vectors, `p' = M · p`, so `P · V · M · p` applies M first | 02–08 | GLSL: same (`M * v`) | HLSL: `mul(M, v)` for column vectors; DirectXMath uses row vectors (`v · M`) |
| Matrix storage | Lab 02: 3×3 nested rows `m[row][col]`. Labs 03+: flat column-major `m[col * 4 + row]`, uploaded with `transpose = false` | 02–08 | GLSL defaults to column-major | HLSL constant buffers default to column-major packing; DirectXMath matrices are row-major in memory |
| NDC y | +y up | 03–08 | **+y down** | +y up |
| Framebuffer origin | CPU labs: pixel row 0 at the top. WebGL window coordinates (`gl_FragCoord`, `readPixels`): bottom-left | 01–08 | Top-left | Top-left |
| Clip-space depth | `−w ≤ z ≤ w`, NDC z in [−1, 1], window depth `d = (z_ndc + 1) / 2` | 03–08 | **`0 ≤ z ≤ w`**, NDC z in [0, 1] (an extension allows [−1, 1]) | **`0 ≤ z ≤ w`** |
| Depth test | Clear to 1, pass if less; reversed-Z is Lab 04 exercise B | 04–08 | Any compare op; reversed-Z is common | Same as Vulkan |
| Front face | Counter-clockwise in NDC with +y up; back faces culled | 01–02 (signed area), 04–08 | `frontFace` pipeline state, decided from framebuffer coordinates | `FrontCounterClockwise` is FALSE by default: **clockwise is front** |
| Pixel centers | Sample point of pixel (x, y) is (x + 0.5, y + 0.5) | 01–08 | Same | Same |
| Texture rows | Image row 0 is the top; in WebGL it becomes v = 0 unless you set `UNPACK_FLIP_Y_WEBGL` | 06, 08 | Row 0 in memory is v = 0 | Row 0 in memory is v = 0 |
| Shadow-map lookup | `(u, v, d) = 0.5 · ndc + 0.5` from the light's clip space, no y flip: rendering into a texture writes window row 0 to v = 0 | 07–08 | `d = z_ndc`; `v = 0.5 + 0.5 · y_ndc` without a y flip in the shadow pass, `0.5 − 0.5 · y_ndc` with a negative-height viewport | `d = z_ndc`; `v = 0.5 − 0.5 · y_ndc` |
| Color | 8-bit colors are sRGB-encoded; mix, light, filter, and blend in linear light; encode once on output | 02, 05–08 | `*_SRGB` formats decode on sampling and encode on writing; HDR via swapchain color spaces | `*_UNORM_SRGB` formats do the same; HDR via scRGB or HDR10 swap chains |
| Angles and units | Radians in code, degrees on sliders; world units are unspecified (think meters) | 02–08 | — | — |

## Details and Part II notes

### Handedness and the camera

The course uses a right-handed world with +y up. Cameras look down their own −z axis: `cameraBasis` in [`labs/common/math.js`](labs/common/math.js) returns `right`, `up` and `forward`, and the view matrix's third row is the camera's *back* axis, `−forward`. Neither Vulkan nor Direct3D fixes the handedness of world or view space; only the projection and the clip volume are API conventions. In Part II, keep the right-handed world and change only the projection and viewport (below). Much Direct3D sample code is left-handed because DirectXMath's `…LH` helpers were the long-time default. If you mix code from such samples, convert at one boundary and write down where.

### Vectors and storage

Column vectors mean `M · p`, read right to left: in `P · V · M` the model matrix acts first. Lab 02 stores 3×3 matrices as nested rows so the code reads like paper; from Lab 03 on, `labs/common/math.js` stores 4×4 matrices as flat column-major arrays (`m[col * 4 + row]`), and `fromRows` lets you write them row by row. WebGL's `uniformMatrix4fv(location, false, m)` reads that layout unchanged. GLSL for Vulkan also defaults to column-major matrices. In HLSL, constant buffers are packed column-major by default, but DirectXMath uses row vectors and row-major storage, so its matrices are usually transposed on upload or declared `row_major` in the shader. Pick one rule per codebase and test it with a non-symmetric matrix.

### NDC y and the framebuffer origin

In OpenGL and WebGL, NDC +y points up and window coordinates start at the bottom-left; the CPU labs map NDC +y up to pixel rows that grow downward (`ndcToPixel`). Vulkan's NDC +y points **down** and its framebuffer origin is the top-left. An unchanged OpenGL-style projection therefore renders upside down in Vulkan. Fix it in one place: either use a negative viewport height (allowed since Vulkan 1.1) or negate the y row of the projection. Direct3D 12 keeps NDC +y up and maps it to a render target whose origin is the top-left, so an OpenGL-style projection renders upright there.

### Clip-space depth

Part I uses the OpenGL / WebGL clip volume `−w ≤ z ≤ w`, so NDC z is in [−1, 1], and window depth is `d = (z_ndc + 1) / 2`. Vulkan and Direct3D 12 clip to `0 ≤ z ≤ w`, so NDC z is already the stored depth in [0, 1]. A Part I `perspective()` matrix used unchanged there clips away everything with a negative NDC z, which in practice is everything closer than about twice the near distance. Build a [0, 1] projection instead: its third row becomes `[0, 0, far / (near − far), far · near / (near − far)]` for the same right-handed view space. Vulkan's `VK_EXT_depth_clip_control` extension can also opt a pipeline into the [−1, 1] range. Part II uses [0, 1] and reversed-Z (near stores 1, far stores 0, test "greater") with a float depth buffer, which spreads precision far more evenly than Lab 04's mapping.

### Front faces and winding

Part I meshes are counter-clockwise when seen from outside, and front faces are counter-clockwise in NDC with +y up (`gl.frontFace(gl.CCW)`, `ndcArea` in Lab 04). Vulkan makes the front face a pipeline setting and decides orientation from framebuffer coordinates. When you flip y as described above so the image is upright, a triangle that looks counter-clockwise on screen is still counter-clockwise, so keep `VK_FRONT_FACE_COUNTER_CLOCKWISE`. Without the flip, the image and every winding are mirrored, and culling removes the faces you wanted. Direct3D 12 treats **clockwise** triangles as front-facing unless `FrontCounterClockwise` is TRUE, so set it to TRUE for this course's meshes.

### Pixel centers and texture rows

All three APIs sample pixel (x, y) at its center (x + 0.5, y + 0.5), as Lab 01 does. Image files and canvases store their top row first. WebGL uploads that first row as texture row v = 0 unless `UNPACK_FLIP_Y_WEBGL` is set (Lab 06 exercise F); Vulkan and Direct3D 12 also treat the first row in memory as v = 0, with texture coordinate (0, 0) at the top-left of the image.

### Rendering into a texture and shadow-map lookups

When WebGL renders into a texture, window coordinates keep their bottom-left origin: window row 0 is written into texture row v = 0. A pass that reads the result back with `v = 0.5 · y_ndc + 0.5` therefore finds the texel that was written, with no flip. Rows only need flipping when an image leaves the GPU for a top-row-first format, as with `readPixels` in Lab 05. Lab 07's shadow lookup uses `(u, v, d) = 0.5 · ndc + 0.5` from the light's clip space, where d repeats the window-depth mapping `(z_ndc + 1) / 2` so it is comparable with the stored depth. In Direct3D 12, NDC y = +1 lands in row 0, so `v = 0.5 − 0.5 · y_ndc`, and `d = z_ndc`. In Vulkan, `d = z_ndc` as well; v follows whatever the shadow pass did with y: `0.5 + 0.5 · y_ndc` without a flip, `0.5 − 0.5 · y_ndc` with the negative-height viewport. Write the map and read it with the same rule.

### Color: linear light inside, sRGB at the edges

8-bit colors in this course are sRGB-encoded: color pickers, images, canvases and the default WebGL framebuffer all use the sRGB transfer function. Any arithmetic that adds or averages light must happen in linear light: interpolation between vertices, lighting, texture filtering and mipmapping, and blending. Decode on the way in, compute, and encode **once** on the way out. `labs/common/color.js` holds the formulas.

- Lab 01 deliberately mixes encoded values; Lab 02 adds a switch so you can compare.
- Labs 03 and 04 use flat colors with no mixing, so the question does not arise.
- Lab 05 decodes vertex colors in the vertex shader and encodes in the fragment shader (switchable).
- Lab 06 decodes textures with an `SRGB8_ALPHA8` texture, which the GPU decodes before filtering, and encodes the lit result in the shader.
- Labs 07 and 08 render into `SRGB8_ALPHA8` targets: shaders write linear light, the GPU encodes on write and decodes on sampling, anti-aliasing samples are averaged and resized in linear light, and the last pass encodes once for the canvas. Lab 08 samples sRGB textures as Lab 06 does.
- Debug views (depth, normals, UVs) write their values directly: they are measurements, not light.

In Part II, `*_SRGB` image formats make the hardware decode on sampling and encode on writing (including blending), and an sRGB swapchain format replaces the manual encode. HDR output uses an extended-range linear format (scRGB) or a PQ-encoded format (HDR10). Part II's color-science stage covers both.

## Checklist for a port

When you move any lab to a new API, write down and test each of these with a deliberately asymmetric scene, such as the F from Lab 02 or the red texture cell from Lab 06:

1. Is the image upright? (NDC y, viewport, framebuffer origin)
2. Are the right faces culled? (front-face setting after any y flip)
3. Is anything in front of the camera clipped by mistake? (depth range of the projection)
4. Does a non-symmetric matrix arrive in the shader unchanged? (storage order, transpose flag, `row_major` / `column_major`)
5. Is the red texture cell in the expected corner? (texture row order)
6. Does a 50 % gray ramp look right, and does a black/white checkerboard average to the right gray far away? (sRGB decode and encode happen exactly once)
7. Does each shadow fall on the side away from the light, touching its caster? (shadow-map row order and depth range)

## Sources

Checked against the official pages on 2026-11-15.

- WebGL 2 / OpenGL ES 3.0: [clip volume and polygon orientation](https://registry.khronos.org/OpenGL/specs/es/3.0/es_spec_3.0.pdf) (§2.18, §3.6.1), [`glFrontFace`](https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glFrontFace.xhtml) (initial value CCW), [`glDepthRangef`](https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glDepthRangef.xhtml), [`glReadPixels`](https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glReadPixels.xhtml) (lower-left origin), [`glUniform`](https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glUniform.xhtml) (transpose).
- Vulkan: [viewport transform](https://docs.vulkan.org/spec/latest/chapters/vertexpostproc.html#vertexpostproc-viewport) (upper-left origin), [clip volume](https://docs.vulkan.org/spec/latest/chapters/vertexpostproc.html#vertexpostproc-clipping) (0 ≤ z ≤ w), [polygon orientation](https://docs.vulkan.org/spec/latest/chapters/primsrast.html#primsrast-polygons-basic) (area computed in framebuffer coordinates, with a leading minus sign), [`VK_KHR_maintenance1`](https://docs.vulkan.org/refpages/latest/refpages/source/VK_KHR_maintenance1.html) (negative viewport height, core in 1.1), [`VK_EXT_depth_clip_control`](https://docs.vulkan.org/refpages/latest/refpages/source/VK_EXT_depth_clip_control.html), [GLSL for Vulkan defaults](https://github.com/KhronosGroup/GLSL/blob/main/extensions/khr/GL_KHR_vulkan_glsl.txt) (column-major blocks).
- Direct3D: [`D3D12_RASTERIZER_DESC`](https://learn.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_rasterizer_desc) (FrontCounterClockwise defaults to FALSE), [pixel coordinates](https://learn.microsoft.com/en-us/windows/win32/direct3d10/d3d10-graphics-programming-guide-resources-coordinates) (upper-left origin), [viewports and clipping](https://learn.microsoft.com/en-us/windows/win32/direct3d9/viewports-and-clipping) (clip-space y from 1 to −1, flipped by the viewport), [HLSL matrix packing](https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-per-component-math), [DirectXMath conventions](https://learn.microsoft.com/en-us/windows/win32/dxmath/pg-xnamath-getting-started).
- Color: [sRGB, IEC 61966-2-1](https://webstore.iec.ch/en/publication/6169), with the transfer functions also given in [CSS Color 4](https://www.w3.org/TR/css-color-4/); [Advanced Color and HDR in DirectX](https://learn.microsoft.com/en-us/windows/win32/direct3darticles/high-dynamic-range); [Vulkan swapchain color spaces](https://docs.vulkan.org/refpages/latest/refpages/source/VkColorSpaceKHR.html).
