// Lab 04: decide what is visible. This is Lab 03's pipeline plus three classic tools:
//
//   clip space --(near-plane clipping)--> ÷ w --(back-face culling)--> rasterize --(depth test)--> pixel
//
// Each tool can be switched off on the page, so you can see exactly what it fixes.
// Conventions are Lab 03's: column vectors, column-major storage, the camera looks
// down −z, the OpenGL clip volume −w ≤ x, y, z ≤ w, and window depth
// d = (z_ndc + 1) / 2, so the near plane stores 0 and the far plane stores 1.
//
// Work through README.md in this folder. TODO(exercise X) comments mark code you write.
(function () {
  const canvas = document.querySelector('#l4-screen');
  const ctx = canvas.getContext('2d', { alpha: false });
  const image = ctx.createImageData(canvas.width, canvas.height);
  const W = canvas.width;
  const H = canvas.height;
  const depth = new Float32Array(W * H); // one stored depth per pixel sample
  const overdraw = new Uint8Array(W * H); // how many covered samples landed on each pixel
  const byId = (id) => document.getElementById(id);
  const sliders = Object.fromEntries(['yaw', 'pitch', 'dist', 'near', 'far', 'sep'].map((k) => [k, byId(`l4-${k}`)]));
  const selects = Object.fromEntries(['scene', 'visibility', 'cull', 'clip', 'format', 'debug'].map((k) => [k, byId(`l4-${k}`)]));
  const DEFAULTS = { near: 0.1, far: 100, sep: 0.02 };
  // A good starting camera for each scene; Reset and switching scenes apply it.
  const PRESETS = {
    intersect: { yaw: 35, pitch: 25, dist: 6 },
    zfight: { yaw: 20, pitch: 14, dist: 9 },
    floor: { yaw: 30, pitch: 6, dist: 3.5 },
  };
  const toggles = { spin: true, wire: false };
  const SPIN_SPEED = 0.4;
  let angle = 0;
  let frameRequest = 0;
  let lastTime = 0;
  let frameMs = 0;
  let mouse = null;

  // ---------------------------------------------------------------------------
  // Vectors and 4×4 matrices, unchanged from Lab 03 (plus rotationX/Z and translation).
  // ---------------------------------------------------------------------------
  const sub = (a, b) => [a[0] - b[0], a[1] - b[1], a[2] - b[2]];
  const scale = (a, s) => [a[0] * s, a[1] * s, a[2] * s];
  const dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  const cross = (a, b) => [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]];
  const normalize = (a) => scale(a, 1 / Math.hypot(a[0], a[1], a[2]));
  const radians = (degrees) => (degrees * Math.PI) / 180;
  const fromRows = (r) => [0, 1, 2, 3].flatMap((c) => [r[c], r[4 + c], r[8 + c], r[12 + c]]);
  const at = (m, row, col) => m[col * 4 + row];
  const IDENTITY = fromRows([1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]);

  function multiply(a, b) {
    const out = new Array(16).fill(0);
    for (let c = 0; c < 4; c++) {
      for (let r = 0; r < 4; r++) {
        let sum = 0;
        for (let k = 0; k < 4; k++) sum += at(a, r, k) * at(b, k, c);
        out[c * 4 + r] = sum;
      }
    }
    return out;
  }
  function transform(m, v) {
    return [0, 1, 2, 3].map((r) => at(m, r, 0) * v[0] + at(m, r, 1) * v[1] + at(m, r, 2) * v[2] + at(m, r, 3) * v[3]);
  }
  const translation = (x, y, z) => fromRows([1, 0, 0, x, 0, 1, 0, y, 0, 0, 1, z, 0, 0, 0, 1]);
  function rotationX(a) {
    const c = Math.cos(a);
    const s = Math.sin(a);
    return fromRows([1, 0, 0, 0, 0, c, -s, 0, 0, s, c, 0, 0, 0, 0, 1]);
  }
  function rotationY(a) {
    const c = Math.cos(a);
    const s = Math.sin(a);
    return fromRows([c, 0, s, 0, 0, 1, 0, 0, -s, 0, c, 0, 0, 0, 0, 1]);
  }
  function rotationZ(a) {
    const c = Math.cos(a);
    const s = Math.sin(a);
    return fromRows([c, -s, 0, 0, s, c, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1]);
  }
  function lookAt(eye, target, worldUp) {
    const forward = normalize(sub(target, eye));
    const right = normalize(cross(forward, worldUp));
    const up = cross(right, forward);
    return fromRows([
      ...right, -dot(right, eye),
      ...up, -dot(up, eye),
      ...scale(forward, -1), dot(forward, eye),
      0, 0, 0, 1,
    ]);
  }
  function perspective(fovY, aspect, near, far) {
    const f = 1 / Math.tan(fovY / 2);
    return fromRows([
      f / aspect, 0, 0, 0,
      0, f, 0, 0,
      0, 0, (far + near) / (near - far), (2 * far * near) / (near - far),
      0, 0, -1, 0,
    ]);
  }

  // ---------------------------------------------------------------------------
  // Meshes. Every triangle is counter-clockwise when seen from its front side.
  // ---------------------------------------------------------------------------
  const FACE_COLORS = [
    [255, 103, 128], [86, 225, 192], [105, 155, 255],
    [255, 179, 105], [190, 140, 255], [240, 220, 110],
  ];
  const CUBE = {
    positions: [
      [-1, -1, -1], [1, -1, -1], [1, 1, -1], [-1, 1, -1],
      [-1, -1, 1], [1, -1, 1], [1, 1, 1], [-1, 1, 1],
    ],
    triangles: [
      [4, 5, 6], [4, 6, 7], [1, 0, 3], [1, 3, 2], [5, 1, 2], [5, 2, 6],
      [0, 4, 7], [0, 7, 3], [7, 6, 2], [7, 2, 3], [0, 1, 5], [0, 5, 4],
    ],
    colors: FACE_COLORS.flatMap((rgb) => [rgb, rgb]),
  };
  const quad = (a, b, c, d, rgb) => ({ positions: [a, b, c, d], triangles: [[0, 1, 2], [0, 2, 3]], colors: [rgb, rgb] });
  // A single-sided square in the xy plane, front side facing +z. It is tilted so it
  // cuts diagonally through the cube.
  const SLICER = quad([-2.4, -2.4, 0], [2.4, -2.4, 0], [2.4, 2.4, 0], [-2.4, 2.4, 0], [150, 172, 196]);
  // Two horizontal squares, front side up; the smaller one floats `sep` above the larger.
  const DECAL_BASE = quad([-4, 0, 4], [4, 0, 4], [4, 0, -4], [-4, 0, -4], [255, 103, 128]);
  const DECAL_TOP = quad([-3, 0, 3], [3, 0, 3], [3, 0, -3], [-3, 0, -3], [86, 225, 192]);
  // A floor of four huge tiles. Their triangles reach under and behind the camera,
  // so they cross the near plane while still covering much of the screen.
  const FLOOR = { positions: [], triangles: [], colors: [] };
  for (let i = 0; i < 2; i++) {
    for (let j = 0; j < 2; j++) {
      const x0 = -10 + i * 10;
      const z0 = -10 + j * 10;
      const base = FLOOR.positions.length;
      FLOOR.positions.push([x0, -1.2, z0 + 10], [x0 + 10, -1.2, z0 + 10], [x0 + 10, -1.2, z0], [x0, -1.2, z0]);
      FLOOR.triangles.push([base, base + 1, base + 2], [base, base + 2, base + 3]);
      const rgb = (i + j) % 2 ? [62, 80, 94] : [104, 126, 142];
      FLOOR.colors.push(rgb, rgb);
    }
  }

  function sceneObjects(name) {
    if (name === 'zfight') {
      return [
        { mesh: DECAL_BASE, model: IDENTITY },
        { mesh: DECAL_TOP, model: translation(0, Number(sliders.sep.value), 0) },
      ];
    }
    if (name === 'floor') {
      return [{ mesh: FLOOR, model: IDENTITY }, { mesh: CUBE, model: rotationY(angle) }];
    }
    return [
      { mesh: CUBE, model: rotationY(angle) },
      { mesh: SLICER, model: multiply(rotationZ(0.35), rotationX(-1.0)) },
    ];
  }

  // ---------------------------------------------------------------------------
  // Stage 1 — clipping against the near plane, in homogeneous clip space.
  // ---------------------------------------------------------------------------
  // Sutherland–Hodgman: walk the polygon's edges, keep inside vertices, and add an
  // intersection point wherever an edge crosses the plane. `distance(v) ≥ 0` is inside.
  function clipPolygon(polygon, distance) {
    const out = [];
    for (let i = 0; i < polygon.length; i++) {
      const a = polygon[i];
      const b = polygon[(i + 1) % polygon.length];
      const da = distance(a);
      const db = distance(b);
      if (da >= 0) out.push(a);
      if ((da >= 0) !== (db >= 0)) {
        const t = da / (da - db);
        out.push(a.map((value, k) => value + (b[k] - value) * t));
      }
    }
    return out;
  }
  // The near plane is z = −w, so "in front of it" is z + w ≥ 0. Testing before the
  // divide also rejects points behind the eye, where w < 0.
  const nearDistance = (c) => c[2] + c[3];

  function nearStage(triangle, stats) {
    const mode = selects.clip.value;
    if (mode === 'none') return [triangle]; // divide by w anyway and see what happens
    const inside = triangle.filter((c) => nearDistance(c) >= 0).length;
    if (inside === 3) return [triangle];
    if (mode === 'skip' || inside === 0) {
      stats.dropped++;
      return [];
    }
    // TODO(exercise C): clip against the far plane (w − z ≥ 0) as well, and optionally
    // the four side planes (w ± x ≥ 0, w ± y ≥ 0). Which ones change the picture?
    const polygon = clipPolygon(triangle, nearDistance);
    stats.added += polygon.length - 3;
    const pieces = [];
    for (let i = 1; i + 1 < polygon.length; i++) pieces.push([polygon[0], polygon[i], polygon[i + 1]]);
    return pieces;
  }

  // ---------------------------------------------------------------------------
  // Stage 2 — the divide, the viewport, and back-face culling.
  // ---------------------------------------------------------------------------
  function toScreen(c) {
    const ndc = [c[0] / c[3], c[1] / c[3], c[2] / c[3]];
    return { x: ((ndc[0] + 1) / 2) * W, y: ((1 - ndc[1]) / 2) * H, z: (ndc[2] + 1) / 2, ndc, w: c[3] };
  }

  // Signed area in NDC, where +y is up: positive means counter-clockwise on screen,
  // which this code (like OpenGL's default) treats as front-facing.
  function ndcArea(v) {
    const [a, b, c] = v.map((p) => p.ndc);
    return (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
  }

  // ---------------------------------------------------------------------------
  // Stage 3 — rasterization with an optional depth test.
  // ---------------------------------------------------------------------------
  function edge(a, b, p) {
    return (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x);
  }

  // Round a depth value the way a buffer of the chosen format would store it.
  // The final Math.fround matters: `depth` is a Float32Array, so the new value must be
  // compared in the same precision it would be stored in, or equal codes look different.
  // TODO(exercise B): add a reversed-Z option: store 1 − d, clear to 0, and pass the
  // test when the new value is greater. Compare float32 precision at the far end.
  function quantize(d) {
    const format = selects.format.value;
    if (format === 'f32') return Math.fround(d);
    const levels = 2 ** Number(format) - 1;
    return Math.fround(Math.round(Math.min(1, Math.max(0, d)) * levels) / levels);
  }

  function rasterize(tri, useDepth, stats) {
    const [a, b, c] = tri.v;
    const area = edge(a, b, c);
    if (!(Math.abs(area) > 1e-9)) return; // also rejects NaN after a divide by w = 0
    const minX = Math.max(0, Math.floor(Math.min(a.x, b.x, c.x)));
    const maxX = Math.min(W - 1, Math.ceil(Math.max(a.x, b.x, c.x)));
    const minY = Math.max(0, Math.floor(Math.min(a.y, b.y, c.y)));
    const maxY = Math.min(H - 1, Math.ceil(Math.max(a.y, b.y, c.y)));
    for (let y = minY; y <= maxY; y++) {
      for (let x = minX; x <= maxX; x++) {
        const p = { x: x + 0.5, y: y + 0.5 };
        const w0 = edge(b, c, p) / area;
        const w1 = edge(c, a, p) / area;
        const w2 = edge(a, b, p) / area;
        if (w0 < -1e-8 || w1 < -1e-8 || w2 < -1e-8) continue;
        const i = y * W + x;
        stats.covered++;
        overdraw[i] = Math.min(255, overdraw[i] + 1);
        if (useDepth) {
          // NDC z is an affine function of screen x and y, so plain screen-space
          // barycentric interpolation gives the correct depth for this sample.
          // TODO(exercise F): add a polygon offset (constant + slope-scaled bias) here.
          const d = quantize(w0 * a.z + w1 * b.z + w2 * c.z);
          // TODO(exercise A): make the comparison selectable (LESS, LEQUAL, GREATER).
          if (!(d < depth[i])) {
            stats.failed++;
            continue;
          }
          depth[i] = d;
        }
        image.data[i * 4] = tri.rgb[0];
        image.data[i * 4 + 1] = tri.rgb[1];
        image.data[i * 4 + 2] = tri.rgb[2];
        stats.written++;
      }
    }
  }

  // ---------------------------------------------------------------------------
  // Debug views and lines.
  // ---------------------------------------------------------------------------
  function drawLine(a, b, rgb) {
    const dx = b.x - a.x;
    const dy = b.y - a.y;
    let t0 = 0;
    let t1 = 1;
    for (const [p, q] of [[-dx, a.x], [dx, W - 1e-3 - a.x], [-dy, a.y], [dy, H - 1e-3 - a.y]]) {
      if (p === 0) {
        if (q < 0) return;
      } else if (p < 0) {
        t0 = Math.max(t0, q / p);
      } else {
        t1 = Math.min(t1, q / p);
      }
      if (!(t0 <= t1)) return;
    }
    const steps = Math.ceil(Math.max(Math.abs(dx), Math.abs(dy)) * (t1 - t0));
    for (let i = 0; i <= steps; i++) {
      const t = t0 + (t1 - t0) * (steps === 0 ? 0 : i / steps);
      const index = (Math.floor(a.y + dy * t) * W + Math.floor(a.x + dx * t)) * 4;
      image.data[index] = image.data[index + 1] = image.data[index + 2] = 255;
    }
  }

  const near = () => Number(sliders.near.value);
  const far = () => Number(sliders.far.value);
  // Invert the depth mapping: the distance in front of the camera that stores d.
  const viewDistance = (d) => (near() * far()) / (far() - d * (far() - near()));

  function paintDebugView(view) {
    const data = image.data;
    if (view === 'depth') {
      // Stored depth as gray: 0 (near plane) black, 1 (far plane or cleared) white.
      for (let i = 0; i < depth.length; i++) data[i * 4] = data[i * 4 + 1] = data[i * 4 + 2] = Math.round(Math.min(1, Math.max(0, depth[i])) * 255);
    } else if (view === 'linear') {
      // The same depth converted back to view distance, stretched over what is visible.
      let lo = Infinity;
      let hi = -Infinity;
      for (let i = 0; i < depth.length; i++) {
        if (depth[i] < 1) {
          const z = viewDistance(depth[i]);
          lo = Math.min(lo, z);
          hi = Math.max(hi, z);
        }
      }
      for (let i = 0; i < depth.length; i++) {
        const gray = depth[i] < 1 ? Math.round((255 * (viewDistance(depth[i]) - lo)) / Math.max(1e-6, hi - lo)) : 255;
        data[i * 4] = data[i * 4 + 1] = data[i * 4 + 2] = gray;
      }
    } else if (view === 'overdraw') {
      const ramp = [[10, 13, 18], [34, 64, 120], [60, 170, 150], [240, 210, 90], [255, 110, 90]];
      for (let i = 0; i < overdraw.length; i++) {
        const rgb = ramp[Math.min(ramp.length - 1, overdraw[i])];
        data[i * 4] = rgb[0];
        data[i * 4 + 1] = rgb[1];
        data[i * 4 + 2] = rgb[2];
      }
    }
  }

  // ---------------------------------------------------------------------------
  // One frame.
  // ---------------------------------------------------------------------------
  function render() {
    const started = performance.now();
    Object.values(sliders).forEach((el) => { el.nextElementSibling.value = el.value; });
    const yaw = radians(Number(sliders.yaw.value));
    const pitch = radians(Number(sliders.pitch.value));
    const dist = Number(sliders.dist.value);
    const eye = [dist * Math.cos(pitch) * Math.sin(yaw), dist * Math.sin(pitch), dist * Math.cos(pitch) * Math.cos(yaw)];
    const viewProj = multiply(perspective(radians(60), W / H, near(), far()), lookAt(eye, [0, 0, 0], [0, 1, 0]));

    for (let i = 0; i < image.data.length; i += 4) {
      image.data[i] = 10;
      image.data[i + 1] = 13;
      image.data[i + 2] = 18;
      image.data[i + 3] = 255;
    }
    depth.fill(1); // cleared to the far plane
    overdraw.fill(0);
    const stats = { submitted: 0, culled: 0, dropped: 0, added: 0, rasterized: 0, covered: 0, failed: 0, written: 0 };
    const cull = selects.cull.value;

    // Geometry: transform, clip, divide, cull. Collect what survives.
    const survivors = [];
    for (const { mesh, model } of sceneObjects(selects.scene.value)) {
      const mvp = multiply(viewProj, model);
      const clip = mesh.positions.map((p) => transform(mvp, [...p, 1]));
      mesh.triangles.forEach((triangle, t) => {
        stats.submitted++;
        for (const piece of nearStage(triangle.map((k) => clip[k]), stats)) {
          const v = piece.map(toScreen);
          const area = ndcArea(v);
          if ((cull === 'back' && area <= 0) || (cull === 'front' && area > 0)) {
            stats.culled++;
            continue;
          }
          // Painter's sort key: average distance in front of the camera (w = −z_view).
          survivors.push({ v, rgb: mesh.colors[t], distance: (v[0].w + v[1].w + v[2].w) / 3 });
        }
      });
    }

    // Visibility: draw in submission order, sort far to near, or test depth per sample.
    const method = selects.visibility.value;
    if (method === 'painter') survivors.sort((a, b) => b.distance - a.distance);
    for (const tri of survivors) rasterize(tri, method === 'depth', stats);
    stats.rasterized = survivors.length;

    paintDebugView(selects.debug.value);
    if (toggles.wire) {
      for (const tri of survivors) {
        for (let e = 0; e < 3; e++) drawLine(tri.v[e], tri.v[(e + 1) % 3]);
      }
    }
    ctx.putImageData(image, 0, 0);

    byId('l4-submitted').textContent = String(stats.submitted);
    byId('l4-culled').textContent = String(stats.culled);
    byId('l4-near-stats').textContent = `${stats.dropped} / ${stats.added}`;
    byId('l4-rasterized').textContent = String(stats.rasterized);
    byId('l4-samples').textContent = `${stats.covered.toLocaleString()} / ${stats.failed.toLocaleString()} / ${stats.written.toLocaleString()}`;
    updateMouseReadout();
    frameMs = frameMs * 0.9 + (performance.now() - started) * 0.1;
    byId('l4-frame').textContent = `${frameMs.toFixed(2)} ms`;
  }

  // Depth under the mouse, the distance it encodes, and how far apart two
  // neighbouring depth codes are at that distance.
  function updateMouseReadout() {
    const out = byId('l4-mouse');
    if (!mouse) {
      out.textContent = '—';
      return;
    }
    const d = depth[mouse[1] * W + mouse[0]];
    if (d >= 1) {
      out.textContent = `d = 1 (clear)`;
      return;
    }
    const format = selects.format.value;
    const code = format === 'f32' ? 2 ** (Math.floor(Math.log2(Math.max(d, 1e-30))) - 23) : 1 / (2 ** Number(format) - 1);
    const z = viewDistance(d);
    const step = (code * (far() - near()) * z * z) / (far() * near());
    out.textContent = `d    = ${d.toFixed(7)}\nz    = ${z.toFixed(3)}\nstep ≈ ${step.toPrecision(2)}`;
  }

  function frame(time) {
    const dt = lastTime ? Math.min(0.1, (time - lastTime) / 1000) : 0;
    lastTime = time;
    if (toggles.spin) angle += dt * SPIN_SPEED;
    render();
    frameRequest = requestAnimationFrame(frame);
  }

  // ---------------------------------------------------------------------------
  // Controls.
  // ---------------------------------------------------------------------------
  function applyPreset() {
    const preset = PRESETS[selects.scene.value];
    Object.entries(preset).forEach(([key, value]) => { sliders[key].value = value; });
  }
  Object.values(sliders).forEach((el) => el.addEventListener('input', render));
  Object.values(selects).forEach((el) => el.addEventListener('change', render));
  selects.scene.addEventListener('change', () => { applyPreset(); render(); });
  document.querySelectorAll('[data-l4-toggle]').forEach((button) => {
    const key = button.dataset.l4Toggle;
    button.setAttribute('aria-pressed', String(toggles[key]));
    button.addEventListener('click', () => {
      toggles[key] = !toggles[key];
      button.setAttribute('aria-pressed', String(toggles[key]));
      render();
    });
  });
  byId('l4-reset').addEventListener('click', () => {
    applyPreset();
    Object.entries(DEFAULTS).forEach(([key, value]) => { sliders[key].value = value; });
    const choices = { visibility: 'depth', cull: 'back', clip: 'clip', format: 'f32', debug: 'color' };
    Object.entries(choices).forEach(([key, value]) => { selects[key].value = value; });
    angle = 0;
    render();
  });
  canvas.addEventListener('mousemove', (event) => {
    const rect = canvas.getBoundingClientRect();
    mouse = [
      Math.min(W - 1, Math.floor(((event.clientX - rect.left) * W) / rect.width)),
      Math.min(H - 1, Math.floor(((event.clientY - rect.top) * H) / rect.height)),
    ];
    updateMouseReadout();
  });
  canvas.addEventListener('mouseleave', () => { mouse = null; updateMouseReadout(); });

  window.visibilityLab = {
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
