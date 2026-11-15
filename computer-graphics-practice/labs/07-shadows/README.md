# Lab 07 — Shadows and image quality

**English** | [繁體中文](README.zh-TW.md)

## Goal

Add the first effect that needs more than one pass. Render the scene's depth from the light into a texture, then let every camera sample compare its own distance from the light with that shadow map. Make the bias trade-off visible (shadow acne versus detached shadows), filter the shadow lookups with PCF, and compare three anti-aliasing strategies at the same zoom. Be able to say what each quality setting costs and how you measured it.

```
pass 1  shadow   world --V_light, P_light (orthographic)--> depth test --> shadow map (a depth texture)
pass 2  camera   world --V, P----------------------------> depth test --> color target (1 or 4 samples per pixel)
                 world --B · P_light · V_light--> (u, v, d)    lit = d − bias ≤ map(u, v), averaged over the taps
pass 3  present  color target --average the samples in linear light--> linearToSrgb --> canvas
```

## Run

Open `../../index.html` and choose **07 · Shadows**. Drag on the canvas to orbit; click without dragging to aim the magnifier (the orange box). The left magnifier always shows the aliased reference, one sample per pixel and one nearest shadow tap; the right one shows the current settings. Both show the same 40 × 27 pixels at 12× zoom. The "Show" select replaces the shaded image with the shadow factor, the shadow-map texel grid, the depth margin, or the shadow map itself. The page needs WebGL 2 but no server or network connection.

The GPU-time readout uses the `EXT_disjoint_timer_query_webgl2` extension. If the browser does not expose it, the readout shows a dash; exercise H describes what to measure instead.

## Read the implementation

1. **The scene.** A ground plane, a cube, a sphere and a thin pillar, built by `../common/mesh.js` in Lab 06's 32-byte vertex layout. Every object rests on the ground, because contact points are where a detached shadow shows first.
2. **The light's camera.** `lightFrame` puts a camera `LIGHT_DISTANCE` units from the scene along the light direction L and gives it an orthographic box (`orthographic()` from `../common/math.js`): a directional light has parallel rays, so there is no perspective. "Light box half-size" sets how much of the world the box covers, and therefore how wide one shadow-map texel is. Near and far enclose a sphere around the scene; their distance is the depth range that the bias is measured in.
3. **Render to texture.** `webgl.depthTarget` in `../common/webgl.js` creates a `DEPTH_COMPONENT24` texture and a framebuffer whose only attachment is that texture. `shadowPass` binds it, sets the viewport to the map size, clears depth to 1, and draws every object with a vertex shader that only transforms positions and a fragment shader that writes nothing: the depth test keeps the nearest depth per texel. "Shadow pass culls" chooses which faces get stored.
4. **From world to shadow-map coordinates.** `shadowMatrix = B · P_light · V_light`, where `NDC_TO_TEXTURE` (B) maps NDC [−1, 1]³ to [0, 1]³. The vertex shader outputs `vShadowCoord = shadowMatrix · world` beside `gl_Position`, and the rasterizer interpolates it like any other varying.
5. **The comparison.** The depth texture has `TEXTURE_COMPARE_MODE = COMPARE_REF_TO_TEXTURE` with `LEQUAL`, so the fragment shader reads it through a `sampler2DShadow`: `texture(uShadowMap, vec3(uv, reference))` returns 1 when `reference ≤ stored depth` and 0 otherwise. With `NEAREST` filtering that compares one texel; with `LINEAR` the hardware compares the 4 nearest texels and blends the 0 / 1 results ("Tap compare"). A sampler object with the comparison switched off (`webgl.rawDepthSampler`) reads the same texture as plain depth for the debug views.
6. **Bias.** `depthBias(n)` returns `constant + slope · texelDepth · tan θ`, where θ is the angle between the normal and L and `texelDepth` is one texel's width divided by the depth range. A surface tilted away from the light changes depth by `texel width · tan θ` from one texel to the next, so the slope term says how many texels of that change to forgive. The constant term is in depth units of the whole light box (0..1 spans 18 world units here). The readouts convert both into world units.
7. **PCF.** `shadowFactor` compares a 1 × 1, 3 × 3 or 5 × 5 grid of taps, one texel apart, against the same reference and averages the results: percentage-closer filtering. Averaging the depths first and comparing once would give a hard edge again.
8. **Anti-aliasing.** The canvas has neither a depth buffer nor antialiasing. The camera pass always draws into an offscreen `SRGB8_ALPHA8` target from `webgl.colorTarget`, which encodes linear values to sRGB on write and decodes them on sampling.
   - **Off:** one sample per pixel.
   - **MSAA 4×:** multisample renderbuffers. Coverage and depth are tested at 4 sample positions per pixel, but the fragment shader runs once per pixel per triangle. `webgl.resolve` (`blitFramebuffer`) averages the samples into a 1-sample texture.
   - **SSAA 2 × 2:** a target twice as wide and twice as tall. Everything, including the shadow lookup, runs 4 times per pixel, and `presentPass` averages each 2 × 2 block in linear light.

   `presentPass` then encodes once with `linearToSrgb` and writes the canvas.
