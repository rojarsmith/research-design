// Lab 07: shadows and image quality. Lab 06's lighting, drawn in three passes:
//
//   pass 1  shadow:  render the scene from the light into a depth texture (the shadow map)
//   pass 2  camera:  render from the camera; each sample looks itself up in the shadow map
//                    and asks "was anything closer to the light than me?"
//   pass 3  present: average 1, 4 (MSAA) or 2 × 2 (SSAA) samples per pixel in linear light,
//                    encode to sRGB once, and write the canvas
//
// The two magnifiers show the same pixels twice: the aliased reference (one sample per
// pixel, one shadow tap) and the current settings, so you compare edges at the same zoom.
//
// Work through README.md in this folder. TODO(exercise X) comments mark code you write.
(function () {
  const canvas = document.querySelector('#l7-screen');
  const W = canvas.width;
  const H = canvas.height;
  const byId = (id) => document.getElementById(id);
  const DEFAULTS = { bias: 0.001, slope: 2, frustum: 5, az: 65, el: 35, yaw: 30, pitch: 30, dist: 7.5 };
  const CHOICES = { size: '1024', taps: '3', compare: 'nearest', cull: 'back', aa: 'msaa', view: '0' };
  const sliders = Object.fromEntries(Object.keys(DEFAULTS).map((key) => [key, byId(`l7-${key}`)]));
  const selects = Object.fromEntries(Object.keys(CHOICES).map((key) => [key, byId(`l7-${key}`)]));
  const toggles = { shadows: true, orbit: false };
  const ZOOM = { w: 40, h: 27, scale: 12 }; // 40 × 27 pixels, each drawn as a 12 × 12 block
  const DEFAULT_FOCUS = [130, 178]; // canvas pixels: the cube's shadow edge in the default view
  const ORBIT_SPEED = 12; // degrees per second when "Orbit light" is on
  let focus = [...DEFAULT_FOCUS];
  let azimuth = DEFAULTS.az; // kept as a float: the slider rounds to whole degrees
  let frameRequest = 0;
  let lastTime = 0;
  let frameMs = 0;

  // ---------------------------------------------------------------------------
  // Shared helpers: matrices from ../common/math.js, sRGB from ../common/color.js, meshes
  // in Lab 06's 32-byte layout from ../common/mesh.js, and render targets, programs and the
  // GPU timer from ../common/webgl.js.
  // ---------------------------------------------------------------------------
  const {
    add, scale, multiply, fromRows, identity, translation, rotationY, radians, orbitEye, lookAt,
    perspective, orthographic, normalMatrix,
  } = window.cg.math;
  const { srgbToLinear, byteToLinear } = window.cg.color;
  const meshes = window.cg.mesh;
  const webgl = window.cg.webgl;

  // ---------------------------------------------------------------------------
  // The scene: a ground plane and three objects that touch it. Contact points are where a
  // detached shadow (peter-panning) shows first. Colors are sRGB, as a color picker shows them.
  // ---------------------------------------------------------------------------
  const SCENE = [
    { mesh: 'ground', model: identity(), color: [0.72, 0.74, 0.76] },
    { mesh: 'cube', model: multiply(translation(-1.1, 0.6, 0.3), rotationY(radians(20))), color: [0.85, 0.42, 0.28] },
    { mesh: 'sphere', model: translation(1.05, 0.6, 0.55), color: [0.36, 0.62, 0.86] },
    { mesh: 'pillar', model: multiply(translation(0.15, 1.0, -1.3), rotationY(radians(10))), color: [0.86, 0.78, 0.52] },
  ];
  const TARGET = [0, 0.5, 0]; // what the camera orbits and the light looks at
  const SCENE_RADIUS = 9; // a sphere around everything; it sets the light's near and far planes
  const LIGHT_DISTANCE = 12;
  const BACKGROUND = [10, 13, 18].map(byteToLinear); // the page's canvas color, as linear light

  // ---------------------------------------------------------------------------
  // The light's camera. A directional light has parallel rays, so its "camera" is an
  // orthographic box: halfSize sets how much ground one shadow map covers.
  // ---------------------------------------------------------------------------
  // NDC [−1, 1]³ -> texture coordinates and depth [0, 1]³. No y flip: rendering into a texture
  // writes window row 0, the bottom row, into texture row v = 0 (see the README's conventions).
  const NDC_TO_TEXTURE = fromRows([
    0.5, 0, 0, 0.5,
    0, 0.5, 0, 0.5,
    0, 0, 0.5, 0.5,
    0, 0, 0, 1,
  ]);

  // TODO(exercise C): fit halfSize and the box center to what the camera can see, and snap
  // the box to whole texels so the shadow edges stop shimmering when the light moves.
  function lightFrame(azimuthDegrees, elevationDegrees, halfSize) {
    const az = radians(azimuthDegrees);
    const el = radians(elevationDegrees);
    const dir = [Math.cos(el) * Math.sin(az), Math.sin(el), Math.cos(el) * Math.cos(az)]; // toward the light
    const eye = add(TARGET, scale(dir, LIGHT_DISTANCE));
    const up = Math.abs(dir[1]) > 0.99 ? [0, 0, -1] : [0, 1, 0];
    const near = LIGHT_DISTANCE - SCENE_RADIUS;
    const far = LIGHT_DISTANCE + SCENE_RADIUS;
    // TODO(exercise E): a spot light uses perspective() here, and the lookup needs the divide by w.
    const viewProjection = multiply(orthographic(halfSize, 1, near, far), lookAt(eye, TARGET, up));
    return { dir, viewProjection, shadowMatrix: multiply(NDC_TO_TEXTURE, viewProjection), near, far, range: far - near };
  }

  function cameraFrame() {
    const eye = orbitEye(Number(sliders.yaw.value), Number(sliders.pitch.value), Number(sliders.dist.value), TARGET);
    return { eye, viewProjection: multiply(perspective(radians(45), W / H, 0.1, 50), lookAt(eye, TARGET, [0, 1, 0])) };
  }

  // ---------------------------------------------------------------------------
  // Shaders.
  // ---------------------------------------------------------------------------
  // Pass 1 needs positions only. There is no color attachment, so the fragment shader
  // writes nothing: the depth test stores the depth of the nearest surface per texel.
  const DEPTH_VERTEX_SHADER = `#version 300 es
layout(location = 0) in vec3 aPosition;
uniform mat4 uLightViewProjection;
uniform mat4 uModel;
void main() {
  gl_Position = uLightViewProjection * uModel * vec4(aPosition, 1.0);
}
`;
  const DEPTH_FRAGMENT_SHADER = `#version 300 es
precision mediump float;
void main() {}
`;

  // Pass 2: Lab 06's lighting plus a shadow factor. The vertex shader carries each vertex
  // into the shadow map's coordinates as well as into the camera's clip space.
  const SCENE_VERTEX_SHADER = `#version 300 es
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

uniform mat4 uModel;
uniform mat4 uViewProjection;
uniform mat4 uShadowMatrix;   // world -> light clip space -> [0, 1]³ shadow-map coordinates
uniform mat3 uNormalMatrix;

out vec3 vWorldPos;
out vec3 vNormal;
out vec4 vShadowCoord;

void main() {
  vec4 world = uModel * vec4(aPosition, 1.0);
  vWorldPos = world.xyz;
  vNormal = uNormalMatrix * aNormal;
  vShadowCoord = uShadowMatrix * world;
  gl_Position = uViewProjection * world;
}
`;
  const SCENE_FRAGMENT_SHADER = `#version 300 es
precision highp float;
precision highp sampler2DShadow;

in vec3 vWorldPos;
in vec3 vNormal;
in vec4 vShadowCoord;

uniform sampler2DShadow uShadowMap;  // compares: 1.0 where the reference is ≤ the stored depth
uniform sampler2D uShadowDepth;      // the same texture read as plain depth (debug view only)
uniform vec3 uAlbedo;                // linear light
uniform vec3 uLightDir;              // world space, unit length, pointing toward the light
uniform vec3 uEye;
uniform bool uShadows;
uniform float uBiasConstant;         // in shadow-map depth units: the light's near..far is 0..1
uniform float uBiasSlope;            // in texels of the surface's depth slope
uniform float uTexelDepth;           // texel width / depth range: the depth change across one texel at 45°
uniform float uMapSize;              // shadow-map width in texels
uniform int uTaps;                   // 1, 3 or 5: a uTaps × uTaps PCF kernel
uniform int uView;                   // 0 shaded, 1 shadow factor, 2 shadow-map texels, 4 depth margin

out vec4 outColor;
${webgl.GLSL_COLOR}
// How much closer to the light than the stored depth a receiver may be and still count as lit.
// A tilted surface changes depth across one texel by texel width × tan θ, so the slope term grows
// toward grazing light; the constant term covers rounding.
float depthBias(vec3 n) {
  float c = clamp(dot(n, uLightDir), 0.05, 1.0);  // cos θ between the normal and the light
  float tanTheta = sqrt(1.0 - c * c) / c;
  return uBiasConstant + uBiasSlope * uTexelDepth * tanTheta;
}

// Percentage-closer filtering: compare each tap first, then average the 0 / 1 results.
// Averaging depths and comparing once would not give a soft edge.
float shadowFactor(vec3 p, float reference) {
  if (!uShadows) return 1.0;
  // Outside the light's box nothing was rendered into the map, so call it lit.
  if (any(lessThan(p, vec3(0.0))) || any(greaterThan(p, vec3(1.0)))) return 1.0;
  float texel = 1.0 / uMapSize;
  int r = uTaps / 2;
  float lit = 0.0;
  // TODO(exercise D): the outer taps compare against the receiver depth at the center;
  // correct the reference per tap with the depth gradient instead of raising the bias.
  for (int y = -r; y <= r; y++) {
    for (int x = -r; x <= r; x++) {
      lit += texture(uShadowMap, vec3(p.xy + vec2(x, y) * texel, reference));
    }
  }
  return lit / float(uTaps * uTaps);
}

void main() {
  vec3 n = normalize(vNormal);
  // TODO(exercise A): normal offset: move the lookup point along n by a fraction of a texel.
  vec3 p = vShadowCoord.xyz / vShadowCoord.w;  // w = 1 for an orthographic light (see exercise E)
  float reference = p.z - depthBias(n);
  float shadow = shadowFactor(p, reference);

  // Debug views are measurements, not light: pre-decode them so the final encode shows the number.
  if (uView == 1) { outColor = vec4(srgbToLinear(vec3(shadow)), 1.0); return; }
  if (uView == 4) {
    // Depth margin: reference minus stored depth, in units of uTexelDepth. Red: shadowed, blue: lit.
    float margin = (reference - texture(uShadowDepth, p.xy).r) / uTexelDepth;
    vec3 c = margin > 0.0 ? vec3(1.0, 0.25, 0.2) : vec3(0.2, 0.7, 1.0);
    outColor = vec4(srgbToLinear(c * clamp(abs(margin) / 4.0, 0.0, 1.0)), 1.0);
    return;
  }

  vec3 h = normalize(uLightDir + normalize(uEye - vWorldPos));
  float diffuse = max(dot(n, uLightDir), 0.0);
  float specular = diffuse > 0.0 ? pow(max(dot(n, h), 0.0), 48.0) : 0.0;
  vec3 ambient = uAlbedo * (0.08 + 0.06 * n.y);
  vec3 color = ambient + shadow * (uAlbedo * diffuse + vec3(0.25 * specular));
  if (uView == 2) {
    vec2 cell = floor(p.xy * uMapSize);  // which shadow-map texel this sample looks up
    color *= mod(cell.x + cell.y, 2.0) > 0.5 ? 1.0 : 0.55;
  }
  outColor = vec4(clamp(color, 0.0, 1.0), 1.0);  // linear; the SRGB8_ALPHA8 target encodes on write
}
`;

  // Pass 3: one texel per pixel, or the average of a 2 × 2 block for SSAA. Sampling the
  // SRGB8_ALPHA8 texture decodes it, so the average is taken in linear light.
  const PRESENT_FRAGMENT_SHADER = `#version 300 es
precision highp float;
uniform sampler2D uColor;
uniform int uFactor;   // 1, or 2 for a target twice as wide and tall as the canvas
out vec4 outColor;
${webgl.GLSL_COLOR}
void main() {
  ivec2 base = ivec2(gl_FragCoord.xy) * uFactor;
  vec3 sum = vec3(0.0);
  for (int y = 0; y < uFactor; y++) {
    for (int x = 0; x < uFactor; x++) sum += texelFetch(uColor, base + ivec2(x, y), 0).rgb;
  }
  // TODO(exercise G): average the encoded values instead and compare the silhouettes.
  outColor = vec4(linearToSrgb(sum / float(uFactor * uFactor)), 1.0);
}
`;

  // "Light's view": the shadow map itself, read through the raw sampler. Near is dark.
  const DEPTH_VIEW_FRAGMENT_SHADER = `#version 300 es
precision highp float;
in vec2 vUV;
uniform sampler2D uDepth;
out vec4 outColor;
void main() {
  outColor = vec4(vec3(texture(uDepth, vUV).r), 1.0);  // a measurement, written directly
}
`;

  // ---------------------------------------------------------------------------
  // WebGL setup, once. The canvas itself has no depth buffer and no antialiasing: every
  // camera pass draws into an offscreen target, and pass 3 is the only one that touches it.
  // ---------------------------------------------------------------------------
  const gl = canvas.getContext('webgl2', { antialias: false, alpha: false, depth: false });
  const zoomRef = byId('l7-zoom-ref').getContext('2d', { alpha: false });
  const zoomCur = byId('l7-zoom-cur').getContext('2d', { alpha: false });
  const zoomImages = [zoomRef, zoomCur].map((ctx) => ctx.createImageData(ZOOM.w * ZOOM.scale, ZOOM.h * ZOOM.scale));
  let programs = null;
  let gpuMeshes = null;
  let shadowMap = null;
  let rawSampler = null;
  let targets = null;
  let timer = null;

  function setup() {
    programs = {
      depth: webgl.program(gl, DEPTH_VERTEX_SHADER, DEPTH_FRAGMENT_SHADER),
      scene: webgl.program(gl, SCENE_VERTEX_SHADER, SCENE_FRAGMENT_SHADER),
      present: webgl.program(gl, webgl.FULLSCREEN_VERTEX_SHADER, PRESENT_FRAGMENT_SHADER),
      depthView: webgl.program(gl, webgl.FULLSCREEN_VERTEX_SHADER, DEPTH_VIEW_FRAGMENT_SHADER),
    };
    gpuMeshes = {
      ground: webgl.uploadMesh(gl, meshes.plane(6)),
      cube: webgl.uploadMesh(gl, meshes.box(0.6, 0.6, 0.6)),
      sphere: webgl.uploadMesh(gl, meshes.sphere(0.6, 24, 48)),
      pillar: webgl.uploadMesh(gl, meshes.box(0.16, 1.0, 0.16)),
    };
    shadowMap = webgl.depthTarget(gl, Number(CHOICES.size));
    rawSampler = webgl.rawDepthSampler(gl);
    targets = {
      single: webgl.colorTarget(gl, W, H, 1), // also receives the MSAA resolve
      msaa: webgl.colorTarget(gl, W, H, 4),
      ssaa: webgl.colorTarget(gl, 2 * W, 2 * H, 1),
    };
    timer = webgl.gpuTimer(gl);
  }

  // Limit drawing to the magnifier's pixels (scaled to the target's size), or switch it off.
  function setScissor(region, target) {
    if (!region) {
      gl.disable(gl.SCISSOR_TEST);
      return;
    }
    const k = target ? target.width / W : 1;
    gl.enable(gl.SCISSOR_TEST);
    gl.scissor(region.x * k, region.glY * k, ZOOM.w * k, ZOOM.h * k);
  }

  function drawScene(program, perObject) {
    SCENE.forEach((object) => {
      const mesh = gpuMeshes[object.mesh];
      perObject(object);
      gl.uniformMatrix4fv(program.uniforms.uModel, false, new Float32Array(object.model));
      gl.bindVertexArray(mesh.vao);
      gl.drawElements(gl.TRIANGLES, mesh.count, gl.UNSIGNED_SHORT, 0);
    });
    gl.bindVertexArray(null);
  }

  // Pass 1: depth only, from the light, into the shadow map.
  function shadowPass(light, s) {
    shadowMap.resize(s.size);
    gl.bindFramebuffer(gl.FRAMEBUFFER, shadowMap.framebuffer);
    gl.viewport(0, 0, s.size, s.size);
    setScissor(null);
    gl.enable(gl.DEPTH_TEST);
    gl.depthFunc(gl.LESS);
    gl.clearDepth(1);
    gl.clear(gl.DEPTH_BUFFER_BIT);
    if (s.cull === 'none') {
      gl.disable(gl.CULL_FACE);
    } else {
      gl.enable(gl.CULL_FACE);
      gl.cullFace(s.cull === 'front' ? gl.FRONT : gl.BACK);
    }
    // TODO(exercise B): gl.enable(gl.POLYGON_OFFSET_FILL) and gl.polygonOffset(factor, units)
    // here move the bias into the shadow pass; then set the shader bias to zero.
    const program = programs.depth;
    gl.useProgram(program.program);
    gl.uniformMatrix4fv(program.uniforms.uLightViewProjection, false, new Float32Array(light.viewProjection));
    drawScene(program, () => {});
    gl.disable(gl.CULL_FACE);
  }

  // Pass 2: the camera's view into `target`, looking every sample up in the shadow map.
  function cameraPass(target, camera, light, s, region) {
    gl.bindFramebuffer(gl.FRAMEBUFFER, target.framebuffer);
    gl.viewport(0, 0, target.width, target.height);
    setScissor(region, target);
    gl.enable(gl.DEPTH_TEST);
    gl.depthFunc(gl.LESS);
    gl.enable(gl.CULL_FACE);
    gl.cullFace(gl.BACK);
    gl.clearColor(...BACKGROUND, 1);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    const program = programs.scene;
    const u = program.uniforms;
    gl.useProgram(program.program);
    // Unit 0 compares (sampler2DShadow). NEAREST compares one texel; LINEAR makes the
    // hardware compare the 4 nearest texels and blend the results (a 2 × 2 PCF in one tap).
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, shadowMap.texture);
    const filter = s.bilinear ? gl.LINEAR : gl.NEAREST;
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, filter);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, filter);
    // Unit 1 reads the same texture as plain depth through a sampler object.
    gl.activeTexture(gl.TEXTURE1);
    gl.bindTexture(gl.TEXTURE_2D, shadowMap.texture);
    gl.bindSampler(1, rawSampler);
    gl.uniform1i(u.uShadowMap, 0);
    gl.uniform1i(u.uShadowDepth, 1);

    gl.uniformMatrix4fv(u.uViewProjection, false, new Float32Array(camera.viewProjection));
    gl.uniformMatrix4fv(u.uShadowMatrix, false, new Float32Array(light.shadowMatrix));
    gl.uniform3fv(u.uLightDir, light.dir);
    gl.uniform3fv(u.uEye, camera.eye);
    gl.uniform1i(u.uShadows, s.shadows ? 1 : 0);
    gl.uniform1f(u.uBiasConstant, s.bias);
    gl.uniform1f(u.uBiasSlope, s.slope);
    gl.uniform1f(u.uTexelDepth, (2 * s.frustum) / s.size / light.range);
    gl.uniform1f(u.uMapSize, s.size);
    gl.uniform1i(u.uTaps, s.taps);
    gl.uniform1i(u.uView, s.view);
    drawScene(program, (object) => {
      gl.uniformMatrix3fv(u.uNormalMatrix, false, new Float32Array(normalMatrix(object.model)));
      gl.uniform3fv(u.uAlbedo, object.color.map(srgbToLinear));
    });
    gl.disable(gl.CULL_FACE);
    gl.disable(gl.DEPTH_TEST);
  }

  // Pass 3: into the canvas, encoding to sRGB exactly once.
  function presentPass(source, factor, region) {
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    gl.viewport(0, 0, W, H);
    setScissor(region);
    const program = programs.present;
    gl.useProgram(program.program);
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, source.texture);
    gl.uniform1i(program.uniforms.uColor, 0);
    gl.uniform1i(program.uniforms.uFactor, factor);
    gl.drawArrays(gl.TRIANGLES, 0, 3);
  }

  // Debug view "Light's view": the shadow map in a square in the middle of the canvas.
  function lightViewPass() {
    gl.bindFramebuffer(gl.FRAMEBUFFER, null);
    setScissor(null);
    gl.viewport(0, 0, W, H);
    gl.clearColor(10 / 255, 13 / 255, 18 / 255, 1);
    gl.clear(gl.COLOR_BUFFER_BIT);
    gl.viewport((W - H) / 2, 0, H, H);
    const program = programs.depthView;
    gl.useProgram(program.program);
    gl.activeTexture(gl.TEXTURE0);
    gl.bindTexture(gl.TEXTURE_2D, shadowMap.texture);
    gl.bindSampler(0, rawSampler);
    gl.uniform1i(program.uniforms.uDepth, 0);
    gl.drawArrays(gl.TRIANGLES, 0, 3);
    gl.bindSampler(0, null);
  }

  // ---------------------------------------------------------------------------
  // The magnifiers: read 40 × 27 canvas pixels back and draw each as a 12 × 12 block.
  // ---------------------------------------------------------------------------
  function zoomRegion() {
    const x = Math.round(Math.min(W - ZOOM.w, Math.max(0, focus[0] - ZOOM.w / 2)));
    const y = Math.round(Math.min(H - ZOOM.h, Math.max(0, focus[1] - ZOOM.h / 2)));
    return { x, y, glY: H - y - ZOOM.h }; // y counts rows from the top; WebGL counts from the bottom
  }

  function readRegion(region) {
    const pixels = new Uint8Array(ZOOM.w * ZOOM.h * 4);
    gl.readPixels(region.x, region.glY, ZOOM.w, ZOOM.h, gl.RGBA, gl.UNSIGNED_BYTE, pixels);
    const rows = new Uint8Array(pixels.length); // flip: readPixels returns the bottom row first
    for (let y = 0; y < ZOOM.h; y++) rows.set(pixels.subarray((ZOOM.h - 1 - y) * ZOOM.w * 4, (ZOOM.h - y) * ZOOM.w * 4), y * ZOOM.w * 4);
    return rows;
  }

  function drawZoom(ctx, image, pixels) {
    const data = image.data;
    const width = ZOOM.w * ZOOM.scale;
    for (let y = 0; y < ZOOM.h * ZOOM.scale; y++) {
      for (let x = 0; x < width; x++) {
        const src = (Math.floor(y / ZOOM.scale) * ZOOM.w + Math.floor(x / ZOOM.scale)) * 4;
        const dst = (y * width + x) * 4;
        // A darker last row and column per block keeps single pixels countable.
        const k = x % ZOOM.scale === ZOOM.scale - 1 || y % ZOOM.scale === ZOOM.scale - 1 ? 0.8 : 1;
        data[dst] = pixels[src] * k;
        data[dst + 1] = pixels[src + 1] * k;
        data[dst + 2] = pixels[src + 2] * k;
        data[dst + 3] = 255;
      }
    }
    ctx.putImageData(image, 0, 0);
  }

  function placeZoomBox(region) {
    const box = byId('l7-zoom-box').style;
    box.left = `${(100 * region.x) / W}%`;
    box.top = `${(100 * region.y) / H}%`;
    box.width = `${(100 * ZOOM.w) / W}%`;
    box.height = `${(100 * ZOOM.h) / H}%`;
  }

  // ---------------------------------------------------------------------------
  // One frame.
  // ---------------------------------------------------------------------------
  const fmt = (v) => (Math.abs(v) < 5e-4 ? 0 : v).toFixed(2).padStart(6);
  const mib = (bytes) => `${(bytes / 1048576).toFixed(1)} MiB`;

  function readSettings() {
    return {
      size: Number(selects.size.value),
      taps: Number(selects.taps.value),
      bilinear: selects.compare.value === 'linear',
      cull: selects.cull.value,
      aa: selects.aa.value,
      view: Number(selects.view.value),
      bias: Number(sliders.bias.value),
      slope: Number(sliders.slope.value),
      frustum: Number(sliders.frustum.value),
      shadows: toggles.shadows,
    };
  }

  function showReadouts(s, light) {
    const texel = (2 * s.frustum) / s.size; // world units per shadow-map texel
    const texelDepth = texel / light.range;
    const biasAt = (degrees) => s.bias + s.slope * texelDepth * Math.tan(radians(degrees));
    const world = (depth) => (depth * light.range).toFixed(3);
    byId('l7-light').textContent = `(${light.dir.map(fmt).join(',')})`;
    byId('l7-box').textContent =
      `±${s.frustum.toFixed(1)} × ±${s.frustum.toFixed(1)} u\nnear ${light.near} · far ${light.far} · ${light.range} u deep`;
    byId('l7-texel').textContent = `${texel.toFixed(4)} u wide\n24-bit depth step ${(light.range / 16777215).toExponential(1)} u`;
    // The farthest texel center any tap can read, measured from the sample along one axis.
    const reach = Math.floor(s.taps / 2) + (s.bilinear ? 1 : 0.5);
    byId('l7-bias-total').textContent = [0, 45, 80]
      .map((deg) => `θ = ${String(deg).padStart(2)}°  ${biasAt(deg).toFixed(4)} = ${world(biasAt(deg))} u`)
      .concat(`taps read texels up to ${reach} texels away along each axis`).join('\n');
    const shaded = s.aa === 'ssaa' ? 4 : 1;
    const samples = s.aa === 'off' ? 1 : 4;
    const target = targets ? targets[s.aa === 'off' ? 'single' : s.aa] : null;
    const targetBytes = target ? target.bytes + (s.aa === 'msaa' ? targets.single.bytes : 0) : 0;
    byId('l7-memory').textContent = `shadow map ${s.size}² × 4 B = ${mib(s.size * s.size * 4)}\ncamera targets ${mib(targetBytes)}`;
    const triangles = gpuMeshes ? SCENE.reduce((sum, o) => sum + gpuMeshes[o.mesh].triangles, 0) : 0;
    byId('l7-work').textContent =
      `shadow pass ${s.size}² texels · ${triangles.toLocaleString()} triangles\n` +
      `camera ${W} × ${H} px · ${samples} sample${samples > 1 ? 's' : ''} · shaded ${shaded}× per px\n` +
      `shadow taps ${s.taps * s.taps} per shaded sample${s.bilinear ? ' × 4 texels' : ''}`;
  }

  function render() {
    const started = performance.now();
    Object.values(sliders).forEach((el) => { el.nextElementSibling.value = el.value; });
    const s = readSettings();
    const light = lightFrame(azimuth, Number(sliders.el.value), s.frustum);
    const camera = cameraFrame();
    const region = zoomRegion();
    placeZoomBox(region);
    showReadouts(s, light);
    if (!gl) return;

    if (timer) timer.begin('shadow');
    shadowPass(light, s);
    if (timer) timer.end();

    let reference;
    let current;
    if (s.view === 3) {
      lightViewPass();
      reference = current = readRegion(region);
    } else {
      // The aliased reference, only inside the magnifier: 1 sample per pixel, 1 nearest tap.
      cameraPass(targets.single, camera, light, { ...s, taps: 1, bilinear: false }, region);
      presentPass(targets.single, 1, region);
      reference = readRegion(region);

      if (timer) timer.begin('camera');
      if (s.aa === 'msaa') {
        cameraPass(targets.msaa, camera, light, s, null);
        webgl.resolve(gl, targets.msaa, targets.single); // average the 4 samples of each pixel
        presentPass(targets.single, 1, null);
      } else if (s.aa === 'ssaa') {
        cameraPass(targets.ssaa, camera, light, s, null); // shade 2 × 2 samples per pixel
        presentPass(targets.ssaa, 2, null);
      } else {
        cameraPass(targets.single, camera, light, s, null);
        presentPass(targets.single, 1, null);
      }
      if (timer) timer.end();
      current = readRegion(region);
    }
    drawZoom(zoomRef, zoomImages[0], reference);
    drawZoom(zoomCur, zoomImages[1], current);

    const center = ((Math.floor(ZOOM.h / 2) * ZOOM.w) + Math.floor(ZOOM.w / 2)) * 4;
    const rgb = (p) => `${p[center]} ${p[center + 1]} ${p[center + 2]}`;
    byId('l7-zoom').textContent =
      `x ${region.x}–${region.x + ZOOM.w - 1}, y ${region.y}–${region.y + ZOOM.h - 1}\ncenter ${rgb(reference)} → ${rgb(current)}`;
    if (timer) {
      const t = timer.poll();
      const ms = (v) => (v === undefined ? '…' : `${v.toFixed(3)} ms`);
      byId('l7-gpu').textContent = `shadow pass      ${ms(t.shadow)}\ncamera + present ${ms(t.camera)}`;
    }
    frameMs = frameMs * 0.9 + (performance.now() - started) * 0.1;
    byId('l7-frame').textContent = `${frameMs.toFixed(2)} ms`;
  }

  function frame(time) {
    const dt = lastTime ? Math.min(0.1, (time - lastTime) / 1000) : 0;
    lastTime = time;
    if (toggles.orbit) {
      azimuth = ((azimuth + dt * ORBIT_SPEED + 540) % 360) - 180;
      sliders.az.value = String(Math.round(azimuth));
    }
    render();
    frameRequest = requestAnimationFrame(frame);
  }

  // ---------------------------------------------------------------------------
  // Start-up and controls.
  // ---------------------------------------------------------------------------
  if (gl) {
    setup();
    byId('l7-gpu').textContent = timer ? '…' : 'EXT_disjoint_timer_query_webgl2: —';
  } else {
    byId('l7-no-webgl').hidden = false;
  }

  Object.values(sliders).forEach((el) => el.addEventListener('input', render));
  sliders.az.addEventListener('input', () => { azimuth = Number(sliders.az.value); render(); });
  Object.values(selects).forEach((el) => el.addEventListener('change', render));
  document.querySelectorAll('[data-l7-toggle]').forEach((button) => {
    const key = button.dataset.l7Toggle;
    button.setAttribute('aria-pressed', String(toggles[key]));
    button.addEventListener('click', () => {
      toggles[key] = !toggles[key];
      button.setAttribute('aria-pressed', String(toggles[key]));
      render();
    });
  });
  byId('l7-reset').addEventListener('click', () => {
    Object.entries(DEFAULTS).forEach(([key, value]) => { sliders[key].value = value; });
    Object.entries(CHOICES).forEach(([key, value]) => { selects[key].value = value; });
    azimuth = DEFAULTS.az;
    focus = [...DEFAULT_FOCUS];
    render();
  });

  // Drag to orbit the camera; click without dragging to move the magnifier.
  let drag = null;
  canvas.addEventListener('pointerdown', (event) => {
    drag = { x: event.clientX, y: event.clientY, moved: false };
    canvas.setPointerCapture(event.pointerId);
  });
  canvas.addEventListener('pointermove', (event) => {
    if (!drag) return;
    const dx = event.clientX - drag.x;
    const dy = event.clientY - drag.y;
    if (!drag.moved && Math.hypot(dx, dy) < 4) return;
    drag.moved = true;
    const yaw = Number(sliders.yaw.value) - dx * 0.5;
    sliders.yaw.value = String(((yaw + 540) % 360) - 180);
    sliders.pitch.value = String(Number(sliders.pitch.value) + dy * 0.5);
    drag.x = event.clientX;
    drag.y = event.clientY;
    render();
  });
  canvas.addEventListener('pointerup', (event) => {
    if (drag && !drag.moved) {
      const rect = canvas.getBoundingClientRect();
      focus = [((event.clientX - rect.left) / rect.width) * W, ((event.clientY - rect.top) / rect.height) * H];
      render();
    }
    drag = null;
  });

  window.shadowLab = {
    init: render,
    start() {
      if (frameRequest) return;
      lastTime = 0;
      frameRequest = requestAnimationFrame(frame);
    },
    stop() {
      cancelAnimationFrame(frameRequest);
      frameRequest = 0;
    },
  };
})();
