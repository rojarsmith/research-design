# Lab 06 — Materials, light and texture

## Goal

Give surfaces a response to light. Add normals and UV coordinates to the Lab 05 vertex layout, compute Lambert diffuse and Blinn–Phong specular lighting, sample a texture with different filters, and do the lighting math in linear light so the result looks right on an sRGB display. Be able to explain each term, which space it is computed in, and what goes wrong when you skip a step.

```
position --M--------------> world position ─┐
normal   --(M⁻¹)ᵀ---------> world normal N ──┼─> N·L, N·H ─> diffuse + specular ─┐
uv       -----------------> texture sample ─┴───────────────> albedo ───────────┴─> linear color ─> sRGB encode ─> screen
```

## Run

Open `../../index.html` and choose **06 · Light & texture**. Turn Spin off when you want to study one pose. Every select and slider changes one thing; the "Show" select replaces the shaded image with the normals, the UVs, or the raw `N · L` term. The page needs WebGL 2 but no server or network connection.

## Read the implementation

1. **Vertex layout.** Each vertex is position, normal, UV: 8 floats, 32 bytes, as in Lab 05 but with the color replaced by a normal. `makeSphere` builds a UV sphere (on a unit sphere the normal equals the position). `makeCube` uses 4 vertices per face so each face has its own flat normal.
2. **Normal matrix.** Positions use the model matrix M. Normals use `normalMatrix(M)`, the inverse transpose of M's upper 3×3, computed on the CPU as the cofactor matrix divided by the determinant. With rotation only, this equals M's 3×3; with non-uniform scale it does not. The "Normals use → M itself" option uploads the wrong matrix on purpose.
3. **Lighting in world space.** The light is directional: `uLightDir` points toward the light, from the azimuth and elevation sliders. The camera position `uEye` gives the view vector V. Diffuse is `max(N · L, 0)`. Specular uses the half vector `H = normalize(L + V)` and `max(N · H, 0)^shininess`, and is zero on faces turned away from the light.
4. **Per vertex or per fragment.** The vertex shader always computes the two factors per vertex (`vGouraud`). In per-fragment mode, the fragment shader ignores them and recomputes both from the interpolated, renormalized normal.
5. **Texture.** `makeTextureImage` draws a 256 × 256 checkerboard on a 2D canvas, with a red cell marking the first row and column, so no image file is needed. It is uploaded twice: as `SRGB8_ALPHA8`, which the GPU decodes to linear values before filtering and mipmapping, and as plain `RGBA8`, whose encoded numbers reach the shader unchanged. Both get a full mip chain from `generateMipmap`. "UV repeat" multiplies the UVs, so the checkerboard gets smaller on screen and minification filtering matters.
6. **Color space.** In "Linear light, sRGB output" mode, the constant base color is decoded from sRGB with `srgbToLinear`, the sRGB texture is decoded by the hardware, light is added in linear units, and `linearToSrgb` encodes the result for the display. In "Directly on sRGB numbers" mode, every step uses the encoded numbers as if they were linear, which is what many early renderers did.

The coordinate and color conventions are the course-wide ones in [`../../CONVENTIONS.md`](../../CONVENTIONS.md), and the matrix and normal-matrix helpers come from `../common/math.js`.

Deliberately left for you: the light is directional only, the specular term has no Fresnel factor and no energy normalization, mipmaps come from `generateMipmap`, and there is no normal mapping. `TODO(exercise …)` comments mark where they go.

## Exercises

- **A — Point light:** replace the directional light with a point light at a world position. Compute L per vertex or per fragment and add `1 / d²` falloff. Which shading mode now shows a visible difference on the cube, and why?
- **B — Fresnel:** multiply the specular term by Schlick's approximation `F = F₀ + (1 − F₀)(1 − V · H)⁵` with `F₀ = 0.04`. Where on the sphere does the highlight get stronger?
- **C — Energy:** as you raise Shininess, the highlight gets smaller but no brighter. Multiply the specular term by `(shininess + 8) / (8π)` and compare. Which version keeps the total reflected light closer to constant?
- **D — Decode before filtering:** use the `RGBA8` texture in linear mode and decode it in the shader with `srgbToLinear` after sampling. Compare the far checkerboard at UV repeat 16 with trilinear filtering against the `SRGB8_ALPHA8` version. Why must decoding happen before the hardware averages texels?
- **E — Mipmaps by hand:** build the mip chain yourself: average each 2 × 2 block of the level above (decode to linear, average, encode again), and upload each level with `texImage2D(gl.TEXTURE_2D, level, …)`. Compare with `generateMipmap`. Then average the encoded values instead and describe the difference.
- **F — Which way is up:** find the red cell on the cube and on the sphere. Set `gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, true)` before uploading and look again. Write down the convention for image rows, texture `v`, NDC y, and pixel rows used in this course.
- **G — Normal mapping (stretch):** add a tangent attribute, generate a small bump pattern as a normal map on a canvas, and perturb N in tangent space. What must happen to the tangent under the squash?
- **H — Cost:** compare the number of vertex-shader runs (vertices) with fragment-shader runs (covered samples) for the sphere and the cube. When is per-vertex lighting a good trade, and when does it fail visibly?

## Checkpoints

Answer these in your lab notebook:

1. Why do normals need the inverse transpose of the model matrix under non-uniform scale? Use the squashed sphere and the "Normals as color" view as evidence, and show the 2D case on paper.
2. Which vectors must be in the same space for `N · L` and `N · H`? Which space does this code use, and what would change if you lit in view space instead?
3. Why does per-vertex lighting lose the cube's highlight, while the sphere only becomes blocky?
4. A black-and-white checkerboard viewed from far away should look like a 50 % mix. What 8-bit value does each color-math mode produce for that mix, and which one matches what your eye expects? Use the `srgbToLinear` and `linearToSrgb` formulas.
5. Where exactly does sRGB decoding happen in the correct mode, and why must it happen before filtering, blending, and lighting?

## Where this shows up later

- **Engines:** physically based shading (Part II) replaces Blinn–Phong but keeps the same skeleton: world-space normals, linear-light math, sRGB textures decoded before filtering, and mipmaps.
- **Compositors:** blending windows and text in linear light, sRGB and HDR output encoding, and color management are the same decode → math → encode rule applied to whole surfaces.

## Next

Lab 07 in the syllabus adds a shadow map and anti-aliasing experiments. It is not implemented yet. It builds directly on this lab's light direction and texture sampling, and on the depth testing and precision experiments from Lab 04.