9. **Magnifiers.** `render` draws the reference first, only inside the magnifier's pixels (`gl.scissor`), and copies those pixels back with `readPixels`, flipping the rows as in Lab 05. Then it draws the current settings over the whole canvas and reads the same pixels again. The last readout shows the center pixel's sRGB values for both.
10. **Cost.** The memory and work readouts are computed from the settings. The GPU-time readout wraps the shadow pass, and the camera and present passes, in timer queries; the reference is not timed. `performance.now()` around WebGL calls measures only the JavaScript thread (Lab 05, checkpoint 5).

Deliberately left for you: the light box does not follow the camera, the bias exists only in the shader, every PCF tap compares against the depth at the center, the light is directional only, and the map is 24-bit. `TODO(exercise …)` comments mark where each change goes.

## Conventions

This lab relies on [`../../CONVENTIONS.md`](../../CONVENTIONS.md) and adds one item, the shadow-map lookup:

- **WebGL:** `(u, v, d) = 0.5 · ndc + 0.5`, with no y flip. Rendering into a texture writes window row 0, the bottom row, into texture row v = 0, so `v = 0.5 · y_ndc + 0.5` reads back the texel that was written. d uses the same `(z_ndc + 1) / 2` mapping as the stored window depth, so the two are comparable.
- **Direct3D 12:** NDC z is already the stored depth, so `d = z_ndc`. NDC y = +1 lands in row 0, which is v = 0, so `v = 0.5 − 0.5 · y_ndc`.
- **Vulkan:** `d = z_ndc` as well. Without a y flip in the shadow pass, NDC y = −1 lands in row 0 and `v = 0.5 + 0.5 · y_ndc`; with the negative-height viewport Part II uses for upright images, `v = 0.5 − 0.5 · y_ndc`. Use the same rule for writing and for reading the map.
- **Color targets** are `SRGB8_ALPHA8`: shaders write linear light, the GPU encodes on write and decodes on sampling, and the present pass encodes once for the canvas. How the MSAA resolve averages sRGB samples is left to the implementation; exercise G checks it.

## Exercises

