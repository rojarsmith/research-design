// Lab 05: hand the geometry to the GPU. The cube, the matrices, the culling rule and the
// depth test are the ones from Labs 03–04; WebGL 2 now runs them on the graphics card.
//
//   setup (once):  compile shaders -> link program -> upload vertex + index buffers -> describe attributes
//   every frame:   set state -> upload uniforms -> drawElements -> (read pixels back to compare)
//
// Next to the GPU image, a CPU reference renders the same buffers with JavaScript copies
// of the two shaders, so you can compare the two pixel by pixel.
//
// Work through README.md in this folder. TODO(exercise X) comments mark code you write.
(function () {
  const gpuCanvas = document.querySelector('#l5-gpu');
  const cpuCanvas = document.querySelector('#l5-cpu');
  const cpuCtx = cpuCanvas.getContext('2d', { alpha: false });
  const W = cpuCanvas.width;
  const H = cpuCanvas.height;
  const cpuImage = cpuCtx.createImageData(W, H); // what the CPU reference renders
  const shownImage = cpuCtx.createImageData(W, H); // what the right-hand canvas shows
  const cpuDepth = new Float32Array(W * H);
  const gpuPixels = new Uint8Array(W * H * 4);
  const byId = (id) => document.getElementById(id);
  const DEFAULTS = { yaw: 30, pitch: 22, dist: 4.5, fov: 60 };
  const sliders = Object.fromEntries(Object.keys(DEFAULTS).map((key) => [key, byId(`l5-${key}`)]));
  const shadingSelect = byId('l5-shading');
  const compareSelect = byId('l5-compare');
  const colorSelect = byId('l5-color');
  const editor = byId('l5-fs');
  const toggles = { spin: true, depth: true, cull: true };
  const NEAR = 0.1;
  const FAR = 100;
  const SPIN_SPEED = 0.5;
  let angle = 0;
  let frameRequest = 0;
  let lastTime = 0;
  const timing = { cpu: 0, submit: 0, readback: 0 };

  // ---------------------------------------------------------------------------
  // Shared math from ../common/math.js: the same column-major matrices as Labs 03–04.
  // WebGL reads these 16-number arrays directly with uniformMatrix4fv(location, false, data).
  // ---------------------------------------------------------------------------
  const { multiply, transform, rotationY, radians, orbitEye, lookAt, perspective } = window.cg.math;
  const { srgbToLinear, linearToSrgb } = window.cg.color;

  // ---------------------------------------------------------------------------
  // The mesh, laid out the way the GPU will read it.
  // 24 vertices (4 per face, so each face can have its own UVs) and 36 indices.
  // One vertex is 8 floats = 32 bytes, interleaved:
  //
  //   | x  y  z | r  g  b | u  v |
  //     0         12        24       <- byte offset of each attribute
  // ---------------------------------------------------------------------------
  const CORNERS = [
    [-1, -1, -1], [1, -1, -1], [1, 1, -1], [-1, 1, -1],
    [-1, -1, 1], [1, -1, 1], [1, 1, 1], [-1, 1, 1],
  ];
  // Corner indices of each face, counter-clockwise seen from outside (Lab 03's winding).
  const FACES = [[4, 5, 6, 7], [1, 0, 3, 2], [5, 1, 2, 6], [0, 4, 7, 3], [7, 6, 2, 3], [0, 1, 5, 4]];
  const FACE_UVS = [[0, 0], [1, 0], [1, 1], [0, 1]];
  const FLOATS_PER_VERTEX = 8;
  const STRIDE = FLOATS_PER_VERTEX * 4;
  const vertexList = [];
  const indexList = [];
  FACES.forEach((face, f) => {
    face.forEach((corner, k) => {
      const p = CORNERS[corner];
      vertexList.push(...p, ...p.map((v) => (v + 1) / 2), ...FACE_UVS[k]); // color = position mapped to 0..1
    });
    const b = f * 4;
    indexList.push(b, b + 1, b + 2, b, b + 2, b + 3);
  });
  const VERTEX_DATA = new Float32Array(vertexList);
  const INDEX_DATA = new Uint16Array(indexList);
  const VERTEX_COUNT = VERTEX_DATA.length / FLOATS_PER_VERTEX;

  // ---------------------------------------------------------------------------
  // Shaders (GLSL ES 3.00). The vertex shader runs once per vertex; the fragment
  // shader runs once per covered sample. Everything between them is fixed-function.
  // ---------------------------------------------------------------------------
  const VERTEX_SHADER = `#version 300 es
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec2 aUV;

uniform mat4 uModelViewProjection;
uniform bool uLinear;  // true: decode colors to linear light so they interpolate correctly

out vec3 vColor;  // "varyings": written per vertex, interpolated per sample
out vec2 vUV;

// The vertex colors are sRGB-encoded, the way a color picker writes them.
vec3 srgbToLinear(vec3 c) {
  return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c));
}

void main() {
  vColor = uLinear ? srgbToLinear(aColor) : aColor;
  vUV = aUV;
  gl_Position = uModelViewProjection * vec4(aPosition, 1.0);  // clip space
}
`;
  const FRAGMENT_SHADER = `#version 300 es
precision highp float;

in vec3 vColor;
in vec2 vUV;

uniform int uMode;     // 0 = vertex color, 1 = checker from UV, 2 = depth
uniform float uNear;
uniform float uFar;
uniform float uFocus;  // camera distance, used to stretch the depth view
uniform bool uLinear;  // true: vColor is linear light, so encode to sRGB once, here

out vec4 outColor;

vec3 srgbToLinear(vec3 c) {
  return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c));
}
vec3 linearToSrgb(vec3 c) {
  return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, c));
}

void main() {
  if (uMode == 2) {
    // gl_FragCoord.z is the window depth d in [0, 1]; turn it back into distance.
    // This gray is a measurement for your eyes, not light, so it is not encoded.
    float z = uNear * uFar / (uFar - gl_FragCoord.z * (uFar - uNear));
    float g = clamp((uFocus + 1.8 - z) / 3.6, 0.0, 1.0);
    outColor = vec4(vec3(g), 1.0);
    return;
  }
  vec3 color = vColor;
  if (uMode == 1) {
    vec3 dark = vec3(0.12, 0.16, 0.2);  // sRGB-encoded, like the vertex colors
    float cell = mod(floor(vUV.x * 4.0) + floor(vUV.y * 4.0), 2.0);
    color = mix(uLinear ? srgbToLinear(dark) : dark, vColor, cell);
  }
  outColor = vec4(uLinear ? linearToSrgb(clamp(color, 0.0, 1.0)) : color, 1.0);
}
`;

  // ---------------------------------------------------------------------------
  // WebGL 2 setup: everything here happens once.
  // ---------------------------------------------------------------------------
  const gl = gpuCanvas.getContext('webgl2', { antialias: false, alpha: false });
  let program = null;
  let uniforms = {};
  let vao = null;

  function compileShader(type, source) {
    const shader = gl.createShader(type);
    gl.shaderSource(shader, source);
    gl.compileShader(shader);
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
      const log = gl.getShaderInfoLog(shader);
      gl.deleteShader(shader);
      throw new Error(log);
    }
    return shader;
  }

  // Compile both stages, link them into one program, and look up the uniforms.
  function buildProgram(fragmentSource) {
    const vs = compileShader(gl.VERTEX_SHADER, VERTEX_SHADER);
    let fs;
    try {
      fs = compileShader(gl.FRAGMENT_SHADER, fragmentSource);
    } catch (error) {
      gl.deleteShader(vs);
      throw error;
    }
    const linked = gl.createProgram();
    gl.attachShader(linked, vs);
    gl.attachShader(linked, fs);
    gl.linkProgram(linked);
    gl.deleteShader(vs);
    gl.deleteShader(fs);
    if (!gl.getProgramParameter(linked, gl.LINK_STATUS)) {
      const log = gl.getProgramInfoLog(linked);
      gl.deleteProgram(linked);
      throw new Error(log);
    }
    if (program) gl.deleteProgram(program);
    program = linked;
    uniforms = Object.fromEntries(
      ['uModelViewProjection', 'uMode', 'uNear', 'uFar', 'uFocus', 'uLinear'].map((name) => [name, gl.getUniformLocation(program, name)]),
    );
  }

  function uploadMesh() {
    // A vertex array object records which buffers feed which attributes.
    vao = gl.createVertexArray();
    gl.bindVertexArray(vao);

    const vertexBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, VERTEX_DATA, gl.STATIC_DRAW); // copied to GPU memory once

    // Describe the interleaved layout: location, component count, type, normalize, stride, offset.
    // TODO(exercise C): add a fourth attribute (a normal for Lab 06) or split this
    // buffer into one buffer per attribute; only these calls and the data change.
    gl.enableVertexAttribArray(0);
    gl.vertexAttribPointer(0, 3, gl.FLOAT, false, STRIDE, 0);
    gl.enableVertexAttribArray(1);
    gl.vertexAttribPointer(1, 3, gl.FLOAT, false, STRIDE, 12);
    gl.enableVertexAttribArray(2);
    gl.vertexAttribPointer(2, 2, gl.FLOAT, false, STRIDE, 24);

    const indexBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, indexBuffer); // remembered by the VAO
    gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, INDEX_DATA, gl.STATIC_DRAW);
    gl.bindVertexArray(null);
  }

  function describeGpu() {
    // Many browsers hide the real GPU name behind a generic string. When that happens,
    // ask for the unmasked name; on Windows it usually shows ANGLE translating to Direct3D.
    let renderer = gl.getParameter(gl.RENDERER);
    if (/^WebKit/.test(renderer)) {
      const info = gl.getExtension('WEBGL_debug_renderer_info');
      if (info) renderer = gl.getParameter(info.UNMASKED_RENDERER_WEBGL);
    }
    return `${renderer}\n${gl.getParameter(gl.VERSION)}`;
  }

  // ---------------------------------------------------------------------------
  // The CPU reference: the same buffers, JavaScript copies of the two shaders, and
  // the Lab 04 rasterizer with culling and a depth test.
  // ---------------------------------------------------------------------------
  function cpuVertexShader(i, u) {
    const v = VERTEX_DATA.subarray(i * FLOATS_PER_VERTEX, (i + 1) * FLOATS_PER_VERTEX);
    const rgb = [v[3], v[4], v[5]].map((c) => (u.linear ? srgbToLinear(c) : c));
    return {
      position: transform(u.modelViewProjection, [v[0], v[1], v[2], 1]),
      varyings: [...rgb, v[6], v[7]], // vColor.rgb, vUV.xy
    };
  }

  function cpuFragmentShader(varyings, fragDepth, u) {
    if (u.mode === 2) {
      const z = (u.near * u.far) / (u.far - fragDepth * (u.far - u.near));
      const g = Math.min(1, Math.max(0, (u.focus + 1.8 - z) / 3.6));
      return [g, g, g];
    }
    let color = varyings.slice(0, 3);
    if (u.mode === 1) {
      const cell = (((Math.floor(varyings[3] * 4) + Math.floor(varyings[4] * 4)) % 2) + 2) % 2;
      const dark = [0.12, 0.16, 0.2].map((c) => (u.linear ? srgbToLinear(c) : c));
      color = dark.map((d, k) => d + (color[k] - d) * cell);
    }
    return u.linear ? color.map((c) => linearToSrgb(Math.min(1, Math.max(0, c)))) : color;
  }

  function edge(a, b, p) {
    return (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x);
  }

  function cpuRender(u) {
    const data = cpuImage.data;
    for (let i = 0; i < data.length; i += 4) {
      data[i] = 10;
      data[i + 1] = 13;
      data[i + 2] = 18;
      data[i + 3] = 255;
    }
    cpuDepth.fill(1);
    // Run the vertex shader once per vertex, then assemble triangles from the indices.
    const shaded = [];
    for (let i = 0; i < VERTEX_COUNT; i++) shaded.push(cpuVertexShader(i, u));
    let fragments = 0;
    for (let t = 0; t < INDEX_DATA.length; t += 3) {
      const tri = [INDEX_DATA[t], INDEX_DATA[t + 1], INDEX_DATA[t + 2]].map((i) => shaded[i]);
      if (tri.some((v) => v.position[3] <= 0)) continue; // the camera never gets that close here
      const s = tri.map(({ position: c }) => ({
        x: ((c[0] / c[3] + 1) / 2) * W,
        y: ((1 - c[1] / c[3]) / 2) * H,
        z: (c[2] / c[3] + 1) / 2,
        ndc: [c[0] / c[3], c[1] / c[3]],
      }));
      const ndcArea = (s[1].ndc[0] - s[0].ndc[0]) * (s[2].ndc[1] - s[0].ndc[1]) - (s[1].ndc[1] - s[0].ndc[1]) * (s[2].ndc[0] - s[0].ndc[0]);
      if (u.cull && ndcArea <= 0) continue; // counter-clockwise is front, as gl.frontFace(gl.CCW)
      const area = edge(s[0], s[1], s[2]);
      if (Math.abs(area) < 1e-9) continue;
      const minX = Math.max(0, Math.floor(Math.min(s[0].x, s[1].x, s[2].x)));
      const maxX = Math.min(W - 1, Math.ceil(Math.max(s[0].x, s[1].x, s[2].x)));
      const minY = Math.max(0, Math.floor(Math.min(s[0].y, s[1].y, s[2].y)));
      const maxY = Math.min(H - 1, Math.ceil(Math.max(s[0].y, s[1].y, s[2].y)));
      for (let y = minY; y <= maxY; y++) {
        for (let x = minX; x <= maxX; x++) {
          const p = { x: x + 0.5, y: y + 0.5 };
          const b0 = edge(s[1], s[2], p) / area;
          const b1 = edge(s[2], s[0], p) / area;
          const b2 = edge(s[0], s[1], p) / area;
          if (b0 < -1e-8 || b1 < -1e-8 || b2 < -1e-8) continue;
          fragments++;
          const i = y * W + x;
          const d = Math.fround(b0 * s[0].z + b1 * s[1].z + b2 * s[2].z);
          if (u.depthTest) {
            if (!(d < cpuDepth[i])) continue;
            cpuDepth[i] = d;
          }
          // Screen-space (affine) interpolation of the varyings, as in Labs 01–04.
          // TODO(exercise B): make this perspective-correct. Interpolate varying / w and
          // 1 / w with b0, b1, b2, then divide; the GPU does exactly that for you.
          const varyings = tri[0].varyings.map((_, k) => b0 * tri[0].varyings[k] + b1 * tri[1].varyings[k] + b2 * tri[2].varyings[k]);
          const rgb = cpuFragmentShader(varyings, d, u);
          data[i * 4] = Math.round(Math.min(1, Math.max(0, rgb[0])) * 255);
          data[i * 4 + 1] = Math.round(Math.min(1, Math.max(0, rgb[1])) * 255);
          data[i * 4 + 2] = Math.round(Math.min(1, Math.max(0, rgb[2])) * 255);
        }
      }
    }
    return fragments;
  }

  // ---------------------------------------------------------------------------
  // One frame on the GPU: state, uniforms, one draw call.
  // ---------------------------------------------------------------------------
  function gpuRender(u) {
    gl.viewport(0, 0, W, H);
    gl.clearColor(10 / 255, 13 / 255, 18 / 255, 1);
    gl.clearDepth(1);
    if (u.depthTest) gl.enable(gl.DEPTH_TEST);
    else gl.disable(gl.DEPTH_TEST);
    gl.depthFunc(gl.LESS);
    if (u.cull) gl.enable(gl.CULL_FACE);
    else gl.disable(gl.CULL_FACE);
    gl.cullFace(gl.BACK);
    gl.frontFace(gl.CCW);
    gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

    gl.useProgram(program);
    // The only per-frame uploads: one matrix and four small values.
    // TODO(exercise D): upload model, view and projection separately and multiply them
    // in the vertex shader instead. How much work moves from once per frame to once per vertex?
    gl.uniformMatrix4fv(uniforms.uModelViewProjection, false, new Float32Array(u.modelViewProjection));
    gl.uniform1i(uniforms.uMode, u.mode);
    gl.uniform1f(uniforms.uNear, u.near);
    gl.uniform1f(uniforms.uFar, u.far);
    gl.uniform1f(uniforms.uFocus, u.focus);
    gl.uniform1i(uniforms.uLinear, u.linear ? 1 : 0);
    gl.bindVertexArray(vao);
    gl.drawElements(gl.TRIANGLES, INDEX_DATA.length, gl.UNSIGNED_SHORT, 0);
    gl.bindVertexArray(null);
  }

  // Copy the GPU image back to the CPU. readPixels waits until the GPU has finished,
  // and its rows start at the bottom of the image, so flip them.
  function readGpuImage() {
    gl.readPixels(0, 0, W, H, gl.RGBA, gl.UNSIGNED_BYTE, gpuPixels);
    const flipped = new Uint8Array(W * H * 4);
    for (let y = 0; y < H; y++) {
      flipped.set(gpuPixels.subarray((H - 1 - y) * W * 4, (H - y) * W * 4), y * W * 4);
    }
    return flipped;
  }

  function compare(gpu, view) {
    const cpu = cpuImage.data;
    const out = shownImage.data;
    let differing = 0;
    let maxDelta = 0;
    for (let i = 0; i < cpu.length; i += 4) {
      const delta = Math.max(Math.abs(gpu[i] - cpu[i]), Math.abs(gpu[i + 1] - cpu[i + 1]), Math.abs(gpu[i + 2] - cpu[i + 2]));
      maxDelta = Math.max(maxDelta, delta);
      if (delta > 2) differing++; // ignore ±1–2 from rounding to 8 bits
      if (view === 'diff') {
        for (let k = 0; k < 3; k++) out[i + k] = Math.min(255, Math.abs(gpu[i + k] - cpu[i + k]) * 8);
      } else if (view === 'mask') {
        const mismatch = delta > 2;
        out[i] = mismatch ? 255 : cpu[i] * 0.3;
        out[i + 1] = mismatch ? 60 : cpu[i + 1] * 0.3;
        out[i + 2] = mismatch ? 200 : cpu[i + 2] * 0.3;
      }
      out[i + 3] = 255;
    }
    return { differing, maxDelta };
  }

  // ---------------------------------------------------------------------------
  // Frame loop and readouts.
  // ---------------------------------------------------------------------------
  function frameUniforms() {
    const dist = Number(sliders.dist.value);
    const eye = orbitEye(Number(sliders.yaw.value), Number(sliders.pitch.value), dist);
    const projection = perspective(radians(Number(sliders.fov.value)), W / H, NEAR, FAR);
    return {
      modelViewProjection: multiply(projection, multiply(lookAt(eye, [0, 0, 0], [0, 1, 0]), rotationY(angle))),
      mode: Number(shadingSelect.value),
      near: NEAR,
      far: FAR,
      focus: dist,
      linear: colorSelect.value === 'linear',
      depthTest: toggles.depth,
      cull: toggles.cull,
    };
  }

  function render() {
    Object.values(sliders).forEach((el) => { el.nextElementSibling.value = el.value; });
    const u = frameUniforms();
    let started = performance.now();
    const fragments = cpuRender(u);
    timing.cpu = timing.cpu * 0.9 + (performance.now() - started) * 0.1;
    const view = compareSelect.value;
    if (gl && program) {
      started = performance.now();
      gpuRender(u);
      timing.submit = timing.submit * 0.9 + (performance.now() - started) * 0.1;
      started = performance.now();
      const gpu = readGpuImage();
      timing.readback = timing.readback * 0.9 + (performance.now() - started) * 0.1;
      const result = compare(gpu, view);
      const percent = ((100 * result.differing) / (W * H)).toFixed(2);
      byId('l5-diff').textContent = `${result.differing.toLocaleString()} px (${percent}%)\nmax Δ ${result.maxDelta}`;
    }
    cpuCtx.putImageData(view === 'cpu' || !gl ? cpuImage : shownImage, 0, 0);
    byId('l5-invocations').textContent = `VS ${VERTEX_COUNT}–${INDEX_DATA.length}\nFS ≈ ${fragments.toLocaleString()}`;
    byId('l5-timing').textContent =
      `CPU raster ${timing.cpu.toFixed(2)} ms\nGL calls   ${timing.submit.toFixed(2)} ms\nreadPixels ${timing.readback.toFixed(2)} ms`;
  }

  function frame(time) {
    const dt = lastTime ? Math.min(0.1, (time - lastTime) / 1000) : 0;
    lastTime = time;
    if (toggles.spin) angle += dt * SPIN_SPEED;
    render();
    frameRequest = requestAnimationFrame(frame);
  }

  function compileFromEditor() {
    const log = byId('l5-log');
    try {
      buildProgram(editor.value);
      log.textContent = `✓ ${new Date().toLocaleTimeString()}`;
    } catch (error) {
      log.textContent = String(error.message).trim(); // the old program keeps running
    }
    render();
  }

  // ---------------------------------------------------------------------------
  // Start-up and controls.
  // ---------------------------------------------------------------------------
  editor.value = FRAGMENT_SHADER;
  byId('l5-buffers').textContent =
    `VBO ${VERTEX_COUNT} × ${STRIDE} B = ${VERTEX_DATA.byteLength} B\nIBO ${INDEX_DATA.length} × 2 B = ${INDEX_DATA.byteLength} B`;
  byId('l5-uploads').textContent = 'mat4              64 B\nint, bool, 3 float 20 B\ntotal             84 B';
  if (gl) {
    buildProgram(FRAGMENT_SHADER);
    uploadMesh();
    byId('l5-gpu-info').textContent = describeGpu();
  } else {
    byId('l5-gpu-info').textContent = 'WebGL 2: —';
    byId('l5-no-webgl').hidden = false;
  }

  Object.values(sliders).forEach((el) => el.addEventListener('input', render));
  [shadingSelect, compareSelect, colorSelect].forEach((el) => el.addEventListener('change', render));
  document.querySelectorAll('[data-l5-toggle]').forEach((button) => {
    const key = button.dataset.l5Toggle;
    button.setAttribute('aria-pressed', String(toggles[key]));
    button.addEventListener('click', () => {
      toggles[key] = !toggles[key];
      button.setAttribute('aria-pressed', String(toggles[key]));
      render();
    });
  });
  byId('l5-reset').addEventListener('click', () => {
    Object.entries(DEFAULTS).forEach(([key, value]) => { sliders[key].value = value; });
    angle = 0;
    render();
  });
  byId('l5-compile').addEventListener('click', () => { if (gl) compileFromEditor(); });
  byId('l5-restore').addEventListener('click', () => {
    editor.value = FRAGMENT_SHADER;
    if (gl) compileFromEditor();
  });

  window.gpuLab = {
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