- **A — Normal offset:** move the lookup point along the surface normal before projecting it into the shadow map: `world + n · k · texel width`, with k around 1. Do it in the vertex shader (offset the position you multiply by `uShadowMatrix`) or pass the world position and the matrix to the fragment shader. Set Slope bias to 0 and compare the acne and the cube's contact shadow at a 10° light with the depth-bias-only version. Which one keeps the shadow closer to the cube, and why?
- **B — Polygon offset:** move the bias into the shadow pass with `gl.enable(gl.POLYGON_OFFSET_FILL)` and `gl.polygonOffset(factor, units)` in `shadowPass`, and set both bias sliders to 0. Which argument plays the role of the slope term and which of the constant term? Why is `units` measured in the smallest resolvable depth difference instead of world units? Compare with Lab 04, exercise F.
- **C — Fit and stabilize the light box:** compute the box from the camera instead of the slider: transform the corners of the camera's view frustum (cut off at a maximum distance) into light view space and take their bounds. Print the texel width it gives. Then compare two kinds of shimmer: turn on Orbit light at 128 texels (the texel grid rotates with the light), and orbit the camera with the fitted box (the grid slides by fractions of a texel). Snap the box center to whole texels in light space. Which shimmer does that remove, and why not the other?
- **D — PCF needs more bias:** set Slope bias to 1 and compare 1 tap, PCF 3 × 3 and PCF 5 × 5 in "Shadow factor only". Use the last line of the bias readout to explain why larger kernels show acne. Then fix it without raising the bias: take the screen-space derivatives of the shadow coordinate with `dFdx` and `dFdy`, solve for how d changes per texel in u and v, and give every tap its own reference depth (receiver-plane depth bias).
- **E — Spot light:** replace the orthographic light with `perspective()` and a light position. The lookup now needs the divide by w (or `textureProj`), the texel width grows with distance from the light, and the stored depth is no longer linear in distance. Which bias term has to change, and how?
- **F — 16-bit shadow map:** allocate the map as `DEPTH_COMPONENT16` with `UNSIGNED_SHORT`. Set Slope bias to 0 and the light elevation to 90°, and find the smallest constant bias that removes the acne with 24 bits and with 16 bits. Relate each to one depth step of the 18-unit box; the texel readout shows the 24-bit step.
- **G — Resolve in the wrong space:** in `PRESENT_FRAGMENT_SHADER`, encode each texel with `linearToSrgb`, average the encoded values, and skip the final encode. With SSAA, aim the magnifier at the sphere's outline against the dark background and compare the center values. Then switch to MSAA at the same pixel: does this browser's resolve of an `SRGB8_ALPHA8` renderbuffer average in linear light?
- **H — Measure the cost:** change one setting at a time (shadow map size, taps, tap compare, anti-aliasing) and record the GPU time of both passes. Which cost grows with shadow-map texels, which with screen pixels × samples, and which with taps per shaded sample? If the timer readout shows a dash, draw the camera pass N times per frame and raise N until the frame rate drops below the display's refresh rate, then compare N between settings. Write a bottleneck hypothesis and a measurement that could refute it.

## Checkpoints

Answer these in your lab notebook:

1. **Shadow acne.** From your screenshot, explain why a lit surface shadows itself. In the depth-margin view, why does the sign flip within a few pixels, and why does the pattern depend on the light angle and on the map size?
2. **Detached shadows.** Give the bias values that produced peter-panning and how far the shadow moved away from its caster. Why does a constant bias large enough for grazing light detach shadows at contact points? Which pair of bias values did you settle on, and for which kernel?
3. **Two passes.** Trace one ground point through both passes: world → light view → light clip → NDC → (u, v, d) → texel → comparison. Which matrix maps NDC to texture coordinates, why does WebGL need no y flip there, and what changes in Direct3D 12?
4. **Edges at the same zoom.** Compare a silhouette edge and a shadow edge in the reference and with filtering. Which setting smooths which edge? Why does MSAA not smooth the shadow edge while SSAA does?
5. **Cost versus quality.** For each quality setting, state what it costs: memory, taps per shaded sample, samples per pixel, shaded samples per pixel. Which one did your measurements show to be the most expensive on your GPU, and is that the one the counts predicted?

## Where this shows up later

- **Engines:** Part II's engine track turns this lab into cascaded shadow maps (E4): one light box per distance range, PCF, cascade blending, and texel snapping against shimmer. Temporal anti-aliasing (E5) replaces SSAA's extra samples with samples spread over several frames, and E7 compares shadow maps with ray-traced shadows.
- **Compositors:** vector paths and text (T1–T3) are anti-aliased by computing how much of each pixel a shape covers, instead of by taking more samples. Window effects are render-to-texture passes, and anti-aliased edges are blended in linear light by the same rule as the resolve here.

## Next

Lab 08 combines Labs 05–07 into a small navigable renderer and asks for a short design note about it.
